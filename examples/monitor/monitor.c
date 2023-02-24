/**
 * @file      monitor.c
 * @brief     Cote Monitor example in C
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
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <cJSON.h>

#include "cote.h"

/******************************************************************************/
/* Variables                                                                  */
/******************************************************************************/

static sem_t sem;               /* Semaphore used to not overlap or other bad effect on the screen */
static bool  terminate = false; /* Flag used to terminate the application */

/******************************************************************************/
/* Prototypes                                                                 */
/******************************************************************************/

/**
 * @brief Signal hanlder
 * @param signo Signal number
 */
static void sig_handler(int signo);

/**
 * @brief Callback function invoked when node is added
 * @param cote Cote instance
 * @param node Node
 * @param user User data
 */
static void added_callback(cote_t *cote, discover_node_t *node, void *user);

/**
 * @brief Callback function invoked when node is removed
 * @param cote Cote instance
 * @param node Node
 * @param user User data
 */
static void removed_callback(cote_t *cote, discover_node_t *node, void *user);

/**
 * @brief Display the list of nodes
 * @param cote Cote instance
 */
static void display_nodes(cote_t *cote);

/**
 * @brief Format a string to be displayed
 * @param input Input string to be formated
 * @param size Expected output size
 * @return Output string
 */
static char *format_string(char *input, size_t size);

/**
 * @brief Format an integer to be displayed
 * @param input Input integer to be formated
 * @param size Expected output size
 * @return Output string
 */
static char *format_integer(int input, size_t size);

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

    /* Initialize semaphore used to access the screen */
    sem_init(&sem, 0, 1);

    /* Create Cote "mon" instance */
    if (NULL == (cote = cote_create("mon", "monitor"))) {
        printf("unable to create cote instance\n");
        exit(EXIT_FAILURE);
    }

    /* Start instance */
    if (0 != cote_start(cote)) {
        printf("unable to start cote instance\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }

    /* Definition of message callback */
    cote_on(cote, "added", &added_callback, NULL);
    cote_on(cote, "removed", &removed_callback, NULL);

    printf("monitor started\n");

    /* Wait before terminating the program */
    while (false == terminate) {
        sleep(1);
    }

    /* Release memory */
    cote_release(cote);

    /* Release semaphore */
    sem_close(&sem);

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
 * @brief Callback function invoked when node is added
 * @param cote Cote instance
 * @param node Node
 * @param user User data
 */
static void
added_callback(cote_t *cote, discover_node_t *node, void *user) {

    assert(NULL != cote);
    (void)node;
    (void)user;

    /* Display nodes */
    display_nodes(cote);
}

/**
 * @brief Callback function invoked when node is removed
 * @param cote Cote instance
 * @param node Node
 * @param user User data
 */
static void
removed_callback(cote_t *cote, discover_node_t *node, void *user) {

    assert(NULL != cote);
    (void)node;
    (void)user;

    /* Display nodes */
    display_nodes(cote);
}

/**
 * @brief Display the list of nodes
 * @param cote Cote instance
 */
static void
display_nodes(cote_t *cote) {

    /* Wait semaphore used to not overlap or other bad effect on the screen */
    sem_wait(&sem);

    /* Clear screen */
    system("clear");

    /* Print title */
    char *name    = format_string("Name", 20);
    char *iid     = format_string("Instance ID", 40);
    char *address = format_string("Address", 18);
    char *port    = format_string("Port", 5);
    printf("\033[32m%s %s %s %s\033[0m\n", name, iid, address, port);
    free(name);
    name = NULL;
    free(iid);
    iid = NULL;
    free(address);
    address = NULL;
    free(port);
    port = NULL;

    /* Parse nodes of the discover structure iself (a simple way to not invent a new wheel here but not the recommend way because internals of the library may change ! */
    discover_node_t *tmp = cote->discover->nodes.first;
    while (NULL != tmp) {

        /* Retrieve and format Name */
        cJSON *name_json = NULL;
        if (NULL != tmp->data.advertisement) {
            name_json = cJSON_GetObjectItemCaseSensitive(tmp->data.advertisement, "name");
            if (NULL != name_json) {
                name = cJSON_GetStringValue(name_json);
            }
        }
        name = format_string(name, 20);

        /* Retrieve and format Instance ID */
        iid = format_string(tmp->iid, 40);

        /* Retrieve and format Address */
        address = format_string(tmp->address, 18);

        /* Retrieve and format Port */
        cJSON *port_json   = NULL;
        int    port_number = 0;
        if (NULL != tmp->data.advertisement) {
            port_json = cJSON_GetObjectItemCaseSensitive(tmp->data.advertisement, "port");
            if (NULL != port_json) {
                port_number = (int)cJSON_GetNumberValue(port_json);
            }
        }
        port = (0 != port_number) ? format_integer(port_number, 5) : format_string(NULL, 5);

        /* Print node data */
        printf("\033[36m%s\033[0m \033[35m%s\033[0m \033[33m%s\033[0m \033[31m%s\033[0m\n", name, iid, address, port);

        /* Release memory */
        free(name);
        name = NULL;
        free(iid);
        iid = NULL;
        free(address);
        address = NULL;
        free(port);
        port = NULL;

        /* Next node */
        tmp = tmp->next;
    }

    /* Release semaphore */
    sem_post(&sem);
}

/**
 * @brief Format a string to be displayed
 * @param input Input string to be formated
 * @param size Expected output size
 * @return Output string
 */
static char *
format_string(char *input, size_t size) {

    /* Allocate output memory */
    unsigned char *output = (unsigned char *)malloc(size + 1);
    if (NULL == output) {
        /* Unable to allocate memory */
        return NULL;
    }
    memset(output, 0, size + 1);

    /* Set output */
    if (NULL != input) {
        if (size < strlen(input)) {
            memcpy(output, input, size - 3);
            strcat(output, "...");
        } else {
            strcpy(output, input);
            while (size > strlen(output)) {
                strcat(output, " ");
            }
        }
    } else {
        strcat(output, "-");
        while (size > strlen(output)) {
            strcat(output, " ");
        }
    }

    return output;
}

/**
 * @brief Format an integer to be displayed
 * @param input Input integer to be formated
 * @param size Expected output size
 * @return Output string
 */
static char *
format_integer(int input, size_t size) {

    /* Allocate output memory */
    unsigned char *output = (unsigned char *)malloc(size + 1);
    if (NULL == output) {
        /* Unable to allocate memory */
        return NULL;
    }
    memset(output, 0, size + 1);

    /* Set output */
    snprintf(output, size + 1, "%d", input);
    while (size > strlen(output)) {
        strcat(output, " ");
    }

    return output;
}
