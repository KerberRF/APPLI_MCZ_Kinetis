#include <string.h>
#include <stdio.h>

#include "MQTT_message.h"

void mqtt_message_init(mqtt_message_t* message) {
  memset(message, 0, sizeof (mqtt_message_t));
}

void mqtt_message_dump(mqtt_message_t* message) {
  printf("message\n");
  printf("  type:              %d\n", message->common.type);
  printf("  qos:               %d\n", message->common.qos);
  printf("  dup:               %s\n", message->common.dup    ? "true" : "false");
  printf("  retain:            %s\n", message->common.retain ? "true" : "false");

  if (message->common.type == MQTT_TYPE_CONNECT) {
    printf("  protocol name:     ");
    mqtt_buffer_dump(&(message->connect.protocol_name));
    printf("\n");

    printf("  protocol version:  %d\n", message->connect.protocol_version);

    printf("  has username:      %s\n", message->connect.flags.username_follows ? "true": "false");
    printf("  has password:      %s\n", message->connect.flags.password_follows ? "true": "false");
    printf("  has will:          %s\n", message->connect.flags.will ? "true": "false");
    printf("  will qos:          %d\n", message->connect.flags.will_qos);
    printf("  retains will:      %s\n", message->connect.flags.will_retain ? "true": "false");
    printf("  clean session:     %s\n", message->connect.flags.clean_session ? "true": "false");

    printf("  keep alive:        %d\n", message->connect.keep_alive);

    printf("  client id:         ");
    mqtt_buffer_dump(&(message->connect.client_id));
    printf("\n");

    printf("  will topic:        ");
    mqtt_buffer_dump(&(message->connect.will_topic));
    printf("\n");
    printf("  will message:      ");
    mqtt_buffer_dump(&(message->connect.will_message));
    printf("\n");

    printf("  username:          ");
    mqtt_buffer_dump(&(message->connect.username));
    printf("\n");
    printf("  password:          ");
    mqtt_buffer_dump(&(message->connect.password));
    printf("\n");
  }
}

void mqtt_message_dump_k25(mqtt_message_t* message, const CLS1_StdIOType *io) {

  CLS1_SendStr("\r\nmessage\n\r", io->stdErr);
  CLS1_SendStr("  type:              ", io->stdErr);
  CLS1_SendNum32u(message->common.type, io->stdErr);
  CLS1_SendStr("\r\n", io->stdErr);
  CLS1_SendStr("  qos:               ", io->stdErr);
  CLS1_SendNum32u(message->common.qos, io->stdErr);
  CLS1_SendStr("\r\n", io->stdErr);
  CLS1_SendStr("  dup:               ", io->stdErr);
  CLS1_SendNum32u(message->common.dup, io->stdErr);
  CLS1_SendStr("\r\n", io->stdErr);
  CLS1_SendStr("  retain:            ", io->stdErr);
  CLS1_SendNum32u(message->common.retain, io->stdErr);
  CLS1_SendStr("\r\n", io->stdErr);

  if (message->common.type == MQTT_TYPE_CONNECT) {
	CLS1_SendStr("  protocol name:     ", io->stdErr);
    //printf("  protocol name:     ");
    mqtt_buffer_dump(&(message->connect.protocol_name));
    CLS1_SendStr("\r\n", io->stdErr);
    //printf("\n");
    CLS1_SendStr("  protocol version:  ", io->stdErr);
    CLS1_SendNum8u(message->connect.protocol_version, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  has username:      ", io->stdErr);
    CLS1_SendChar(message->connect.flags.username_follows);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  has password:       ", io->stdErr);
    CLS1_SendChar(message->connect.flags.password_follows);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  has will:          ", io->stdErr);
    CLS1_SendChar(message->connect.flags.will);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  will qos:          ", io->stdErr);
    CLS1_SendChar(message->connect.flags.will_qos);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  retains will:      ", io->stdErr);
    CLS1_SendChar(message->connect.flags.will_retain);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  clean session:     ", io->stdErr);
    CLS1_SendChar(message->connect.flags.clean_session);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  keep alive:         ", io->stdErr);
    CLS1_SendNum16u(message->connect.keep_alive, io->stdErr);
    CLS1_SendStr("\r\n", io->stdErr);

    CLS1_SendStr("  client id:         ", io->stdErr);
    mqtt_buffer_dump_kinetis(&(message->connect.client_id),io);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  will topic:        ", io->stdErr);
    mqtt_buffer_dump_kinetis(&(message->connect.will_topic),io);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  will message:      ", io->stdErr);
    mqtt_buffer_dump_kinetis(&(message->connect.will_message),io);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  username:          ", io->stdErr);
    mqtt_buffer_dump_kinetis(&(message->connect.username),io);
    CLS1_SendStr("\r\n", io->stdErr);
    CLS1_SendStr("  password:          ", io->stdErr);
    mqtt_buffer_dump_kinetis(&(message->connect.password),io);
    CLS1_SendStr("\r\n", io->stdErr);
    }
}
