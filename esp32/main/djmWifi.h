#ifndef __DJM_WIFI_H__
#define __DJM_WIFI_H__

#include <stdint.h>

int djm_wifi_init(const char *ssid, const char *password);

typedef int (*fileChunkHandler_f)(int len, uint8_t *bytes, void *user);
int djm_download(const char *url, fileChunkHandler_f callback, void *user);


#endif // __DJM_WIFI_H__
