/**
 * @file      cote.c
 * @brief     Cote library
 *
 * MIT License
 *
 * Copyright (c) 2021-2023 joelguittet and c-cote contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <regex.h>
#include <cJSON.h>
#include <time.h>

#include "cote.h"

/******************************************************************************/
/* Prototypes                                                                 */
/******************************************************************************/

/**
 * @brief Callback function called when Axon socket is bound
 * @param axon Axon instance
 * @param port Port on which the socket is bound
 * @param user User data
 */
static void cote_axon_bind_cb(axon_t *axon, uint16_t port, void *user);

/**
 * @brief Callback function invoked when Axon data is received
 * @param axon Axon instance
 * @param amp AMP message
 * @param user User data
 * @return Reply to the message received
 */
static amp_msg_t *cote_axon_message_cb(axon_t *axon, amp_msg_t *amp, void *user);

/**
 * @brief Callback function called to handle error from Axon instance
 * @param axon Axon instance
 * @param err Error as string
 * @param user User data
 */
static void cote_axon_error_cb(axon_t *axon, char *err, void *user);

/**
 * @brief Format fulltopic
 * @param cote Cote instance
 * @param topic Topic
 * @return Fulltopic if the fonction succeeded, NULL otherwise
 */
static char *cote_axon_format_fulltopic(cote_t *cote, char *topic);

/**
 * @brief Callback function invoked when a discovery node instance is added
 * @param discover Discover instance
 * @param node Node
 * @param user User data
 */
static void cote_discovery_added_cb(discover_t *discover, discover_node_t *node, void *user);

/**
 * @brief Callback function invoked when discovery node instance is removed
 * @param discover Discover instance
 * @param node Node
 * @param user User data
 */
static void cote_discovery_removed_cb(discover_t *discover, discover_node_t *node, void *user);

/**
 * @brief Callback function called to handle error from Axon instance
 * @param discover Discover instance
 * @param err Error as string
 * @param user User data
 */
static void cote_discovery_error_cb(discover_t *discover, char *err, void *user);

/**
 * @brief Function used to set Discovery instance advertisement
 * @param cote Cote instance
 * @return 0 if the function succeeded, -1 otherwise
 */
static int cote_discovery_set_advertisement(cote_t *cote);

/**
 * @brief Function used to check discovery callback node
 * @param cote Cote instance
 * @param node Node
 * @return 0 if the function succeeded (node is valid), -1 otherwise
 */
static int cote_discovery_check_node(cote_t *cote, discover_node_t *node);

/******************************************************************************/
/* Functions                                                                  */
/******************************************************************************/

/**
 * @brief Function used to create cote instance
 * @param type Type of Cote instance
 * @param name Name of Cote instance
 * @return Cote instance if the function succeeded, NULL otherwise
 */
cote_t *
cote_create(char *type, char *name) {

    assert(NULL != type);
    assert(NULL != name);

    /* Create cote instance */
    cote_t *cote = (cote_t *)malloc(sizeof(cote_t));
    if (NULL == cote) {
        /* Unable to allocate memory */
        return NULL;
    }
    memset(cote, 0, sizeof(cote_t));

    /* Set Cote type */
    if (!strcmp(type, "pub")) {
        cote->type = COTE_TYPE_PUB;
    } else if (!strcmp(type, "sub")) {
        cote->type = COTE_TYPE_SUB;
    } else if (!strcmp(type, "req")) {
        cote->type = COTE_TYPE_REQ;
    } else if (!strcmp(type, "rep")) {
        cote->type = COTE_TYPE_REP;
    } else if (!strcmp(type, "mon")) {
        cote->type = COTE_TYPE_MON;
    } else {
        /* Incorrect type */
        free(cote);
        return NULL;
    }

    /* Set Cote name */
    if (NULL == (cote->name = strdup(name))) {
        /* Unable to allocate memory */
        free(cote);
        return NULL;
    }

    /* Create discover instance */
    if (NULL == (cote->discover = discover_create())) {
        /* Unable to create Discover instance */
        free(cote);
        return NULL;
    }

    /* Definition of discover added/removed/error callbacks */
    discover_on(cote->discover, "added", &cote_discovery_added_cb, cote);
    discover_on(cote->discover, "removed", &cote_discovery_removed_cb, cote);
    discover_on(cote->discover, "error", &cote_discovery_error_cb, cote);

    /* Set discover options */
    int helloInterval = 2000;
    discover_set_option(cote->discover, "helloInterval", &helloInterval);
    int checkInterval = 4000;
    discover_set_option(cote->discover, "checkInterval", &checkInterval);
    int nodeTimeout = 5000;
    discover_set_option(cote->discover, "nodeTimeout", &nodeTimeout);
    int masterTimeout = 6000;
    discover_set_option(cote->discover, "masterTimeout", &masterTimeout);

    /* Treatment depending of Cote instance type */
    if (COTE_TYPE_MON != cote->type) {

        /* Create axon instance */
        if (NULL == (cote->axon = axon_create(type))) {
            /* Unable to create Axon instance */
            discover_release(cote->discover);
            free(cote);
            return NULL;
        }

        /* Definition of axon error callback */
        axon_on(cote->axon, "error", &cote_axon_error_cb, cote);
    }

    /* Initialize semaphore used to access options */
    sem_init(&cote->options.sem, 0, 1);

    /* Initialize semaphore used to access subscriptions */
    sem_init(&cote->subs.sem, 0, 1);

    return cote;
}

