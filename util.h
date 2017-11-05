#pragma once

#define _TRACE_ 

extern void __trace_at_out(const char* buff);
extern void __trace_at_in(const char* buff, int len);
extern void __trace_log(const char* format, ...);

//extern char *strtrim(char *str);
extern void strtrim(const char *src, char* dest);

extern void sleep(int millis);

