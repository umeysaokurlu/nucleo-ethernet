/*
 * mqtt_driver.c
 *
*  Created on: Aug 5, 2025
 *      Author: Umeysa Okurlu
 */

#include "main.h"
#include "lwip.h"
#include "lwip/apps/mqtt.h"
#include <string.h>
#include <stdio.h>

#include "mqtt_driver.h"
#include "app_commands.h"
#include "led_control.h"

/* Private define -----------------------------------------------------------*/
/* Kullanici ayarlari - kendi broker bilgilerinize gore duzenleyin */
#define MQTT_BROKER_IP_ADDR0   192
#define MQTT_BROKER_IP_ADDR1   168
#define MQTT_BROKER_IP_ADDR2   137
#define MQTT_BROKER_IP_ADDR3   1

#define MQTT_CLIENT_ID          "stm32_nucleo_f439zi"
#define MQTT_KEEPALIVE_SEC      120U
#define MQTT_RECONNECT_MS       5000U

#define MQTT_CMD_TOPIC          "stm32/led/cmd"
#define MQTT_ACK_TOPIC          "stm32/led/ack"
#define MQTT_STATUS_TOPIC_FMT   "stm32/led/status/%d"

/* Private variables ----------------------------------------------------------*/
static mqtt_client_t *s_client;
static uint32_t        s_last_attempt_tick;
static volatile bool    s_mqtt_connected = false;

static char     s_rx_buffer[64];
static uint16_t s_rx_index = 0;

/* Private function prototypes -------------------------------------------------*/
static void mqtt_do_connect(mqtt_client_t *client);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_pub_request_cb(void *arg, err_t result);
static void mqtt_reply(const char *msg, uint16_t len);
static void publish_led_status(led_id_t led, bool is_on);
static void publish_all_status(void);

/**
  * @brief  MQTT istemcisini olusturur ve ilk baglanti denemesini baslatir.
  * @param  None
  * @retval None
  */
void mqtt_driver_init(void)
{
  s_client = mqtt_client_new();
  s_last_attempt_tick = HAL_GetTick();

  /* LED durumu her degistiginde (TCP ya da MQTT kaynakli fark etmez)
     otomatik olarak status topic'ine publish edilsin */
  led_control_set_status_callback(publish_led_status);

  if (s_client != NULL)
  {
    mqtt_do_connect(s_client);
  }
}

/**
  * @brief  Baglanti kopmussa periyodik olarak yeniden dener.
  * @param  None
  * @retval None
  */
void mqtt_driver_run(void)
{
  uint32_t now;

  if (s_client == NULL)
  {
    return;
  }

  if (!s_mqtt_connected)
  {
    now = HAL_GetTick();
    if ((now - s_last_attempt_tick) >= MQTT_RECONNECT_MS)
    {
      s_last_attempt_tick = now;
      mqtt_do_connect(s_client);
    }
  }
}

/**
  * @brief  Belirtilen topic'e mesaj yayinlar.
  * @param  topic   Hedef MQTT topic
  * @param  payload Gonderilecek veri
  * @param  retain  true ise broker mesaji retained olarak saklar
  * @retval 0 basarili, -1 hata
  */
int mqtt_driver_publish(const char *topic, const char *payload, bool retain)
{
  err_t err;

  if (!s_mqtt_connected || s_client == NULL || topic == NULL || payload == NULL)
  {
    return -1;
  }

  err = mqtt_publish(s_client, topic, payload, (u16_t)strlen(payload),
                      1 /* qos */, retain ? 1 : 0, mqtt_pub_request_cb, NULL);

  return (err == ERR_OK) ? 0 : -1;
}

/**
  * @brief  Broker'a baglanti dener.
  * @param  client MQTT istemci nesnesi
  * @retval None
  */
static void mqtt_do_connect(mqtt_client_t *client)
{
  struct mqtt_connect_client_info_t ci;
  ip_addr_t broker_ip;

  memset(&ci, 0, sizeof(ci));
  ci.client_id  = MQTT_CLIENT_ID;
  ci.keep_alive = MQTT_KEEPALIVE_SEC;

  IP4_ADDR(&broker_ip, MQTT_BROKER_IP_ADDR0, MQTT_BROKER_IP_ADDR1,
                        MQTT_BROKER_IP_ADDR2, MQTT_BROKER_IP_ADDR3);

  mqtt_client_connect(client, &broker_ip, MQTT_PORT,
                       mqtt_connection_cb, NULL, &ci);
}

