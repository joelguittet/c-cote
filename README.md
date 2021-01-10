# c-cote

[![CMake Badge](https://github.com/joelguittet/c-cote/workflows/CMake%20+%20SonarCloud%20Analysis/badge.svg)](https://github.com/joelguittet/c-cote/actions)
[![Issues Badge](https://img.shields.io/github/issues/joelguittet/c-cote)](https://github.com/joelguittet/c-cote/issues)
[![License Badge](https://img.shields.io/github/license/joelguittet/c-cote)](https://github.com/joelguittet/c-cote/blob/master/LICENSE)

[![Bugs](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=bugs)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)
[![Code Smells](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=code_smells)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)
[![Duplicated Lines (%)](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=duplicated_lines_density)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)
[![Lines of Code](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=ncloc)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)
[![Vulnerabilities](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=vulnerabilities)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)

[![Maintainability Rating](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=sqale_rating)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)
[![Reliability Rating](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=reliability_rating)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)
[![Security Rating](https://sonarcloud.io/api/project_badges/measure?project=joelguittet_c-cote&metric=security_rating)](https://sonarcloud.io/dashboard?id=joelguittet_c-cote)

Zero-configuration microservices library in C. Built in support for a variable number of master processes, service advertising and channel messaging.

This repository is not a fork of [cote](https://github.com/dashersw/cote) ! It has the same behavior but it is a complete library written in C in order to be portable and used in various applications. The goal of this library is to be fully compatible with cote Node.js version and permit discovery, monitoring and communications between applications written in C and Javascript.

## Features

*   zero-dependency: microservices fully written in C
*   zero-configuration: no IP address, no ports, no routing to configure
*   decentralized: no fixed parts, no "manager" nodes, no single point of failure
*   auto-discovery: services discover each other without a central bookkeeper
*   fault-tolerant: don't lose any requests when a service is down (a buffer is used and requests are dropped anyway if impossible to send)
*   scalable: horizontally scale to any number of machines
*   performant: process thousands of messages per second
*   humanized API: extremely simple to get started with a reasonable API!

## Building

Build `libcote.so` with the following commands:

``` bash
cmake .
make
```

## Protocol

Cote uses the extremely simple AMP protocol to send messages on the wire.

The `libcote.so` library depends of `libdiscover.so` from [c-discover](https://github.com/joelguittet/c-discover), `libaxon.so` from [c-axon](https://github.com/joelguittet/c-axon) and `libamp.so` from [c-amp](https://github.com/joelguittet/c-amp).

## Compatibility

This library is compatible with [cote](https://github.com/dashersw/cote) release 1.0.0.

## Examples

Several examples are available in the `examples\` directory.

Build examples with the following commands:
``` bash
cmake -DENABLE_COTE_EXAMPLES=ON .
make
```

### Pub / Sub

Pub instances distribute messages to all connected Sub clients using a broadcast mechanism. Messages are sent each time to all available sockets. Pub instances have no queing mechanism.

Sub instances receives messages from Pub servers.

Sub instances may optionally subscribe to one or more "topics" (the first multipart value), using string patterns or regular expressions.
 
### Req / Rep

Req instances distribute messages to connected Rep servers using a Round-Robin mechanism. Messages are sent each time to the next available socket. Req instances have also a queing mechanism to handle loss of connections and send them upton the next connection. A timeout is specified to wait the reply.

Rep instances receives messages from Req clients and reply (or not).

### Monitor

Provide a simple application to show the nodes currently discovered.

## Performances

Performances have not been evaluated yet.

## What's it good for?

This goal of this library is to provide a C implementation of the original cote Node.js version. This allow a communication between processes written using different languages.

## API

### cote_t *cote_create(char *type, char *name)

Create a new cote instance of type `type` which is "pub", "sub", "req", "rep" or "mon". The name of the instance is `name`and is visible in the Monitor example application.

### int cote_set_option(cote_t *cote, char *option, void *value)

Set cote instance `option` to the wanted `value` which is passed by address. The following table shows the available options and their default value. Options must be set before starting the instance.

| Option          | Type          | Default              |
|-----------------|---------------|----------------------|
| helloInterval   | int           | 2000ms               |
| checkInterval   | int           | 4000ms               |
| nodeTimeout     | int           | 5000ms               |
| masterTimeout   | int           | 6000ms               |
| address         | char *        | "0.0.0.0"            |
| port            | uint16_t      | 12345                |
| broadcast       | char *        | "255.255.255.255"    |
| multicast       | char *        | NULL                 |
| multicastTTL    | unsigned char | 1                    |
| unicast         | char *        | NULL                 |
| key             | char *        | NULL                 |
| mastersRequired | int           | 1                    |
| weight          | double        | Computed on startup  |
| client          | bool          | false                |
| reuseAddr       | bool          | true                 |
| ignoreProcess   | bool          | false                |
| ignoreInstance  | bool          | false                |
| advertisement   | cJSON *       | NULL                 |
| hostname        | char *        | Retrieved on startup |
| namespace       | char *        | NULL                 |
| useHostNames    | bool          | false                |
| advertisement   | cJSON *       | NULL                 |
| broadcasts      | cJSON *       | NULL                 |
| subscribesTo    | cJSON *       | NULL                 |
| requests        | cJSON *       | NULL                 |
| respondsTo      | cJSON *       | NULL                 |

| :exclamation: No key available today. Encryption of data is not available. First because I have not found any simple and satisfying library to do it, and then because the Cipher initialization used in discover Node.js version is currently deprecated. |
|-|

### int cote_start(cote_t *cote)

Start the cote instance.

### int cote_advertise(cote_t *cote, cJSON *advertisement)

Set `advertisement` object. Can be used after starting the instance to update the advertisement content.

### int cote_on(cote_t *cote, char *topic, void *fct, void *user)

Register a callback `fct` on the event `topic`. An optionnal `user` argument is available. The following table shows the available topics and their callback prototype.

| Topic   | Callback                                                 | Description                        |
|---------|----------------------------------------------------------|------------------------------------|
| added   | void *(*fct)(struct cote_s *, discover_node_t *, void *) | Called when a node is discovered   |
| removed | void *(*fct)(struct cote_s *, discover_node_t *, void *) | Called when a node has disappeared |
| message | amp_msg_t *(*fct)(struct cote_s *, amp_msg_t *, void *)  | Called when a message is received  |
| error   | void *(*fct)(struct cote_s *, char *, void *)            | Called when an error occured       |

### int cote_subscribe(cote_t *cote, char *topic, void *fct, void *user)

Subscribe a callback `fct` on the `topic`. An optionnal `user` argument is available. Can be called to update a subscription.

### int cote_unsubscribe(cote_t *cote, char *topic)

Unsubscribe to the `topic`.

### int cote_send(cote_t *cote, char *topic, int count, ...)

Send data. The `count` value indicate the amount of fields. `...` expects the fields with type and value for each of them. Requester should terminate the list of argument by an `amp_msg_t **` to receive the response from the Replier and a timeout `int` value.

### amp_msg_t *cote_reply(cote_t *cote, int count, ...)

Format a new reply (Replier instances only). The `count` value indicate the amount of fields. `...` expects the fields with type and value for each of them.

### void cote_release(cote_t *cote)

Release internal memory and stop cote instance. All sockets are closed. Must be called to free ressources.

## License

MIT
