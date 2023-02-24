/**
 * @file      publisher_topic1_topic2.c
 * @brief     Cote Publisher example in C
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
    if (NULL == (cote = cote_create("pub", "publisher_topic1_topic2"))) {
        printf("unable to create cote instance\n");
        exit(EXIT_FAILURE);
    }

    /* Set cote options */
    cJSON *broadcasts = cJSON_CreateArray();
    if (NULL == broadcasts) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }
    cJSON *topic1 = cJSON_CreateString("topic1");
    if (NULL == topic1) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        cJSON_Delete(broadcasts);
        exit(EXIT_FAILURE);
    }
    cJSON_AddItemToArray(broadcasts, topic1);
    cJSON *topic2 = cJSON_CreateString("topic2");
    if (NULL == topic2) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        cJSON_Delete(broadcasts);
        exit(EXIT_FAILURE);
    }
    cJSON_AddItemToArray(broadcasts, topic2);
    if (0 != cote_set_option(cote, "broadcasts", broadcasts)) {
        printf("unable to set cote options\n");
        cJSON_Delete(broadcasts);
        cote_release(cote);
        exit(EXIT_FAILURE);
    }
    cJSON_Delete(broadcasts);

    /* Start instance */
    if (0 != cote_start(cote)) {
        printf("unable to start cote instance\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }

    printf("publisher started\n");

    /* Loop */
    while (false == terminate) {

        printf("sending\n");

        /* Sending JSON object topic 1 */
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "payload", "the payload of topic 1");
        cote_send(cote, "topic1", 1, AMP_TYPE_JSON, json);
        cJSON_Delete(json);

        /* Sending JSON object topic 2 */
        json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "payload", "the payload of topic 2");
        cote_send(cote, "topic2", 1, AMP_TYPE_JSON, json);
        cJSON_Delete(json);

        /* Wait for a while */
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
