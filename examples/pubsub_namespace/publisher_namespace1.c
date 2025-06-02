/**
 * @file      publisher_namespace1.c
 * @brief     Cote Publisher example in C
 *
 * MIT License
 *
 * Copyright joelguittet and c-cote contributors
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
    if (NULL == (cote = cote_create("pub", "publisher_namespace1"))) {
        printf("unable to create cote instance\n");
        exit(EXIT_FAILURE);
    }

    /* Set cote options */
    char *namespace1 = "namespace1";
    if (0 != cote_set_option(cote, "namespace", (void *)namespace1)) {
        printf("unable to set cote namespace option\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }
    cJSON *broadcasts = cJSON_CreateArray();
    if (NULL == broadcasts) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        exit(EXIT_FAILURE);
    }
    cJSON *hello = cJSON_CreateString("hello");
    if (NULL == hello) {
        printf("unable to set allocate memory\n");
        cote_release(cote);
        cJSON_Delete(broadcasts);
        exit(EXIT_FAILURE);
    }
    cJSON_AddItemToArray(broadcasts, hello);
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

        /* Sending blob */
        unsigned char tmp[3] = { 1, 2, 3 };
        cote_send(cote, "hello", 1, AMP_TYPE_BLOB, tmp, 3);

        /* Sending string */
        cote_send(cote, "hello", 1, AMP_TYPE_STRING, "hello");

        /* Sending bigint */
        cote_send(cote, "hello", 1, AMP_TYPE_BIGINT, 123451234512345);

        /* Sending JSON object */
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "payload", "hello world!");
        cote_send(cote, "hello", 1, AMP_TYPE_JSON, json);
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
