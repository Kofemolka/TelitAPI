#pragma once

typedef enum
{
	DEV_CFG_NONE,
	DEV_CFG_OK
} device_config_status_t;

typedef struct
{
	int		poll_period;
	char	firmware_url[50];
	char	sas[50];
	int		last_telemetry;

	char	etag[50];
} device_config_t;

extern device_config_status_t parseDeviceConfig(const char* data, int len, device_config_t* cfg);