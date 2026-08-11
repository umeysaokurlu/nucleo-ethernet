/*
 * ethernet_driver.c
 *
 *  Created on: Aug 5, 2025
 *      Author: Umeysa Okurlu
 */

#include "ethernet_driver.h"
#include "lwip.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include <string.h>

/* lwip.c icinde tanimlanan global network interface */
extern struct netif gnetif;

/* Private variables ----------------------------------------------------------*/
static struct tcp_pcb *server_pcb = NULL;
static struct tcp_pcb *client_pcbs[ETHERNET_MAX_CLIENTS] = { NULL };
static ethernet_rx_callback_t rx_callback = NULL;

/* Private function prototypes -------------------------------------------------*/
static err_t ethernet_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void  ethernet_tcp_error(void *arg, err_t err);
static err_t ethernet_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static int   client_slot_find(struct tcp_pcb *pcb);
static int   client_slot_find_free(void);
static void  client_slot_remove(struct tcp_pcb *pcb);
static int   tcp_send_chunked(struct tcp_pcb *pcb, const uint8_t *data, uint16_t len);

/**
  * @brief  Ethernet/lwIP konfigurasyonunu yapar ve TCP sunucusunu baslatir.
  * @param  None
  * @retval None
  */
void ethernet_init(void)
{
  ip4_addr_t ipaddr, netmask, gw;

  /* --- lwIP / netif konfigurasyonu --- */
  IP4_ADDR(&ipaddr, ETHERNET_IP_ADDR0, ETHERNET_IP_ADDR1, ETHERNET_IP_ADDR2, ETHERNET_IP_ADDR3);
  IP4_ADDR(&netmask, ETHERNET_NETMASK_ADDR0, ETHERNET_NETMASK_ADDR1, ETHERNET_NETMASK_ADDR2, ETHERNET_NETMASK_ADDR3);
  IP4_ADDR(&gw, ETHERNET_GW_ADDR0, ETHERNET_GW_ADDR1, ETHERNET_GW_ADDR2, ETHERNET_GW_ADDR3);

  netif_set_addr(&gnetif, &ipaddr, &netmask, &gw);

  /* --- TCP sunucusu: bind + listen + accept --- */
  server_pcb = tcp_new();
  if (server_pcb == NULL)
  {
    Error_Handler();
  }

  if (tcp_bind(server_pcb, IP_ADDR_ANY, ETHERNET_TCP_PORT) != ERR_OK)
  {
    Error_Handler();
  }

  server_pcb = tcp_listen(server_pcb);
  if (server_pcb == NULL)
  {
    Error_Handler();
  }

  tcp_accept(server_pcb, ethernet_tcp_accept);
}

/**
  * @brief  lwIP'nin periyodik islerini yurutur.
  * @param  None
  * @retval None
  */
void ethernet_run(void)
{
  MX_LWIP_Process();
}

/**
  * @brief  En az bir istemci bagli olup olmadigini dondurur.
  * @param  None
  * @retval true en az bir istemci bagliysa
  */
bool ethernet_is_connected(void)
{
  return (ethernet_client_count() > 0U);
}

/**
  * @brief  Su an bagli istemci sayisini dondurur.
  * @param  None
  * @retval Bagli istemci sayisi
  */
uint8_t ethernet_client_count(void)
{
  uint8_t count = 0U;
  int i;

  for (i = 0; i < ETHERNET_MAX_CLIENTS; i++)
  {
    if (client_pcbs[i] != NULL)
    {
      count++;
    }
  }

  return count;
}

/**
  * @brief  Belirtilen tek istemciye veri gonderir.
  * @param  client Hedef istemci kimligi
  * @param  data   Gonderilecek veri
  * @param  len    Veri uzunlugu
  * @retval 0 basarili, -1 hata
  */
int ethernet_send_to(ethernet_client_t client, const uint8_t *data, uint16_t len)
{
  struct tcp_pcb *pcb = (struct tcp_pcb *)client;

  if (pcb == NULL || data == NULL || len == 0U)
  {
    return -1;
  }

  if (client_slot_find(pcb) < 0)
  {
    /* Bu istemci artik bagli degil (baglanti kopmus olabilir) */
    return -1;
  }

  return tcp_send_chunked(pcb, data, len);
}

/**
  * @brief  Bagli tum istemcilere ayni veriyi gonderir.
  * @param  data Gonderilecek veri
  * @param  len  Veri uzunlugu
  * @retval Basariyla gonderilen istemci sayisi
  */
int ethernet_broadcast(const uint8_t *data, uint16_t len)
{
  int sent_count = 0;
  int i;

  if (data == NULL || len == 0U)
  {
    return 0;
  }

  for (i = 0; i < ETHERNET_MAX_CLIENTS; i++)
  {
    if (client_pcbs[i] != NULL)
    {
      if (tcp_send_chunked(client_pcbs[i], data, len) == 0)
      {
        sent_count++;
      }
    }
  }

  return sent_count;
}

/**
  * @brief  Mesaj alindiginda cagrilacak callback fonksiyonunu kaydeder.
  * @param  callback Kaydedilecek callback
  * @retval None
  */
