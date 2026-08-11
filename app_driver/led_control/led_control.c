/*
 * led_control.c
 *
 *  Created on: Aug 5, 2025
 *      Author: Umeysa Okurlu
 */

#include "led_control.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  bool          state;
} led_desc_t;

/* Private variables ----------------------------------------------------------*/
static led_desc_t leds[LED_ID_COUNT] =
{
  { GPIOB, LD1_Pin, false },
  { GPIOB, LD2_Pin, false },
  { GPIOB, LD3_Pin, false },
};

static led_status_cb_t s_status_cb = NULL;

/* Private function prototypes -------------------------------------------------*/
static void notify_status(led_id_t led);

/**
  * @brief  LED donanimini hazirlar.
  * @param  None
  * @retval None
  */
void led_control_init(void)
{
  int i;

  for (i = 0; i < LED_ID_COUNT; i++)
  {
    leds[i].state = false;
  }
}

/**
  * @brief  Belirtilen LED'i acar.
  * @param  led Kontrol edilecek LED kimligi
  * @retval None
  */
void led_control_on(led_id_t led)
{
  if (led >= LED_ID_COUNT)
  {
    return;
  }

  HAL_GPIO_WritePin(leds[led].port, leds[led].pin, GPIO_PIN_SET);
  leds[led].state = true;
  notify_status(led);
}

/**
  * @brief  Belirtilen LED'i kapatir.
  * @param  led Kontrol edilecek LED kimligi
  * @retval None
  */
void led_control_off(led_id_t led)
{
  if (led >= LED_ID_COUNT)
  {
    return;
  }

  HAL_GPIO_WritePin(leds[led].port, leds[led].pin, GPIO_PIN_RESET);
  leds[led].state = false;
  notify_status(led);
}

/**
  * @brief  Belirtilen LED'in durumunu tersine cevirir.
  * @param  led Kontrol edilecek LED kimligi
  * @retval None
  */
void led_control_toggle(led_id_t led)
{
  if (led >= LED_ID_COUNT)
  {
    return;
  }

  HAL_GPIO_TogglePin(leds[led].port, leds[led].pin);
  leds[led].state = !leds[led].state;
  notify_status(led);
}

/**
  * @brief  Belirtilen LED'in mevcut durumunu dondurur.
  * @param  led Sorgulanacak LED kimligi
  * @retval true: yanik, false: sonuk
  */
bool led_control_get_state(led_id_t led)
{
  if (led >= LED_ID_COUNT)
  {
    return false;
  }

  return leds[led].state;
}

/**
  * @brief  Durum degisim callback'ini kaydeder.
  * @param  cb Cagrilacak callback fonksiyonu, devre disi birakmak icin NULL
  * @retval None
  */
void led_control_set_status_callback(led_status_cb_t cb)
{
  s_status_cb = cb;
}

/**
  * @brief  Kayitli callback varsa durum degisimini bildirir.
  * @param  led Durumu degisen LED kimligi
  * @retval None
  */
static void notify_status(led_id_t led)
{
  if (s_status_cb != NULL)
  {
    s_status_cb(led, leds[led].state);
  }
}
