/*
 * app_commands.c
 */

#include "app_commands.h"
#include "led_control.h"
#include <string.h>

/* Private define ---------------------------------------------------------*/
#define CMD_BUF_SIZE  32U

/**
  * @brief  Gelen ham veriyi isler, ilgili LED aksiyonunu tetikler ve
  *         verilen reply fonksiyonu araciligiyla cevap gonderir.
  * @param  data     Gelen veri bufferi
  * @param  len      Gelen veri uzunlugu
  * @param  reply_fn Cevabi gondermek icin kullanilacak fonksiyon
  * @retval None
  */
void app_commands_process(const uint8_t *data, uint16_t len, app_reply_fn_t reply_fn)
{
  char cmd[CMD_BUF_SIZE];
  uint16_t copy_len;
  uint16_t i;
  uint8_t  valid;

  copy_len = (len < (CMD_BUF_SIZE - 1U)) ? len : (CMD_BUF_SIZE - 1U);

  memcpy(cmd, data, copy_len);
  cmd[copy_len] = '\0';

  /* CR/LF temizligi: ilk satir sonu karakterinde kes */
  for (i = 0; i < copy_len; i++)
  {
    if (cmd[i] == '\r' || cmd[i] == '\n')
    {
      cmd[i] = '\0';
      break;
    }
  }

  valid = 1U;

  if (strcmp(cmd, "LED1_ON") == 0)
  {
    led_control_on(LED_ID_1);
  }
  else if (strcmp(cmd, "LED1_OFF") == 0)
  {
    led_control_off(LED_ID_1);
  }
  else if (strcmp(cmd, "LED2_ON") == 0)
  {
    led_control_on(LED_ID_2);
  }
  else if (strcmp(cmd, "LED2_OFF") == 0)
  {
    led_control_off(LED_ID_2);
  }
  else if (strcmp(cmd, "LED3_ON") == 0)
  {
    led_control_on(LED_ID_3);
  }
  else if (strcmp(cmd, "LED3_OFF") == 0)
  {
    led_control_off(LED_ID_3);
  }
  else
  {
    valid = 0U;
  }

  if (reply_fn == NULL)
  {
    return;
  }

  if (valid)
  {
    reply_fn("OK\r\n", 4U);
  }
  else
  {
    reply_fn("ERR\r\n", 5U);
  }
}
