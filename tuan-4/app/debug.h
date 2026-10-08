#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include <stddef.h>

void debug_init(void);
void debug_write(const char *data, size_t length);
void debug_printf(const char *format, ...);

#endif
