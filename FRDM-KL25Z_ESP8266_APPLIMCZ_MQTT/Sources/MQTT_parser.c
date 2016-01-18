#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "MQTT_errors.h"
#include "MQTT_message.h"
#include "MQTT_serialiser.h"
#include "MQTT_parser.h"

#define READ_STRING(into) { \
    if ((len - *nread) < 2) { \
      return MQTT_PARSER_RC_INCOMPLETE; \
    } \
    \
    int str_length = data[*nread] * 256 + data[*nread + 1]; \
    \
    if ((len - *nread - 2) < str_length) { \
      return MQTT_PARSER_RC_INCOMPLETE; \
    } \
    \
    if (parser->buffer_pending == 0) { \
      parser->buffer_length = str_length; \
      \
      return MQTT_PARSER_RC_WANT_MEMORY; \
    } \
    \
    parser->buffer_pending = 0; \
    \
    if (parser->buffer != NULL) { \
      memcpy(parser->buffer, data + *nread + 2, fmin(str_length, parser->buffer_length)); \
      \
      into.length = fmin(str_length, parser->buffer_length); \
      into.data = parser->buffer; \
      \
      parser->buffer = NULL; \
      parser->buffer_length = 0; \
    } \
    \
    *nread += 2 + str_length; \
}

void mqtt_parser_init(mqtt_parser_t* parser) {
  parser->state = MQTT_PARSER_STATE_INITIAL;
  parser->buffer_pending = 0;
  parser->buffer = NULL;
  parser->buffer_length = 0;
}

void mqtt_parser_buffer(mqtt_parser_t* parser, uint8_t* buffer, size_t buffer_length) {
  parser->buffer_pending = 1;
  parser->buffer = buffer;
  parser->buffer_length = buffer_length;
}