/**
 * @brief Set cote options
 * @param cote Cote instance
 * @param option Option by name
 * @param value New value of the option
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_set_option(cote_t *cote, char *option, void *value) {

    assert(NULL != cote);
    assert(NULL != cote->discover);
    assert(NULL != option);

    int ret = -1;

    /* Wait options semaphore */
    sem_wait(&cote->options.sem);

    /* Treatment depending of the option */
    if (!strcmp("helloInterval", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("checkInterval", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("nodeTimeout", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("masterTimeout", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("address", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("port", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("broadcast", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("multicast", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("multicastTTL", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("unicast", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("key", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("mastersRequired", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("weight", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("client", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("reuseAddr", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("ignoreProcess", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("ignoreInstance", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("hostname", option)) {
        ret = discover_set_option(cote->discover, option, value);
    } else if (!strcmp("namespace", option)) {
        if (NULL != cote->options.namespace_) {
            free(cote->options.namespace_);
        }
        cote->options.namespace_ = strdup((char *)value);
        if (NULL != cote->options.namespace_) {
            ret = 0;
        }
    } else if (!strcmp("useHostNames", option)) {
        cote->options.use_hostname = *((bool *)value);
        ret                        = 0;
    } else if (!strcmp("advertisement", option)) {
        if (NULL != cote->options.advertisement) {
            cJSON_Delete(cote->options.advertisement);
        }
        cote->options.advertisement = (NULL != value) ? cJSON_Duplicate((cJSON *)value, 1) : NULL;
        ret                         = 0;
    } else if (!strcmp("broadcasts", option)) {
        if (NULL != cote->options.broadcasts) {
            cJSON_Delete(cote->options.broadcasts);
        }
        cote->options.broadcasts = (NULL != value) ? cJSON_Duplicate((cJSON *)value, 1) : NULL;
        ret                      = 0;
    } else if (!strcmp("subscribesTo", option)) {
        if (NULL != cote->options.subscribesTo) {
            cJSON_Delete(cote->options.subscribesTo);
        }
        cote->options.subscribesTo = (NULL != value) ? cJSON_Duplicate((cJSON *)value, 1) : NULL;
        ret                        = 0;
    } else if (!strcmp("requests", option)) {
        if (NULL != cote->options.requests) {
            cJSON_Delete(cote->options.requests);
        }
        cote->options.requests = (NULL != value) ? cJSON_Duplicate((cJSON *)value, 1) : NULL;
        ret                    = 0;
    } else if (!strcmp("respondsTo", option)) {
        if (NULL != cote->options.respondsTo) {
            cJSON_Delete(cote->options.respondsTo);
        }
        cote->options.respondsTo = (NULL != value) ? cJSON_Duplicate((cJSON *)value, 1) : NULL;
        ret                      = 0;
    }

    /* Release options semaphore */
    sem_post(&cote->options.sem);

    /* Update advertisement, if required */
    if (0 == ret) {
        ret = cote_discovery_set_advertisement(cote);
    }

    return ret;
}

/**
 * @brief Start Cote instance
 * @param cote Cote instance
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_start(cote_t *cote) {

    assert(NULL != cote);

    /* Treatment depending of cote type */
    if ((COTE_TYPE_SUB == cote->type) || (COTE_TYPE_REP == cote->type)) {

        /* Definition of axon message callback */
        axon_on(cote->axon, "message", &cote_axon_message_cb, cote);
    }

    if ((COTE_TYPE_PUB == cote->type) || (COTE_TYPE_REP == cote->type)) {

        /* Definition of axon bind callback */
        axon_on(cote->axon, "bind", &cote_axon_bind_cb, cote);

        /* Bind axon instance to any available port */
        if (0 != axon_bind(cote->axon, 0)) {
            /* Unable to bind Axon instance */
            return -1;
        }

    } else if ((COTE_TYPE_SUB == cote->type) || (COTE_TYPE_REQ == cote->type) || (COTE_TYPE_MON == cote->type)) {

        /* Set Discovery advertisement */
        if (0 != cote_discovery_set_advertisement(cote)) {
            /* Unable to set advertisement */
            return -1;
        }

        /* Start discover instance */
        if (0 != discover_start(cote->discover)) {
            /* Unable to create Discover instance */
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Function used to set advertisement
 * @param cote Cote instance
 * @param advertisement Advestisement object
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_advertise(cote_t *cote, cJSON *advertisement) {

    assert(NULL != cote);

    /* Wait options semaphore */
    sem_wait(&cote->options.sem);

    /* Set advertisement */
    if (NULL != cote->options.advertisement) {
        cJSON_Delete(cote->options.advertisement);
    }
    cote->options.advertisement = (NULL != advertisement) ? cJSON_Duplicate(advertisement, 1) : NULL;

    /* Release options semaphore */
    sem_post(&cote->options.sem);

    /* Update advertisement, if required */
    return cote_discovery_set_advertisement(cote);
}

/**
 * @brief Register callbacks
 * @param cote Cote instance
 * @param topic Topic
 * @param fct Callback funtion
 * @param user User data
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_on(cote_t *cote, char *topic, void *fct, void *user) {

    assert(NULL != cote);
    assert(NULL != topic);

    /* Record callback depending of the topic */
    if (!strcmp(topic, "added")) {
        cote->cb.added.fct  = fct;
        cote->cb.added.user = user;
    } else if (!strcmp(topic, "removed")) {
        cote->cb.removed.fct  = fct;
        cote->cb.removed.user = user;
    } else if (!strcmp(topic, "message")) {
        cote->cb.message.fct  = fct;
        cote->cb.message.user = user;
    } else if (!strcmp(topic, "error")) {
        cote->cb.error.fct  = fct;
        cote->cb.error.user = user;
    }

    return 0;
}

/**
 * @brief Subscribe to wanted topic
 * @param cote Cote instance
 * @param topic Topic
 * @param fct Callback funtion
 * @param user User data
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_subscribe(cote_t *cote, char *topic, void *fct, void *user) {

    assert(NULL != cote);
    assert(NULL != topic);

    /* Check Cote instance type */
    if ((COTE_TYPE_SUB != cote->type) && (COTE_TYPE_REP != cote->type)) {
        /* Not compatible */
        return -1;
    }

    /* Format full topic */
    char *fulltopic = cote_axon_format_fulltopic(cote, topic);
    if (NULL == fulltopic) {
        /* Unable to allocate memory */
        return -1;
    }

    int         ret      = 0;
    cote_sub_t *last_sub = NULL;

    /* Wait semaphore */
    sem_wait(&cote->subs.sem);

    /* Parse subscriptions, update callback and user data if topic is found */
    cote_sub_t *curr_sub = cote->subs.first;
    while (NULL != curr_sub) {
        if (!strcmp(fulltopic, curr_sub->topic)) {
            curr_sub->fct  = fct;
            curr_sub->user = user;
            goto LEAVE;
        }
        last_sub = curr_sub;
        curr_sub = curr_sub->next;
    }

    /* Subscription not found, add a new one */
    cote_sub_t *new_sub = (cote_sub_t *)malloc(sizeof(cote_sub_t));
    if (NULL == new_sub) {
        /* Unable to allocate memory */
        ret = -1;
        goto LEAVE;
    }
    memset(new_sub, 0, sizeof(cote_sub_t));
    new_sub->topic = strdup(fulltopic);
    if (NULL == new_sub->topic) {
        /* Unable to allocate memory */
        free(new_sub);
        ret = -1;
        goto LEAVE;
    }
    new_sub->fct  = fct;
    new_sub->user = user;
    if (NULL != last_sub) {
        last_sub->next = new_sub;
    } else {
        cote->subs.first = new_sub;
    }

LEAVE:

    /* Release semaphore */
    sem_post(&cote->subs.sem);

    /* Release memory */
    free(fulltopic);

    return ret;
}

/**
 * @brief Unubscribe to wanted topic
 * @param cote Cote instance
 * @param topic Topic
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_unsubscribe(cote_t *cote, char *topic) {

    assert(NULL != cote);
    assert(NULL != topic);

    /* Check Cote instance type */
    if ((COTE_TYPE_SUB != cote->type) && (COTE_TYPE_REP != cote->type)) {
        /* Not compatible */
        return -1;
    }

    cote_sub_t *last_sub = NULL;

    /* Wait semaphore */
    sem_wait(&cote->subs.sem);

    /* Parse subscriptions, remove subscription if topic is found */
    cote_sub_t *curr_sub = cote->subs.first;
    while (NULL != curr_sub) {
        if (!strcmp(topic, curr_sub->topic)) {
            if (cote->subs.first == curr_sub) {
                cote->subs.first = curr_sub->next;
            } else {
                last_sub->next = curr_sub->next;
            }
            free(curr_sub->topic);
            free(curr_sub);
            goto LEAVE;
        }
        last_sub = curr_sub;
        curr_sub = curr_sub->next;
    }

LEAVE:

    /* Release semaphore */
    sem_post(&cote->subs.sem);

    return 0;
}

/**
 * @brief Function used to send data to all connected subscribers
 * @param cote Cote instance
 * @param topic Topic of the message
 * @param count Amount of data to be sent
 * @param ... type, data Array of type and data (and size for blob type) to be sent, AMP response message and timeout for Requester instance
 * @return 0 if the function succeeded, -1 otherwise
 */
int
cote_send(cote_t *cote, char *topic, int count, ...) {

    assert(NULL != cote);
    assert(NULL != cote->axon);
    assert(NULL != topic);

    /* Check Cote instance type */
    if ((COTE_TYPE_PUB != cote->type) && (COTE_TYPE_REQ != cote->type)) {
        /* Not compatible */
        return -1;
    }

    int ret = -1;

    /* Treatment depending of Cote instance type (format of messages is very different !) */
    if (COTE_TYPE_PUB == cote->type) {

        /* Format full topic */
        char *fulltopic = cote_axon_format_fulltopic(cote, topic);
        if (NULL == fulltopic) {
            /* Unable to allocate memory */
            return -1;
        }

        /* Retrieve params */
        va_list params;
        va_start(params, count);

        /* Send message */
        ret = axon_vsend(cote->axon, count + 1, AMP_TYPE_STRING, fulltopic, params);

        /* End of params */
        va_end(params);

        /* Release memory */
        free(fulltopic);

    } else if (COTE_TYPE_REQ == cote->type) {

        cJSON *     json    = NULL;
        amp_msg_t **resp    = NULL;
        int         timeout = 0;

        /* Retrieve params and format message */
        va_list params;
        va_start(params, count);
        amp_type_e type = va_arg(params, int);
        if (AMP_TYPE_JSON == type) {
            cJSON *tmp = va_arg(params, cJSON *);
            if (NULL != tmp) {
                json = cJSON_Duplicate(tmp, 1);
                if (NULL != json) {
                    cJSON_AddStringToObject(json, "type", topic);
                }
            }
        }
        resp    = va_arg(params, amp_msg_t **);
        timeout = va_arg(params, int);
        va_end(params);

        /* Send message */
        if (NULL != json) {
            ret = axon_send(cote->axon, 1, AMP_TYPE_JSON, json, resp, timeout);
            cJSON_Delete(json);
        }
    }

    return ret;
}

/**
 * @brief Release cote instance
 * @param cote Cote instance
 */
void
cote_release(cote_t *cote) {

    /* Release cote instance */
    if (NULL != cote) {

        /* Release discover instance */
        discover_release(cote->discover);

        /* Release axon instance */
        axon_release(cote->axon);

        /* Release subscriptions */
        sem_wait(&cote->subs.sem);
        cote_sub_t *curr_sub = cote->subs.first;
        while (NULL != curr_sub) {
            cote_sub_t *tmp = curr_sub;
            curr_sub        = curr_sub->next;
            if (NULL != tmp->topic) {
                free(tmp->topic);
            }
            free(tmp);
        }
        sem_post(&cote->subs.sem);
        sem_close(&cote->subs.sem);

        /* Release options */
        sem_wait(&cote->options.sem);
        if (NULL != cote->options.namespace_) {
            free(cote->options.namespace_);
        }
        if (NULL != cote->options.advertisement) {
            cJSON_Delete(cote->options.advertisement);
        }
        if (NULL != cote->options.broadcasts) {
            cJSON_Delete(cote->options.broadcasts);
        }
        if (NULL != cote->options.subscribesTo) {
            cJSON_Delete(cote->options.subscribesTo);
        }
        if (NULL != cote->options.requests) {
            cJSON_Delete(cote->options.requests);
        }
        if (NULL != cote->options.respondsTo) {
            cJSON_Delete(cote->options.respondsTo);
        }
        sem_post(&cote->options.sem);
        sem_close(&cote->options.sem);

        /* Release name */
        if (NULL != cote->name) {
            free(cote->name);
        }

        /* Release Cote instance */
        free(cote);
    }
}

/**
 * @brief Callback function called when Axon socket is bound
 * @param axon Axon instance
 * @param port Port on which the socket is bound
 * @param user User data
 */
static void
cote_axon_bind_cb(axon_t *axon, uint16_t port, void *user) {

    (void)axon;
    assert(NULL != user);

    /* Retrieve cote instance using user data */
    cote_t *cote = (cote_t *)user;

    /* Check Cote instance type */
    if ((COTE_TYPE_PUB != cote->type) && (COTE_TYPE_REP != cote->type)) {
        /* Not compatible */
        return;
    }

    /* Memorize port */
    cote->port = port;

    /* Set Discovery advertisement */
    if (0 != cote_discovery_set_advertisement(cote)) {
        /* Unable to set advertisement */
        /* Invoke error callback if defined */
        if (NULL != cote->cb.error.fct) {
            cote->cb.error.fct(cote, "cote: unable to set advertisement", cote->cb.error.user);
        }
        return;
    }

    /* Start discover instance */
    if (0 != discover_start(cote->discover)) {
        /* Unable to create Discover instance */
        /* Invoke error callback if defined */
        if (NULL != cote->cb.error.fct) {
            cote->cb.error.fct(cote, "cote: unable to start discover instance", cote->cb.error.user);
        }
        return;
    }
}

/**
 * @brief Callback function invoked when Axon data is received
 * @param axon Axon instance
 * @param amp AMP message
 * @param user User data
 * @return Reply to the message received
 */
static amp_msg_t *
cote_axon_message_cb(axon_t *axon, amp_msg_t *amp, void *user) {

    (void)axon;
    assert(NULL != amp);
    assert(NULL != user);

    amp_msg_t *ret = NULL;

    /* Retrieve cote instance using user data */
    cote_t *cote = (cote_t *)user;

    /* Check Cote instance type */
    if ((COTE_TYPE_SUB != cote->type) && (COTE_TYPE_REP != cote->type)) {
        /* Not compatible */
        return NULL;
    }

    /* Check the message has at least one field */
    if ((NULL == amp->first) || (NULL == amp->last)) {
        /* Invalid message */
        return NULL;
    }

    /* Check if message callback is define */
    if (NULL != cote->cb.message.fct) {

        /* Invoke message callback */
        cote->cb.message.fct(cote, amp, cote->cb.message.user);
    }

    /* Wait subscriptions semaphore */
    sem_wait(&cote->subs.sem);

    /* Treatment depending of Cote instance type */
    if (COTE_TYPE_SUB == cote->type) {

        /* Cote is Subscriber - Invoke susbscriptions callback(s) if defined and if the first field of the AMP message is a string */
        if ((NULL != cote->subs.first) && (AMP_TYPE_STRING == amp->first->type) && (NULL != amp->first->data)) {

            /* Extract topic from the message */
            amp_field_t *topic_field = amp->first;
            amp->first               = amp->first->next;
            amp->count--;

            /* Parse all subscriptions */
            cote_sub_t *curr_sub = cote->subs.first;
            while (NULL != curr_sub) {
                if (NULL != curr_sub->fct) {
                    regex_t regex;
                    if (0 == regcomp(&regex, curr_sub->topic, REG_NOSUB | REG_EXTENDED)) {
                        if (0 == regexec(&regex, topic_field->data, 0, NULL, 0)) {
                            /* Topic match subscription */
                            /* Invoke subscription callback */
                            ret = curr_sub->fct(cote,
                                                topic_field->data + strlen("message::")
                                                    + ((NULL != cote->options.namespace_) ? (strlen(cote->options.namespace_) + 2) : 0),
                                                amp,
                                                curr_sub->user);
                        }
                        regfree(&regex);
                    }
                }
                curr_sub = curr_sub->next;
            }
            free(topic_field->data);
            free(topic_field);
        }

    } else if (COTE_TYPE_REP == cote->type) {

        /* Cote is Responder - Invoke susbscriptions callback(s) if defined and if the first field of the AMP message is a JSON */
        if ((NULL != cote->subs.first) && (AMP_TYPE_JSON == amp->first->type) && (NULL != amp->first->data)) {

            /* Extract topic from the message */
            cJSON *tmp = cJSON_DetachItemFromObjectCaseSensitive(amp->first->data, "type");
            if (NULL != tmp) {
                char *topic = cJSON_GetStringValue(tmp);
                if (NULL != topic) {
                    /* Parse all subscriptions */
                    cote_sub_t *curr_sub = cote->subs.first;
                    while (NULL != curr_sub) {
                        if (NULL != curr_sub->fct) {
                            regex_t regex;
                            if (0 == regcomp(&regex, curr_sub->topic, REG_NOSUB | REG_EXTENDED)) {
                                if (0 == regexec(&regex, topic, 0, NULL, 0)) {
                                    /* Topic match subscription */
                                    /* Invoke subscription callback */
                                    ret = curr_sub->fct(cote, topic, amp, curr_sub->user);
                                }
                                regfree(&regex);
                            }
                        }
                        curr_sub = curr_sub->next;
                    }
                }
                cJSON_Delete(tmp);
            }
        }
    }

    /* Release subscriptions semaphore */
    sem_post(&cote->subs.sem);

    return ret;
}

/**
 * @brief Callback function called to handle error from Axon instance
 * @param axon Axon instance
 * @param err Error as string
 * @param user User data
 */
static void
cote_axon_error_cb(axon_t *axon, char *err, void *user) {

    (void)axon;
    assert(NULL != err);
    assert(NULL != user);

    /* Retrieve cote instance using user data */
    cote_t *cote = (cote_t *)user;

    /* Invoke error callback if defined */
    if (NULL != cote->cb.error.fct) {
        cote->cb.error.fct(cote, err, cote->cb.error.user);
    }
}

/**
 * @brief Format fulltopic
 * @param cote Cote instance
 * @param topic Topic
 * @return Fulltopic if the fonction succeeded, NULL otherwise
 */
static char *
cote_axon_format_fulltopic(cote_t *cote, char *topic) {

    char *fulltopic = NULL;

    /* Treatment depending of Cote instance type */
    if ((COTE_TYPE_PUB == cote->type) || (COTE_TYPE_SUB == cote->type)) {

        /* Wait options semaphore */
        sem_wait(&cote->options.sem);

        /* Format full topic */
        size_t len = 0;
        if (NULL != cote->options.namespace_) {
            len = strlen("message::") + strlen(cote->options.namespace_) + strlen("::") + strlen(topic) + 1;
        } else {
            len = strlen("message::") + strlen(topic) + 1;
        }
        if (NULL == (fulltopic = (char *)malloc(len))) {
            /* Unable to allocate memory */
            sem_post(&cote->options.sem);
            return NULL;
        }
        memset(fulltopic, 0, len);
        strcat(fulltopic, "message::");
        if (NULL != cote->options.namespace_) {
            strcat(fulltopic, cote->options.namespace_);
            strcat(fulltopic, "::");
        }
        strcat(fulltopic, topic);

        /* Release options semaphore */
        sem_post(&cote->options.sem);

    } else if ((COTE_TYPE_REQ == cote->type) || (COTE_TYPE_REP == cote->type)) {

        /* Fulltopic is the topic itself */
        fulltopic = strdup(topic);
    }

    return fulltopic;
}

/**
 * @brief Callback function invoked when a discovery node instance is added
 * @param discover Discover instance
 * @param node Node
 * @param user User data
 */
static void
cote_discovery_added_cb(discover_t *discover, discover_node_t *node, void *user) {

    (void)discover;
    assert(NULL != node);
    assert(NULL != user);

    /* Retrieve cote instance using user data */
    cote_t *cote = (cote_t *)user;

    /* Check node content */
    if (0 != cote_discovery_check_node(cote, node)) {
        /* Invalid node, ignore message */
        return;
    }

    /* Treatment depending of the Cote instance type */
    if ((COTE_TYPE_SUB == cote->type) || (COTE_TYPE_REQ == cote->type)) {

        /* Get port of the publisher/replier */
        uint16_t port = 0;
        if (NULL != node->data.advertisement) {
            cJSON *tmp = cJSON_GetObjectItemCaseSensitive(node->data.advertisement, "port");
            if (NULL != tmp) {
                port = (uint16_t)cJSON_GetNumberValue(tmp);
            }
        }
        if (0 == port) {
            /* Unable to find port */
            return;
        }

        /* Wait options semaphore */
        sem_wait(&cote->options.sem);

        /* Check if already connected to this node */
        if (true == axon_is_connected(cote->axon, (true == cote->options.use_hostname) ? node->hostname : node->address, port)) {
            /* Already connected, ignore new node */
            sem_post(&cote->options.sem);
            return;
        }

        /* Parse broadcasts/respondsTo and subscribesTo/requests arrays to check if we need to connect to this publisher instance */
        bool   match         = false;
        cJSON *server_topics = NULL;
        cJSON *client_topics = NULL;
        if (COTE_TYPE_SUB == cote->type) {
            server_topics = cJSON_GetObjectItemCaseSensitive(node->data.advertisement, "broadcasts");
            client_topics = cote->options.subscribesTo;
        } else if (COTE_TYPE_REQ == cote->type) {
            server_topics = cJSON_GetObjectItemCaseSensitive(node->data.advertisement, "respondsTo");
            client_topics = cote->options.requests;
        }
        if (NULL != client_topics) {
            if (NULL != server_topics) {
                for (int index_subscribesTo = 0; (index_subscribesTo < cJSON_GetArraySize(client_topics)) && (false == match); index_subscribesTo++) {
                    for (int index_broadcasts = 0; (index_broadcasts < cJSON_GetArraySize(server_topics)) && (false == match); index_broadcasts++) {
                        cJSON *tmp1 = cJSON_GetArrayItem(client_topics, index_subscribesTo);
                        char * str1 = cJSON_GetStringValue(tmp1);
                        cJSON *tmp2 = cJSON_GetArrayItem(server_topics, index_broadcasts);
                        char * str2 = cJSON_GetStringValue(tmp2);
                        if ((NULL != str1) && (NULL != str2)) {
                            regex_t regex;
                            if (0 == regcomp(&regex, str1, REG_NOSUB | REG_EXTENDED)) {
                                if (0 == regexec(&regex, str2, 0, NULL, 0)) {
                                    match = true;
                                }
                                regfree(&regex);
                            }
                        }
                    }
                }
            }
        } else {
            match = true;
        }
        if (false == match) {
            /* No match between broadcasts/respondsTo and subscribesTo/requests arrays, ignore this node */
            sem_post(&cote->options.sem);
            return;
        }

        /* Connect to the node */
        if (0 != axon_connect(cote->axon, (true == cote->options.use_hostname) ? node->hostname : node->address, port)) {
            /* Unable to connect */
            sem_post(&cote->options.sem);
            /* Invoke error callback if defined */
            if (NULL != cote->cb.error.fct) {
                cote->cb.error.fct(cote, "cote: unable to connect to new node", cote->cb.error.user);
            }
            return;
        }

        /* Release options semaphore */
        sem_post(&cote->options.sem);
    }

    /* Invoke added callback if defined */
    if (NULL != cote->cb.added.fct) {
        cote->cb.added.fct(cote, node, cote->cb.added.user);
    }
}

/**
 * @brief Callback function invoked when discovery node instance is removed
 * @param discover Discover instance
 * @param node Node
 * @param user User data
 */
static void
cote_discovery_removed_cb(discover_t *discover, discover_node_t *node, void *user) {

    (void)discover;
    assert(NULL != node);
    assert(NULL != user);

    /* Retrieve cote instance using user data */
    cote_t *cote = (cote_t *)user;

    /* Check node content */
    if (0 != cote_discovery_check_node(cote, node)) {
        /* Invalid node, ignore message */
        return;
    }

    /* Invoke removed callback if defined */
    if (NULL != cote->cb.removed.fct) {
        cote->cb.removed.fct(cote, node, cote->cb.removed.user);
    }
}

/**
 * @brief Callback function called to handle error from Axon instance
 * @param discover Discover instance
 * @param err Error as string
 * @param user User data
 */
static void
cote_discovery_error_cb(discover_t *discover, char *err, void *user) {

    (void)discover;
    assert(NULL != err);
    assert(NULL != user);

    /* Retrieve cote instance using user data */
    cote_t *cote = (cote_t *)user;

    /* Invoke error callback if defined */
    if (NULL != cote->cb.error.fct) {
        cote->cb.error.fct(cote, err, cote->cb.error.user);
    }
}

/**
 * @brief Function used to set Discovery instance advertisement
 * @param cote Cote instance
 * @return 0 if the function succeeded, -1 otherwise
 */
static int
cote_discovery_set_advertisement(cote_t *cote) {

    /* Wait options semaphore */
    sem_wait(&cote->options.sem);

    /* Definition of advertisement */
    cJSON *advertisement = NULL;
    if (NULL != cote->options.advertisement) {
        advertisement = cJSON_Duplicate(cote->options.advertisement, 1);
    } else {
        advertisement = cJSON_CreateObject();
    }
    if (NULL == advertisement) {
        /* Unable to allocate memory */
        sem_post(&cote->options.sem);
        return -1;
    }
    cJSON_AddStringToObject(advertisement, "type", (COTE_TYPE_MON == cote->type) ? "monitor" : "service");
    cJSON_AddStringToObject(advertisement, "name", cote->name);
    if (NULL != cote->options.namespace_) {
        cJSON_AddStringToObject(advertisement, "namespace", cote->options.namespace_);
    }
    if ((COTE_TYPE_PUB == cote->type) && (NULL != cote->options.broadcasts)) {
        cJSON *broadcasts = cJSON_Duplicate(cote->options.broadcasts, 1);
        if (NULL != broadcasts) {
            cJSON_AddItemToObject(advertisement, "broadcasts", broadcasts);
        }
    } else if ((COTE_TYPE_SUB == cote->type) && (NULL != cote->options.subscribesTo)) {
        cJSON *subscribesTo = cJSON_Duplicate(cote->options.subscribesTo, 1);
        if (NULL != subscribesTo) {
            cJSON_AddItemToObject(advertisement, "subscribesTo", subscribesTo);
        }
    } else if ((COTE_TYPE_REQ == cote->type) && (NULL != cote->options.requests)) {
        cJSON *requests = cJSON_Duplicate(cote->options.requests, 1);
        if (NULL != requests) {
            cJSON_AddItemToObject(advertisement, "requests", requests);
        }
    } else if ((COTE_TYPE_REP == cote->type) && (NULL != cote->options.respondsTo)) {
        cJSON *respondsTo = cJSON_Duplicate(cote->options.respondsTo, 1);
        if (NULL != respondsTo) {
            cJSON_AddItemToObject(advertisement, "respondsTo", respondsTo);
        }
    }
    cJSON_AddStringToObject(advertisement, "key", "$$");
    if (COTE_TYPE_PUB == cote->type) {
        cJSON_AddStringToObject(advertisement, "axon_type", "pub-emitter");
        cJSON_AddNumberToObject(advertisement, "port", cote->port);
    } else if (COTE_TYPE_SUB == cote->type) {
        cJSON_AddStringToObject(advertisement, "axon_type", "sub-emitter");
    } else if (COTE_TYPE_REQ == cote->type) {
        cJSON_AddStringToObject(advertisement, "axon_type", "req");
    } else if (COTE_TYPE_REP == cote->type) {
        cJSON_AddStringToObject(advertisement, "axon_type", "rep");
        cJSON_AddNumberToObject(advertisement, "port", cote->port);
    } else if (COTE_TYPE_MON == cote->type) {
        cJSON_AddNumberToObject(advertisement, "port", 0);
    }
    discover_advertise(cote->discover, advertisement);
    cJSON_Delete(advertisement);

    /* Release options semaphore */
    sem_post(&cote->options.sem);

    return 0;
}

/**
 * @brief Function used to check discovery callback node
 * @param cote Cote instance
 * @param node Node
 * @return 0 if the function succeeded (node is valid), -1 otherwise
 */
static int
cote_discovery_check_node(cote_t *cote, discover_node_t *node) {

    assert(NULL != cote);
    assert(NULL != node);

    /* Check advertisement content */
    if (NULL == node->data.advertisement) {
        /* No advertisement, ignore message */
        return -1;
    }

    /* Treatment depending of Cote instance type */
    if (COTE_TYPE_MON != cote->type) {

        /* Check advertisement content */
        cJSON *axon_type = cJSON_GetObjectItemCaseSensitive(node->data.advertisement, "axon_type");
        if (NULL == axon_type) {
            /* No axon type, ignore message */
            return -1;
        }
        if (((COTE_TYPE_PUB == cote->type) && (strcmp("sub-emitter", cJSON_GetStringValue(axon_type))))
            || ((COTE_TYPE_SUB == cote->type) && (strcmp("pub-emitter", cJSON_GetStringValue(axon_type))))
            || ((COTE_TYPE_REQ == cote->type) && (strcmp("rep", cJSON_GetStringValue(axon_type))))
            || ((COTE_TYPE_REP == cote->type) && (strcmp("req", cJSON_GetStringValue(axon_type))))) {
            /* Invalid axon type, ignore message */
            return -1;
        }
        cJSON *key = cJSON_GetObjectItemCaseSensitive(node->data.advertisement, "key");
        if (NULL == key) {
            /* No key, ignore message */
            return -1;
        }
        if (strcmp("$$", cJSON_GetStringValue(key))) {
            /* Invalid key, ignore message */
            return -1;
        }
        cJSON *namespace = cJSON_GetObjectItemCaseSensitive(node->data.advertisement, "namespace");
        sem_wait(&cote->options.sem);
        if (NULL != cote->options.namespace_) {
            if (NULL == namespace) {
                /* No namespace, ignore message */
                sem_post(&cote->options.sem);
                return -1;
            }
            if (strcmp(cote->options.namespace_, cJSON_GetStringValue(namespace))) {
                /* Invalid namespace, ignore message */
                sem_post(&cote->options.sem);
                return -1;
            }
        } else {
            if (NULL != namespace) {
                /* Namespace present but not expected, ignore message */
                sem_post(&cote->options.sem);
                return -1;
            }
        }
        sem_post(&cote->options.sem);
    }

    return 0;
}
