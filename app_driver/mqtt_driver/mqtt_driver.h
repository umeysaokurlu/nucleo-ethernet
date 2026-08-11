/*
 * mqtt_driver.h
 *
 *  Created on: Aug 6, 2026
 *   Author: Umeysa Okurlu
 */

#ifndef MQTT_DRIVER_H_
#define MQTT_DRIVER_H_

#ifdef __cplusplus
extern "C"  {
#endif

#include <stdbool.h>

/**
  * @brief  MQTT istemcisini olusturur. main() icinde bir kere,
  *         ethernet_init() sonrasinda cagrilmalidir.
  * @param  None
  * @retval None
  */
void mqtt_driver_init(void);

/**
  * @brief  MQTT baglantisini periyodik olarak yonetir (baglanti kopmussa
  *         yeniden dener). main() icindeki while(1) donguesunde surekli
  *         cagrilmalidir.
  * @param  None
  * @retval None
  */
void mqtt_driver_run(void);

/**
  * @brief  Belirtilen topic'e mesaj yayinlar (publish).
  * @param  topic   Hedef MQTT topic
  * @param  payload Gonderilecek veri (NULL-terminated string)
  * @param  retain  true ise broker mesaji "son bilinen deger" olarak saklar
  * @retval 0 basarili, -1 hata (baglanti yok vs.)
  */
int mqtt_driver_publish(const char *topic, const char *payload, bool retain);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_DRIVER_H_ */
