/** @file badge_sdcard.h */
#ifndef BADGE_SDCARD_H
#define BADGE_SDCARD_H

#include <stdbool.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif


/** initialize the sdcard inserted sensor
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_sdcard_init(void);

/** report if an sdcard is inserted */
extern bool badge_sdcard_detected(void);

#ifdef __cplusplus
};
#endif


#endif // BADGE_SDCARD_H
