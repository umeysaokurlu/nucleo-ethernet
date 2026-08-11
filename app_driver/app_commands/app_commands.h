/*
 * app_commands.h
 *
 * TCP veya MQTT uzerinden gelen metin komutlarini yorumlayip ilgili
 * donanim aksiyonuna (su an icin LED) yonlendiren uygulama katmani.
 *
 * Bu katman haberlesme yontemini bilmez: cevap gondermek icin
 * cagiran taraftan bir "reply" fonksiyonu alir. Boylece TCP ve MQTT
 * ayni komut isleme mantigini paylasirken, cevabi kendi kanalindan
 * (TCP soketi veya MQTT ack topic'i) gonderebilir.
 */

#ifndef __APP_COMMANDS_H__
#define __APP_COMMANDS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
  * @brief  Komut isleme sonucunda cevap gondermek icin kullanilan
  *         fonksiyon tipi. Cagiran taraf (TCP ya da MQTT) kendi
  *         gonderim mekanizmasini bu imzayla saglar.
  * @param  msg Gonderilecek metin (ornegin "OK\r\n")
  * @param  len Metin uzunlugu
  */
typedef void (*app_reply_fn_t)(const char *msg, uint16_t len);

/**
  * @brief  Gelen ham veriyi (ornegin "LED1_ON\r\n") isler, ilgili LED
  *         aksiyonunu tetikler ve verilen reply fonksiyonu araciligiyla
  *         "OK"/"ERR" cevabi gonderir.
  * @param  data     Gelen veri bufferi
  * @param  len      Gelen veri uzunlugu
  * @param  reply_fn Cevabi gondermek icin kullanilacak fonksiyon;
  *                   NULL gecilirse cevap gonderilmez
  * @retval None
  */
void app_commands_process(const uint8_t *data, uint16_t len, app_reply_fn_t reply_fn);

#ifdef __cplusplus
}
#endif

#endif /* __APP_COMMANDS_H__ */
