#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <stddef.h>

int get_serial(char *buf, size_t size);
int get_hardware_uuid(char *buf, size_t size);
int get_product_name(char *buf, size_t size);
int get_board_id(char *buf, size_t size);

#endif
