#pragma once

typedef struct
{
	int		poll_period;
	char	firmware_url[50];
	char	sas[50];
	int		last_telemetry;

	char	etag[50];
} device_config_t;

extern int parseDeviceConfig(const char* data, int len, device_config_t* cfg);