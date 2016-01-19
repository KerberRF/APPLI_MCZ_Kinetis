#include <stdio.h>

#include "MQTT_buffer.h"

void mqtt_buffer_dump(mqtt_buffer_t* buffer) {

	printf("[%d] ", buffer->length);

  char hex = 0;
  for (int i=0;i<buffer->length;++i) {
    if (buffer->data[i] < 0x20 || buffer->data[i] > 0x7e) {
      hex = 1;
      break;
    }
  }

  if (hex) {
    mqtt_buffer_dump_hex(buffer);
  } else {
    mqtt_buffer_dump_ascii(buffer);
  }
}

void mqtt_buffer_dump_kinetis(mqtt_buffer_t* buffer, const CLS1_StdIOType *io) {

	CLS1_SendStr("[", io->stdErr);
	CLS1_SendNum32u(buffer->length, io->stdErr);
	CLS1_SendStr("] ", io->stdErr);

  char hex = 0;
  for (int i=0;i<buffer->length;++i) {
    if (buffer->data[i] < 0x20 || buffer->data[i] > 0x7e) {
      hex = 1;
      break;
    }
  }

  if (hex) {
    mqtt_buffer_dump_hex_kinetis(buffer,io);
  } else {
    mqtt_buffer_dump_ascii_kinetis(buffer,io);
  }
}

void mqtt_buffer_dump_ascii(mqtt_buffer_t* buffer) {
  for (int i=0;i<buffer->length;++i) {
    printf("%c", buffer->data[i]);
  }
}

void mqtt_buffer_dump_hex(mqtt_buffer_t* buffer) {
  for (int i=0;i<buffer->length;++i) {
    printf("%02x ", buffer->data[i]);
  }
}

void mqtt_buffer_dump_ascii_kinetis(mqtt_buffer_t* buffer, const CLS1_StdIOType *io) {
  for (int i=0;i<buffer->length;++i) {
	 CLS1_SendChar(buffer->data[i]);
	 //printf("%c", buffer->data[i]);
  }
}

void mqtt_buffer_dump_hex_kinetis(mqtt_buffer_t* buffer, const CLS1_StdIOType *io) {
  for (int i=0;i<buffer->length;++i) {
	 CLS1_SendNum8u(buffer->data[i], io->stdErr);
	 CLS1_SendStr(" ", io->stdErr);
    //printf("%02x ", buffer->data[i]);
  }
}
