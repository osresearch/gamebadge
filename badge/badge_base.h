/** @file badge_base.h */
#ifndef BADGE_BASE_H
#define BADGE_BASE_H

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize badge base driver.
 * @return ESP_OK on success; any other value indicates an error
 */
extern esp_err_t badge_base_init(void);

#ifdef __cplusplus
};
#endif


#endif // BADGE_BASE_H
