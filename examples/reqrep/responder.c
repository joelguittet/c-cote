/**
 * @file responder.c
 * @brief Cote Responder example in C
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

static bool terminate = false;                                      /* Flag used to terminate the application */


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
int main(int argc, char** argv) {

  cote_t *cote;
  
  /* Initialize sig handler */
  signal(SIGINT, sig_handler);
  
  /* Create Cote "pub" instance */
  if (NULL == (cote = cote_create("rep", "responder"))) {
    printf("unable to create cote instance\n");
    exit(EXIT_FAILURE);
  }
  
  /* Set cote options */
  cJSON *respondsTo = cJSON_CreateArray();
  if (NULL == respondsTo) {
    printf("unable to set allocate memory\n");
    cote_release(cote);
    exit(EXIT_FAILURE);
  }
  cJSON *hello = cJSON_CreateString("hello");
  if (NULL == hello) {
    printf("unable to set allocate memory\n");
    cote_release(cote);
    cJSON_Delete(respondsTo);
    exit(EXIT_FAILURE);
  }
  cJSON_AddItemToArray(respondsTo, hello);
  if (0 != cote_set_option(cote, "respondsTo", respondsTo)) {
    printf("unable to set cote respondsTo option\n");
    cote_release(cote);
    cJSON_Delete(respondsTo);
    exit(EXIT_FAILURE);
  }
  cJSON_Delete(respondsTo);
  
  /* Start instance */
  if (0 != cote_start(cote)) {
    printf("unable to start cote instance\n");
    cote_release(cote);
    exit(EXIT_FAILURE);
  }
  
  /* Subscribe to hello */
  cote_subscribe(cote, "hello", &callback, NULL);

  printf("responder started\n");

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
static void sig_handler(int signo) {

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
static amp_msg_t *callback(cote_t *cote, char *topic, amp_msg_t *amp, void *user) {

  assert(NULL != cote);
  assert(NULL != topic);
  assert(NULL != amp);
  (void)user;

  int64_t bint; char *str;

  printf("rep client message received from topic '%s'\n", topic);

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

  printf("replying\n");

  /* Format reply */
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "goodbye", "world");
  amp_msg_t *reply = cote_reply(cote, 1, AMP_TYPE_JSON, json);
  cJSON_Delete(json);

  return reply;
}
