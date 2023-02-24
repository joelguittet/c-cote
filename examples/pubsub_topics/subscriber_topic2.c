/**
 * @file      subscriber_topic2.c
 * @brief     Cote Subscriber example in C
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
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <signal.h>
#include <cJSON.h>

#include "cote.h"

/******************************************************************************/
/* Variables                                                                  */
/******************************************************************************/

static bool terminate = false; /* Flag used to terminate the application */

/******************************************************************************/
/* Prototypes                                                                 */
/******************************************************************************/

/**
 * @brief Signal hanlder
 * @param signo Signal number
 */
static void sig_handler(int signo);

/**
 * @brief Callback function invoked when message is received
 * @param cote Cote instance
 * @param topic Topic of the message
 * @param amp AMP message
 * @param user User data
 * @return Always return NULL
 */
static amp_msg_t *callback(cote_t *cote, char *topic, amp_msg_t *amp, void *user);

/******************************************************************************/
/* Functions                                                                  */
/******************************************************************************/

/**
 * @brief Main function
 * @param argc Number of arguments
 * @param argv Arguments
 * @return Always returns 0
 */
int
main(int argc, char **argv) {

    cote_t *cote;

    /* Initialize sig handler */
    signal(SIGINT, sig_handler);

    /* Create Cote "pub" instance */
    if (NULL == (cote = cote_create("sub", "subscriber_topic2"))) {
        printf("unable to create cote instance\n");
        exit(EXIT_FAILURE);
    }

    /* Set cote options */
    cJSON *subscribesTo = cJSON_CreateArray();
    if (NULL == subscribesTo) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }
    cJSON *topic2 = cJSON_CreateString("topic2");
    if (NULL == topic2) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        cJSON_Delete(subscribesTo);
        exit(EXIT_FAILURE);
    }
    cJSON_AddItemToArray(subscribesTo, topic2);
    if (0 != cote_set_option(cote, "subscribesTo", subscribesTo)) {
        printf("unable to set cote subscribesTo option\n");
        cote_release(cote);
        cJSON_Delete(subscribesTo);
        exit(EXIT_FAILURE);
    }
    cJSON_Delete(subscribesTo);

    /* Start instance */
    if (0 != cote_start(cote)) {
        printf("unable to start cote instance\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }

    /* Subscribe to topic2 */
    cote_subscribe(cote, "topic2", &callback, NULL);

    printf("subscriber started\n");

    /* Wait before terminating the program */
    while (false == terminate) {
        sleep(1);
    }

    /* Release memory */
    cote_release(cote);

    return 0;
}

/**
 * @brief Signal hanlder
 * @param signo Signal number
 */
static void
sig_handler(int signo) {

    /* SIGINT handling */
    if (SIGINT == signo) {
        terminate = true;
    }
}

/**
 * @brief Callback function invoked when message is received
 * @param cote Cote instance
 * @param topic Topic of the message
 * @param amp AMP message
 * @param user User data
 * @return Always return NULL
 */
static amp_msg_t *
callback(cote_t *cote, char *topic, amp_msg_t *amp, void *user) {

    (void)cote;
    assert(NULL != topic);
    assert(NULL != amp);
    (void)user;

    int64_t bint;
    char *  str;

    printf("sub client message received from topic '%s'\n", topic);

    /* Parse all fields of the message */
    amp_field_t *field = amp_get_first(amp);
    while (NULL != field) {

        /* Switch depending of the type */
        switch (field->type) {
            case AMP_TYPE_BLOB:
                printf("<Buffer");
                for (int index_data = 0; index_data < field->size; index_data++) {
                    printf(" %02x", ((unsigned char *)field->data)[index_data]);
                }
                printf(">\n");
                break;
            case AMP_TYPE_STRING:
                printf("%s\n", (char *)field->data);
                break;
            case AMP_TYPE_BIGINT:
                bint = (*(int64_t *)field->data);
                printf("%" PRId64 "\n", bint);
                break;
            case AMP_TYPE_JSON:
                str = cJSON_PrintUnformatted((cJSON *)field->data);
                printf("%s\n", str);
                free(str);
                break;
            default:
                /* Should not occur */
                break;
        }

        /* Next field */
        field = amp_get_next(amp);
    }

    return NULL;
}
