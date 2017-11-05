#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

#include "util.h"

#define _TRACE_WHITE_ "                                                                                          "
#define _TRACE_TRAIL_	60

#ifdef _TRACE_
void __trace_at_out(const char* buff)
{
	printf(">%s\n\r", buff);
}

void __trace_at_in(const char* buff, int len)
{
	printf("<%.*s", len, buff);
}

void __trace_log(const char* format, ...)
{
	va_list arg;

	va_start(arg, format);

	char buff[256];
	_vsprintf_l(buff, format, NULL, arg);

	va_end(arg);

	printf("\n\r%.*s%s\n\r", _TRACE_TRAIL_, _TRACE_WHITE_, buff);
}
#else
void __trace_at_out(const char* buff) {}
void __trace_at_in(const char* buff, int len) {}
void __trace_log(const char* buff, ...) {}
#endif


static short is_space(char c)
{
	return c == '\n' || c == '\r' || c == ' ';
}

void strtrim(const char *src, char* dest)
{	
	char *end;
	
	// Trim leading space
	while (is_space(*src)) src++;

	if (*src == 0)  // All spaces?
	{		
		return;
	}

	// Trim trailing space
	end = src + strlen(src) - 1;
	while (end > src && is_space(*end)) end--;

	strncpy(dest, src, end - src + 1);
	dest[end - src + 1] = 0;	
}

void sleep(int millis)
{
	Sleep(millis);
}