/**
  * @brief  Baglanti durumu degistiginde cagrilir.
  * @param  client MQTT istemci nesnesi
  * @param  arg    Kullanilmiyor
  * @param  status Yeni baglanti durumu
  * @retval None
  */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  (void)arg;

  if (status == MQTT_CONNECT_ACCEPTED)
  {
    s_mqtt_connected = true;

    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb,
                             mqtt_incoming_data_cb, NULL);

    mqtt_subscribe(client, MQTT_CMD_TOPIC, 1, mqtt_sub_request_cb, NULL);

    /* Baglanti kurulur kurulmaz mevcut LED durumlarini yayinla */
    publish_all_status();
  }
  else
  {
    s_mqtt_connected = false;
  }
}

/**
  * @brief  Yeni bir publish mesaji basladiginda cagrilir.
  * @param  arg     Kullanilmiyor
  * @param  topic   Mesajin geldigi topic
  * @param  tot_len Mesajin toplam uzunlugu
  * @retval None
  */
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  (void)arg;
  (void)topic;
  (void)tot_len;

  s_rx_index = 0;
}

/**
  * @brief  Mesaj verisi geldikce cagrilir; mesaj tamamlaninca
  *         app_commands_process()'e yonlendirir. Cevap MQTT ack
  *         topic'i uzerinden gonderilir.
  * @param  arg   Kullanilmiyor
  * @param  data  Gelen veri parcasi
  * @param  len   Gelen veri parcasinin uzunlugu
  * @param  flags Mesajin son parca olup olmadigini belirtir
  * @retval None
  */
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  uint16_t i;

  (void)arg;

  for (i = 0; i < len && s_rx_index < (sizeof(s_rx_buffer) - 1U); i++)
  {
    s_rx_buffer[s_rx_index++] = (char)data[i];
  }

  if (flags & MQTT_DATA_FLAG_LAST)
  {
    s_rx_buffer[s_rx_index] = '\0';
    app_commands_process((const uint8_t *)s_rx_buffer, s_rx_index, mqtt_reply);
    s_rx_index = 0;
  }
}

/**
  * @brief  Subscribe isteminin sonuc callback'i.
  * @param  arg    Kullanilmiyor
  * @param  result Istek sonucu
  * @retval None
  */
static void mqtt_sub_request_cb(void *arg, err_t result)
{
  (void)arg;
  (void)result;
}

/**
  * @brief  Publish isteminin sonuc callback'i.
  * @param  arg    Kullanilmiyor
  * @param  result Istek sonucu
  * @retval None
  */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
  (void)arg;
  (void)result;
}

/**
  * @brief  app_commands_process()'in MQTT kaynakli komutlara cevap
  *         vermek icin kullandigi reply fonksiyonu. Cevabi TCP yerine
  *         MQTT ack topic'ine publish eder.
  * @param  msg Gonderilecek metin ("OK\r\n" / "ERR\r\n")
  * @param  len Metin uzunlugu (bu implementasyonda kullanilmiyor,
  *             mqtt_publish kendi strlen() hesaplar)
  * @retval None
  */
static void mqtt_reply(const char *msg, uint16_t len)
{
  (void)len;

  mqtt_driver_publish(MQTT_ACK_TOPIC, msg, false /* retain */);
}

/**
  * @brief  Tek bir LED'in durumunu kendi status topic'ine publish eder.
  * @param  led   Durumu bildirilecek LED kimligi
  * @param  is_on LED'in yeni durumu
  * @retval None
  */
static void publish_led_status(led_id_t led, bool is_on)
{
  char topic[32];

  snprintf(topic, sizeof(topic), MQTT_STATUS_TOPIC_FMT, (int)(led + 1));
  mqtt_driver_publish(topic, is_on ? "ON" : "OFF", true /* retain */);
}

/**
  * @brief  Tum LED'lerin mevcut durumunu publish eder.
  * @param  None
  * @retval None
  */
static void publish_all_status(void)
{
  int i;

  for (i = 0; i < LED_ID_COUNT; i++)
  {
    publish_led_status((led_id_t)i, led_control_get_state((led_id_t)i));
  }
}
