#ifndef MQTT_BUFFER_H
#define MQTT_BUFFER_H

#include <stdint.h>
#include "CLS1.h"

typedef struct mqtt_buffer_s {
  uint32_t length;
  uint8_t* data;
} mqtt_buffer_t;

void mqtt_buffer_dump(mqtt_buffer_t* buffer);
void mqtt_buffer_dump_kinetis(mqtt_buffer_t* buffer, const CLS1_StdIOType *io);
void mqtt_buffer_dump_ascii(mqtt_buffer_t* buffer);
void mqtt_buffer_dump_hex(mqtt_buffer_t* buffer);
void mqtt_buffer_dump_ascii_kinetis(mqtt_buffer_t* buffer, const CLS1_StdIOType *io);
void mqtt_buffer_dump_hex_kinetis(mqtt_buffer_t* buffer, const CLS1_StdIOType *io);

#endif
