/*
 * ethernet_driver.h
 *
 *  Created on: Aug 5, 2025
 *      Author: Umeysa Okurlu
 */

#ifndef __ETHERNET_DRIVER_H__
#define __ETHERNET_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* TCP sunucusunun dinleyecegi port */
#define ETHERNET_TCP_PORT       7000

/* Ayni anda kabul edilecek maksimum istemci sayisi */
#define ETHERNET_MAX_CLIENTS    4U

/* Statik IP konfigurasyonu (ethernet_init icinde uygulanir)
   ICS (Internet Connection Sharing) subnetine gore ayarlandi */
#define ETHERNET_IP_ADDR0       192
#define ETHERNET_IP_ADDR1       168
#define ETHERNET_IP_ADDR2       137
#define ETHERNET_IP_ADDR3       50

#define ETHERNET_NETMASK_ADDR0  255
#define ETHERNET_NETMASK_ADDR1  255
#define ETHERNET_NETMASK_ADDR2  255
#define ETHERNET_NETMASK_ADDR3  0

#define ETHERNET_GW_ADDR0       192
#define ETHERNET_GW_ADDR1       168
#define ETHERNET_GW_ADDR2       137
#define ETHERNET_GW_ADDR3       1

/**
  * @brief  Bir istemciyi temsil eden opak referans. Gercek tipi
  *         (struct tcp_pcb *) ethernet_driver.c icinde saklidir;
  *         disariya sadece bir "kimlik" olarak verilir. Cevap
  *         gonderirken ethernet_send_to()'ya geri iletilmelidir.
  */
typedef void *ethernet_client_t;

/**
  * @brief  Bir istemciden mesaj geldiginde cagrilacak callback tipi.
  * @param  client Mesaji gonderen istemcinin kimligi (cevap icin sakla)
  * @param  data   Gelen veri
  * @param  len    Gelen veri uzunlugu
  */
typedef void (*ethernet_rx_callback_t)(ethernet_client_t client, const uint8_t *data, uint16_t len);

/**
  * @brief  Ethernet/lwIP konfigurasyonunu yapar: netif/IP ayarlari ve
  *         TCP sunucusunu (bind + listen + accept) baslatir.
  *         main() icinde bir kere cagrilmalidir.
  * @param  None
  * @retval None
  */
void ethernet_init(void);

/**
  * @brief  lwIP'nin periyodik islerini (paket alma, timeout, link durumu)
  *         yurutur. main() icindeki while(1) donguesunde surekli
  *         cagrilmalidir.
  * @param  None
  * @retval None
  */
void ethernet_run(void);

/**
  * @brief  En az bir istemci bagli olup olmadigini dondurur.
  * @param  None
  * @retval true en az bir istemci bagliysa
  */
bool ethernet_is_connected(void);

/**
  * @brief  Su an bagli istemci sayisini dondurur.
  * @param  None
  * @retval Bagli istemci sayisi
  */
uint8_t ethernet_client_count(void);

/**
  * @brief  Belirtilen tek istemciye veri gonderir.
  * @param  client Hedef istemci kimligi (rx callback'ten alinan)
  * @param  data   Gonderilecek veri
  * @param  len    Veri uzunlugu
  * @retval 0 basarili, -1 istemci gecersiz/bagli degil ya da gonderim hatasi
  */
int ethernet_send_to(ethernet_client_t client, const uint8_t *data, uint16_t len);

/**
  * @brief  Bagli tum istemcilere ayni veriyi gonderir.
  * @param  data Gonderilecek veri
  * @param  len  Veri uzunlugu
  * @retval Basariyla gonderilen istemci sayisi
  */
int ethernet_broadcast(const uint8_t *data, uint16_t len);

/**
  * @brief  Mesaj alindiginda cagrilacak callback fonksiyonunu kaydeder.
  * @param  callback Kaydedilecek callback
  * @retval None
  */
void ethernet_set_rx_callback(ethernet_rx_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* __ETHERNET_DRIVER_H__ */
