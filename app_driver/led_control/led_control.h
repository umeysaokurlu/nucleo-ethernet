/*
 * led_control.h
 *
 *  Created on: Aug 5, 2025
 *      Author: Umeysa Okurlu
 *
 * NUCLEO-F439ZI uzerindeki kullanici LED'lerini (LD1/LD2/LD3)
 * kontrol etmek icin donanim soyutlama katmani.
 */

#ifndef __LED_CONTROL_H__
#define __LED_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum
{
  LED_ID_1 = 0,   /* LD1 - Yesil   */
  LED_ID_2,       /* LD2 - Mavi    */
  LED_ID_3,       /* LD3 - Kirmizi */
  LED_ID_COUNT
} led_id_t;

/**
  * @brief  LED durumu degistiginde cagrilacak callback tipi.
  *         Kaynagi (TCP mi MQTT mi) fark etmeksizin her degisimde tetiklenir.
  */
typedef void (*led_status_cb_t)(led_id_t led, bool is_on);

/**
  * @brief  LED donanimini hazirlar.
  * @param  None
  * @retval None
  */
void led_control_init(void);

/**
  * @brief  Belirtilen LED'i acar.
  * @param  led Kontrol edilecek LED kimligi
  * @retval None
  */
void led_control_on(led_id_t led);

/**
  * @brief  Belirtilen LED'i kapatir.
  * @param  led Kontrol edilecek LED kimligi
  * @retval None
  */
void led_control_off(led_id_t led);

/**
  * @brief  Belirtilen LED'in durumunu tersine cevirir.
  * @param  led Kontrol edilecek LED kimligi
  * @retval None
  */
void led_control_toggle(led_id_t led);

/**
  * @brief  Belirtilen LED'in mevcut durumunu dondurur.
  * @param  led Sorgulanacak LED kimligi
  * @retval true: yanik, false: sonuk
  */
bool led_control_get_state(led_id_t led);

/**
  * @brief  Durum degisim callback'ini kaydeder (ornegin MQTT publish icin).
  * @param  cb Cagrilacak callback fonksiyonu, devre disi birakmak icin NULL
  * @retval None
  */
void led_control_set_status_callback(led_status_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONTROL_H__ */
