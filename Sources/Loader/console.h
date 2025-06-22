
#ifndef _DRV_CONSOLE_H_
#define _DRV_CONSOLE_H_

#include <stdarg.h>

extern int g_disable_c_printf;


void console_enter (void);
void console_leave (void);
void c_vprintf (const char *format, va_list argp);
void c_printf (const char *format, ...);
int c_sprintf(char *buf, const char *format, ...);
int c_vsprintf (char *buf, const char *format, va_list argp);

#endif
