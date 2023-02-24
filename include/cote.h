/**
 * @file      cote.h
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

#ifndef __COTE_H__
#define __COTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

COTE_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
COTE_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
COTE_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the COTE_API_VISIBILITY flag to "export" the same symbols the way COTE_EXPORT_SYMBOLS does

*/

#define COTE_CDECL   __cdecl
#define COTE_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(COTE_HIDE_SYMBOLS) && !defined(COTE_IMPORT_SYMBOLS) && !defined(COTE_EXPORT_SYMBOLS)
#define COTE_EXPORT_SYMBOLS
#endif

#if defined(COTE_HIDE_SYMBOLS)
#define COTE_PUBLIC(type) type COTE_STDCALL
#elif defined(COTE_EXPORT_SYMBOLS)
#define COTE_PUBLIC(type) __declspec(dllexport) type COTE_STDCALL
#elif defined(COTE_IMPORT_SYMBOLS)
#define COTE_PUBLIC(type) __declspec(dllimport) type COTE_STDCALL
#endif
#else /* !__WINDOWS__ */
#define COTE_CDECL
#define COTE_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__SUNPRO_C)) && defined(COTE_API_VISIBILITY)
#define COTE_PUBLIC(type) __attribute__((visibility("default"))) type
#else
#define COTE_PUBLIC(type) type
#endif
#endif

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include <stdint.h>
#include <semaphore.h>
#include <cJSON.h>

#include "discover.h"
#include "axon.h"

/******************************************************************************/
/* Definitions                                                                */
/******************************************************************************/

/* Cote type */
typedef enum {
    COTE_TYPE_PUB, /* Publisher (server which is broadcasting data to all its clients) */
    COTE_TYPE_SUB, /* Subscriber (client receiving broadcasted data from the Publisher server) */
    COTE_TYPE_REQ, /* Requester (client sending requests and receiving responses from the Replier server - with Round-Robin mechanism and queing) */
    COTE_TYPE_REP, /* Replier (server waiting for message from clients and replying to the client) */
    COTE_TYPE_MON  /* Monitor (discover configuration only) */
} cote_enum_e;

/* Cote topic subscription */
struct cote_s;
typedef struct cote_sub_s {
    struct cote_sub_s *next;                                         /* Next subscription */
    char *             topic;                                        /* Topic of the subscription */
    amp_msg_t *(*fct)(struct cote_s *, char *, amp_msg_t *, void *); /* Callback function invoked when topic is received */
    void *user;                                                      /* User data passed to the callback */
} cote_sub_t;

/* Cote instance */
typedef struct cote_s {
    cote_enum_e type; /* Cote instance type */
    char *      name; /* Name of the instance */
    uint16_t    port; /* Port of axon instance */
    struct {
        char * namespace_;    /* Namespace used to format message topics */
        bool   use_hostname;  /* Use hostname instead of address to connect to the other nodes */
        cJSON *advertisement; /* The initial advertisement which is sent with each hello packet */
        cJSON *broadcasts;    /* Publisher broadcast string array */
        cJSON *subscribesTo;  /* Subscriber subscribe string array */
        cJSON *requests;      /* Requester request string array */
        cJSON *respondsTo;    /* Replier respond string array */
        sem_t  sem;           /* Semaphore used to protect options */
    } options;
    discover_t *discover; /* Discover instance */
    axon_t *    axon;     /* Axon instance */
    struct {
        cote_sub_t *first; /* Topic subscription daisy chain */
        sem_t       sem;   /* Semaphore used to protect daisy chain */
    } subs;
    struct {
        struct {
            amp_msg_t *(*fct)(struct cote_s *, amp_msg_t *, void *); /* Callback function invoked when message is received */
            void *user;                                              /* User data passed to the callback */
        } message;
        struct {
            void *(*fct)(struct cote_s *, discover_node_t *, void *); /* Callback function invoked when a node is added */
            void *user;                                               /* User data passed to the callback */
        } added;
        struct {
            void *(*fct)(struct cote_s *, discover_node_t *, void *); /* Callback function invoked when a node is removed */
            void *user;                                               /* User data passed to the callback */
        } removed;
        struct {
            void *(*fct)(struct cote_s *, char *, void *); /* Callback function invoked when an error occurs */
            void *user;                                    /* User data passed to the callback */
        } error;
    } cb;
} cote_t;

/******************************************************************************/
/* Prototypes                                                                 */
/******************************************************************************/

/**
 * @brief Function used to create cote instance
 * @param type Type of Cote instance
 * @param name Name of Cote instance
 * @return Cote instance if the function succeeded, NULL otherwise
 */
COTE_PUBLIC(cote_t *) cote_create(char *type, char *name);

/**
 * @brief Set cote options
 * @param cote Cote instance
 * @param option Option by name
 * @param value New value of the option
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_set_option(cote_t *cote, char *option, void *value);

/**
 * @brief Start Cote instance
 * @param cote Cote instance
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_start(cote_t *cote);

/**
 * @brief Function used to set advertisement
 * @param cote Cote instance
 * @param advertisement Advestisement object
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_advertise(cote_t *cote, cJSON *advertisement);

/**
 * @brief Register callbacks
 * @param cote Cote instance
 * @param topic Topic
 * @param fct Callback funtion
 * @param user User data
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_on(cote_t *cote, char *topic, void *fct, void *user);

/**
 * @brief Subscribe to wanted topic
 * @param cote Cote instance
 * @param topic Topic
 * @param fct Callback funtion
 * @param user User data
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_subscribe(cote_t *cote, char *topic, void *fct, void *user);

/**
 * @brief Unubscribe to wanted topic
 * @param cote Cote instance
 * @param topic Topic
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_unsubscribe(cote_t *cote, char *topic);

/**
 * @brief Function used to send data to all connected subscribers
 * @param cote Cote instance
 * @param topic Topic of the message
 * @param count Amount of data to be sent
 * @param ... type, data Array of type and data (and size for blob type) to be sent, AMP response message and timeout for Requester instance
 * @return 0 if the function succeeded, -1 otherwise
 */
COTE_PUBLIC(int) cote_send(cote_t *cote, char *topic, int count, ...);

/**
 * @brief Function used by Replier instance to format response to the server or to a single client
 * @param cote Cote instance
 * @param count Amount of data to be sent
 * @param ... type, data Array of type and data (and size for blob type) to be sent
 * @return AMP response message
 */
#define cote_reply(cote, ...) axon_reply(cote->axon, ##__VA_ARGS__)

/**
 * @brief Release cote instance
 * @param cote Cote instance
 */
COTE_PUBLIC(void) cote_release(cote_t *cote);

#ifdef __cplusplus
}
#endif

#endif /* __COTE_H__ */
