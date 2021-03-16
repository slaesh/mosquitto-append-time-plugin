/*
Copyright (c) 2020 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mosquitto.h"
#include "mosquitto_broker.h"
#include "mosquitto_plugin.h"
#include "mqtt_protocol.h"

#define JSON_TIME_KEY "\"__t\""

static mosquitto_plugin_id_t *mosq_pid = NULL;

size_t findLastBracket(char *payload) {
   size_t idxOfEndBracket = 0;
   size_t payloadlen = strlen(payload);
   char *pp = payload + payloadlen - 1;

   for (size_t idx = payloadlen - 1; idx >= 0; --idx, --pp) {
      if (*pp == ' ' || *pp == '\n' || *pp == '\r') continue;

      if (*pp == '}') {
         idxOfEndBracket = idx;
      }

      break;
   }

   return idxOfEndBracket;
}

bool isJsonPayload(char *payload) {
   char *pp = payload;
   bool msgStartsWithBracket = false;

   size_t payloadlen = strlen(payload);
   if (payloadlen < 2) return false;

   for (size_t idx = 0; idx < payloadlen; ++idx, ++pp) {
      if (*pp == ' ' || *pp == '\n' || *pp == '\r') continue;

      msgStartsWithBracket = (*pp == '{');
      break;
   }

   return msgStartsWithBracket && findLastBracket(payload) > 0;
}

void *appendTime(char *payload) {
   const bool payloadIncludesTime = strstr(payload, JSON_TIME_KEY ":") != NULL;
   if (payloadIncludesTime) return payload;

   struct timespec ts;
   struct tm *ti;
   clock_gettime(CLOCK_REALTIME, &ts);
   ti = gmtime(&ts.tv_sec);
   int ms = (int)round(ts.tv_nsec / 1.0e6);  // convert nanoseconds to milliseconds

   // calculate new length
   size_t payloadlen = strlen(payload);
   size_t newPayloadlen = payloadlen + strlen("," JSON_TIME_KEY ":'2020-11-28T12:44:32.555Z' }") + 2 /* spare */;

   // alloc new payload!
   char *newPayload = mosquitto_calloc(1, newPayloadlen);

   if (newPayload == NULL) {
      return payload;
   }

   // copy old payload! except the last bracket.. ;)
   char *pp = newPayload;
   snprintf(pp, newPayloadlen, "%s", payload);

   size_t idxOfEndBracket = findLastBracket(payload);
   pp += idxOfEndBracket;

   char commaOrSpace = ',';

   // minimal valid json length, with at least one prop: {"_":1} => 7 chars
   if (payloadlen < 7 || strstr(payload, ":") == NULL) {
      commaOrSpace = ' ';
   }

   char time_buf[20];
   strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", ti);

   // append timestamp
   snprintf(pp, newPayloadlen - idxOfEndBracket,
            "%c" JSON_TIME_KEY ":\"%s.%03dZ\" }",
            commaOrSpace,
            time_buf,
            ms);

   return newPayload;
}

static int callback_message(int event, void *event_data, void *userdata) {
   struct mosquitto_evt_message *ed = event_data;

   if (ed->payload == NULL) {
      mosquitto_log_printf(MOSQ_LOG_DEBUG, "payload is NULL!");

      return MOSQ_ERR_SUCCESS;
   }

   if (ed->payloadlen <= 0) {
      mosquitto_log_printf(MOSQ_LOG_DEBUG, "payloadlen is 0!");

      return MOSQ_ERR_SUCCESS;
   }

   if (!isJsonPayload(ed->payload)) {
      mosquitto_log_printf(MOSQ_LOG_DEBUG, "payload is no json content!");

      return MOSQ_ERR_SUCCESS;
   }

   char *newPayload = appendTime(ed->payload);

   if (ed->payload != newPayload) {
      ed->payload = newPayload;
      ed->payloadlen = (uint32_t)strlen(newPayload);
   }

   return MOSQ_ERR_SUCCESS;
}

int mosquitto_plugin_version(int supported_version_count, const int *supported_versions) {
   int i;

   for (i = 0; i < supported_version_count; i++) {
      if (supported_versions[i] == 5) {
         return 5;
      }
   }
   return -1;
}

int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data, struct mosquitto_opt *opts, int opt_count) {
   mosq_pid = identifier;
   return mosquitto_callback_register(mosq_pid, MOSQ_EVT_MESSAGE, callback_message, NULL, NULL);
}

int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *opts, int opt_count) {
   return mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_MESSAGE, callback_message, NULL);
}