void ethernet_set_rx_callback(ethernet_rx_callback_t callback)
{
  rx_callback = callback;
}

/**
  * @brief  Bagli istemciden veri geldiginde lwIP tarafindan cagrilir.
  * @param  arg  Kullanilmiyor
  * @param  tpcb Veriyi gonderen istemcinin PCB'si
  * @param  p    Gelen veri (pbuf zinciri)
  * @param  err  lwIP hata kodu
  * @retval ERR_OK
  */
static err_t ethernet_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct pbuf *q;

  (void)arg;

  if (p == NULL)
  {
    /* Karsi taraf baglantiyi kapatti */
    client_slot_remove(tpcb);
    tcp_close(tpcb);
    return ERR_OK;
  }

  if (err != ERR_OK)
  {
    pbuf_free(p);
    return err;
  }

  tcp_recved(tpcb, p->tot_len);

  if (rx_callback != NULL)
  {
    for (q = p; q != NULL; q = q->next)
    {
      rx_callback((ethernet_client_t)tpcb, (const uint8_t *)q->payload, (uint16_t)q->len);
    }
  }

  pbuf_free(p);
  return ERR_OK;
}

/**
  * @brief  Baglanti hatasinda lwIP tarafindan cagrilir (pcb zaten serbest
  *         birakilmis olur, tekrar tcp_close cagirmaya gerek yoktur).
  * @param  arg PCB'ye ait arg (bu tasarimda kullanilmiyor)
  * @param  err Hata kodu
  * @retval None
  */
static void ethernet_tcp_error(void *arg, err_t err)
{
  (void)err;

  /* arg olarak tcp_arg() ile pcb'nin kendisini sakladik */
  client_slot_remove((struct tcp_pcb *)arg);
}

/**
  * @brief  Yeni bir istemci baglandiginda lwIP tarafindan cagrilir.
  *         Bos slot varsa istemciyi listeye ekler; doluysa baglantiyi
  *         reddeder (mevcut istemciler etkilenmez).
  * @param  arg    Kullanilmiyor
  * @param  newpcb Yeni baglanan istemcinin PCB'si
  * @param  err    lwIP hata kodu
  * @retval ERR_OK basarili, ERR_MEM slot yoksa
  */
static err_t ethernet_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  int slot;

  (void)arg;

  if (err != ERR_OK || newpcb == NULL)
  {
    return ERR_VAL;
  }

  slot = client_slot_find_free();
  if (slot < 0)
  {
    /* Maksimum istemci sayisina ulasildi, yeni baglantiyi reddet */
    tcp_close(newpcb);
    return ERR_MEM;
  }

  client_pcbs[slot] = newpcb;

  tcp_arg(newpcb, newpcb);
  tcp_recv(newpcb, ethernet_tcp_recv);
  tcp_err(newpcb, ethernet_tcp_error);
  tcp_setprio(newpcb, TCP_PRIO_NORMAL);

  return ERR_OK;
}

/**
  * @brief  Verilen PCB'nin istemci listesindeki slot indeksini bulur.
  * @param  pcb Aranacak PCB
  * @retval Slot indeksi, bulunamazsa -1
  */
static int client_slot_find(struct tcp_pcb *pcb)
{
  int i;

  for (i = 0; i < ETHERNET_MAX_CLIENTS; i++)
  {
    if (client_pcbs[i] == pcb)
    {
      return i;
    }
  }

  return -1;
}

/**
  * @brief  Bos (kullanilmayan) bir slot bulur.
  * @param  None
  * @retval Bos slot indeksi, hicbiri yoksa -1
  */
static int client_slot_find_free(void)
{
  return client_slot_find(NULL);
}

/**
  * @brief  Verilen PCB'yi istemci listesinden cikarir.
  * @param  pcb Cikarilacak PCB
  * @retval None
  */
static void client_slot_remove(struct tcp_pcb *pcb)
{
  int slot = client_slot_find(pcb);

  if (slot >= 0)
  {
    client_pcbs[slot] = NULL;
  }
}

/**
  * @brief  Verilen PCB'ye veriyi parca parca (gonderim tamponu
  *         doluysa bekleyerek) yazar.
  * @param  pcb  Hedef PCB
  * @param  data Gonderilecek veri
  * @param  len  Veri uzunlugu
  * @retval 0 basarili, -1 hata
  */
static int tcp_send_chunked(struct tcp_pcb *pcb, const uint8_t *data, uint16_t len)
{
  uint16_t remaining = len;
  const uint8_t *ptr = data;
  uint16_t chunk;
  err_t err;

  while (remaining > 0U)
  {
    chunk = tcp_sndbuf(pcb);
    if (chunk == 0U)
    {
      tcp_output(pcb);
      chunk = tcp_sndbuf(pcb);
      if (chunk == 0U)
      {
        return -1;
      }
    }

    if (chunk > remaining)
    {
      chunk = remaining;
    }

    err = tcp_write(pcb, ptr, chunk, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK)
    {
      return -1;
    }

    ptr += chunk;
    remaining -= chunk;
  }

  tcp_output(pcb);
  return 0;
}
