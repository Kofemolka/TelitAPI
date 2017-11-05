#pragma once

typedef enum {
	AT_NONE = -1,
	AT_OK = 0,
	AT_ERR,
	AT_TIMEOUT
} at_err_code_t;

extern void at_cmd_send(const char* cmd);
extern at_err_code_t at_cmd(const char* cmd);
extern at_err_code_t at_cmd_arg(const char* cmd, ...);
extern at_err_code_t at_req(const char* cmd, void(*cb)(const char*));
extern at_err_code_t at_raw(const char* data, int len);
extern at_err_code_t at_recv(void(*handleData)(const char*, int));