mqtt_parser_rc_t mqtt_parser_execute(mqtt_parser_t* parser, mqtt_message_t* message, uint8_t* data, size_t len, size_t* nread) {
  do {
    switch (parser->state) {
      case MQTT_PARSER_STATE_INITIAL: {
        if ((len - *nread) < 1) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->common.retain = (data[*nread + 0] >> 0) & 0x01;
        message->common.qos    = (data[*nread + 0] >> 1) & 0x03;
        message->common.dup    = (data[*nread + 0] >> 3) & 0x01;
        message->common.type   = (data[*nread + 0] >> 4) & 0x0f;

        *nread += 1;

        parser->state = MQTT_PARSER_STATE_REMAINING_LENGTH;

        break;
      }

      case MQTT_PARSER_STATE_REMAINING_LENGTH: {
        int digit_bytes = 0,
            multiplier = 1,
            remaining_length = 0;

        do {
          digit_bytes += 1;

          if ((len - *nread) < digit_bytes) {
            return MQTT_PARSER_RC_INCOMPLETE;
          }

          remaining_length += (data[*nread + digit_bytes - 1] & 0x7f) * multiplier;
          multiplier *= 128;
        } while (data[*nread + digit_bytes - 1] >= 0x80 && digit_bytes < 4);

        if (data[*nread + digit_bytes - 1] >= 0x80) {
          parser->error = MQTT_ERROR_PARSER_INVALID_REMAINING_LENGTH;

          return MQTT_PARSER_RC_ERROR;
        }

        message->common.length = remaining_length;

        *nread += digit_bytes;

        switch (message->common.type) {
          case MQTT_TYPE_CONNECT: {
            parser->state = MQTT_PARSER_STATE_CONNECT;
            break;
          }
          case MQTT_TYPE_CONNACK: {
            parser->state = MQTT_PARSER_STATE_CONNACK;
            break;
          }
          case MQTT_TYPE_PUBLISH: {
            parser->state = MQTT_PARSER_STATE_PUBACK;
            break;
          }
          case MQTT_TYPE_PUBREC: {
            parser->state = MQTT_PARSER_STATE_PUBREC;
            break;
          }
          case MQTT_TYPE_PUBREL: {
            parser->state = MQTT_PARSER_STATE_PUBREL;
            break;
          }
          case MQTT_TYPE_PUBCOMP: {
            parser->state = MQTT_PARSER_STATE_PUBCOMP;
            break;
          }
          default: {
            parser->error = MQTT_ERROR_PARSER_INVALID_MESSAGE_ID;
            return MQTT_PARSER_RC_ERROR;
          }
        }

        break;
      }

      case MQTT_PARSER_STATE_VARIABLE_HEADER: {
        if (message->common.type == MQTT_TYPE_CONNECT) {
          parser->state = MQTT_PARSER_STATE_CONNECT_PROTOCOL_NAME;
        }

        break;
      }

      case MQTT_PARSER_STATE_CONNECT: {
        parser->state = MQTT_PARSER_STATE_CONNECT_PROTOCOL_NAME;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_PROTOCOL_NAME: {
        READ_STRING(message->connect.protocol_name)

        parser->state = MQTT_PARSER_STATE_CONNECT_PROTOCOL_VERSION;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_PROTOCOL_VERSION: {
        if ((len - *nread) < 1) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->connect.protocol_version = data[*nread];

        *nread += 1;

        parser->state = MQTT_PARSER_STATE_CONNECT_FLAGS;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_FLAGS: {
        if ((len - *nread) < 1) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->connect.flags.username_follows = (data[*nread] >> 7) & 0x01;
        message->connect.flags.password_follows = (data[*nread] >> 6) & 0x01;
        message->connect.flags.will_retain      = (data[*nread] >> 5) & 0x01;
        message->connect.flags.will_qos         = (data[*nread] >> 4) & 0x02;
        message->connect.flags.will             = (data[*nread] >> 2) & 0x01;
        message->connect.flags.clean_session    = (data[*nread] >> 1) & 0x01;

        *nread += 1;

        parser->state = MQTT_PARSER_STATE_CONNECT_KEEP_ALIVE;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_KEEP_ALIVE: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->connect.keep_alive = (data[*nread] << 8) + data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_CONNECT_CLIENT_IDENTIFIER;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_CLIENT_IDENTIFIER: {
        READ_STRING(message->connect.client_id)

        parser->state = MQTT_PARSER_STATE_CONNECT_WILL_TOPIC;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_WILL_TOPIC: {
        if (message->connect.flags.will) {
          READ_STRING(message->connect.will_topic)
        }

        parser->state = MQTT_PARSER_STATE_CONNECT_WILL_MESSAGE;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_WILL_MESSAGE: {
        if (message->connect.flags.will) {
          READ_STRING(message->connect.will_message)
        }

        parser->state = MQTT_PARSER_STATE_CONNECT_USERNAME;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_USERNAME: {
        if (message->connect.flags.username_follows) {
          READ_STRING(message->connect.username)
        }

        parser->state = MQTT_PARSER_STATE_CONNECT_PASSWORD;

        break;
      }

      case MQTT_PARSER_STATE_CONNECT_PASSWORD: {
        if (message->connect.flags.password_follows) {
          READ_STRING(message->connect.password)
        }

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      case MQTT_PARSER_STATE_CONNACK: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->connack._unused     = data[*nread];
        message->connack.return_code = data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      case MQTT_PARSER_STATE_PUBLISH: {
        parser->state = MQTT_PARSER_STATE_PUBLISH_TOPIC_NAME;

        break;
      }

      case MQTT_PARSER_STATE_PUBLISH_TOPIC_NAME: {
        READ_STRING(message->publish.topic_name)

        parser->state = MQTT_PARSER_STATE_PUBLISH_MESSAGE_ID;

        break;
      }

      case MQTT_PARSER_STATE_PUBLISH_MESSAGE_ID: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->publish.message_id = (data[*nread] << 8) + data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      case MQTT_PARSER_STATE_PUBACK: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->puback.message_id = (data[*nread] << 8) + data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      case MQTT_PARSER_STATE_PUBREC: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->pubrec.message_id = (data[*nread] << 8) + data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      case MQTT_PARSER_STATE_PUBREL: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->pubrel.message_id = (data[*nread] << 8) + data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      case MQTT_PARSER_STATE_PUBCOMP: {
        if ((len - *nread) < 2) {
          return MQTT_PARSER_RC_INCOMPLETE;
        }

        message->pubcomp.message_id = (data[*nread] << 8) + data[*nread + 1];

        *nread += 2;

        parser->state = MQTT_PARSER_STATE_INITIAL;

        return MQTT_PARSER_RC_DONE;
      }

      default: {
        parser->error = MQTT_ERROR_PARSER_INVALID_STATE;

        return MQTT_PARSER_RC_ERROR;
      }
    }
  } while (1);
}

/***************************************************************************
              Test du protocole MQTT.
    Entree : packet 12bits et n nombre de repetition

/***************************************************************************/
void MQTT_test(const CLS1_StdIOType *io) {

mqtt_parser_t parser;
mqtt_serialiser_t serialiser;
mqtt_message_t message;

uint8_t data[] = {
    // type 1
    0x10,
    // length 94
    0x5e,
    // protocol name
    0x00, 0x06,
    0x4d, 0x51, 0x49, 0x73, 0x64, 0x70,
    // protocol version
    0x03,
    // flags
    0xf6,
    // keep-alive
    0x00, 0x1e,
    // client id
    0x00, 0x05,
    0x68, 0x65, 0x6c, 0x6c, 0x6f,
    // will topic
    0x00, 0x06,
    0x73, 0x68, 0x6f, 0x75, 0x74, 0x73,
    // will message
    0x00, 0x16,
    0x59, 0x4f, 0x20, 0x57, 0x48, 0x41, 0x54, 0x27, 0x53, 0x20, 0x55, 0x50, 0x20, 0x4d, 0x59, 0x20, 0x48, 0x4f, 0x4d, 0x49, 0x45, 0x53,
    // username
    0x00, 0x07,
    0x74, 0x68, 0x65, 0x75, 0x73, 0x65, 0x72,
    // password (md5)
    0x00, 0x20,
    0x62, 0x61, 0x33, 0x63, 0x38, 0x33, 0x33, 0x34, 0x38, 0x62, 0x64, 0x64, 0x66, 0x37, 0x62, 0x33, 0x36, 0x38, 0x62, 0x34, 0x37, 0x38, 0x61, 0x63, 0x30, 0x36, 0x64, 0x33, 0x33, 0x34, 0x30, 0x65,
	  };

	mqtt_parser_init(&parser);
	mqtt_serialiser_init(&serialiser);
	mqtt_message_init(&message);

 size_t nread = 0;
 int rc = 0, loops = 0;

	  printf("parser running (%zu bytes)\n", sizeof data);
	  do {
	    printf("  loop %d\n", ++loops);
	    printf("    state: %d\n", parser.state);
	    printf("    offset: %zu\n", nread);
	    rc = mqtt_parser_execute(&parser, &message, data, sizeof data, &nread);
	    printf("    rc: %d\n", rc);

	    if (rc == MQTT_PARSER_RC_WANT_MEMORY) {
	      printf("    bytes requested: %zu\n", parser.buffer_length);
	      mqtt_parser_buffer(&parser, malloc(parser.buffer_length), parser.buffer_length);
	    }
	  } while (rc == MQTT_PARSER_RC_CONTINUE || rc == MQTT_PARSER_RC_WANT_MEMORY);

	  printf("\n");
	  printf("parser info\n");
	  printf("  state: %d\n", parser.state);
	  if (rc == MQTT_PARSER_RC_ERROR) {
	    printf("  error: %s\n", mqtt_error_string(parser.error));
	  }
	  printf("  nread: %zd\n", nread);
	  printf("  loops: %d\n", loops);
	  printf("\n");

	  //mqtt_message_dump(&message);
	  mqtt_message_dump_k25(&message,io);

	  printf("\n");

	  size_t packet_length = mqtt_serialiser_size(&serialiser, &message);
	  uint8_t* packet = malloc(packet_length);
	  mqtt_serialiser_write(&serialiser, &message, packet, packet_length);

	  printf("packet length: %zu\n", packet_length);
	  printf("packet data:   ");
	  for (int i=0;i<packet_length;++i) {
	    printf("%02x ", packet[i]);
	  }
	  printf("\n");

	  printf("\n");
	  printf("difference: %d\n", memcmp(data, packet, packet_length));

	  //return 0;

}

static uint8_t MQTT_PrintHelp(const CLS1_StdIOType *io) {
  CLS1_SendHelpStr("MQTT", "MQTT commands\r\n", io->stdOut);
  CLS1_SendHelpStr("  help|status", "Print help or status information\r\n", io->stdOut);
  CLS1_SendHelpStr("  MCZ test ", "Test MQTT parser\r\n", io->stdOut);
}

uint8_t MQTT_ParseCommand(const unsigned char *cmd, bool *handled, const CLS1_StdIOType *io) {
  uint32_t val;
  uint8_t res;
  const unsigned char *p;
  uint8_t pwd[24], ssid[24];

  if (UTIL1_strcmp((char*)cmd, CLS1_CMD_HELP)==0 || UTIL1_strcmp((char*)cmd, "MQTT help")==0) {
    *handled = TRUE;
    res = MQTT_PrintHelp(io);
  } else if (UTIL1_strcmp((char*)cmd, "MQTT test")==0) {
    *handled = TRUE;
    MQTT_test(io);
    CLS1_SendStr("Test réalisé\r\n", io->stdErr);
  }
 return res;
}
