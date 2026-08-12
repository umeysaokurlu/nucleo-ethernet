# NUCLEO-F439ZI Ethernet TCP/IP & MQTT Communication

Bu proje, NUCLEO-F439ZI geliştirme kartı üzerinde Ethernet tabanlı TCP/IP ve MQTT haberleşmesinin gerçekleştirilmesini amaçlamaktadır.

Proje kapsamında STM32'nin Ethernet arabirimi ve LwIP TCP/IP yığını kullanılarak ağ haberleşmesi oluşturulmuş, TCP üzerinden gelen komutların STM32 tarafından işlenmesi ve LED kontrolüne dönüştürülmesi sağlanmıştır. Ayrıca MQTT haberleşmesi için ayrı bir MQTT driver katmanı oluşturularak STM32'nin MQTT altyapısına dahil edilmesi amaçlanmıştır.

## Kullanılan Donanım

- NUCLEO-F439ZI
- Ethernet kablosu
- Bilgisayar

## Kullanılan Yazılımlar ve Teknolojiler

- STM32CubeIDE
- STM32 HAL
- LwIP
- Ethernet
- TCP/IP
- MQTT
- C programlama dili

## Proje Mimarisi

Proje modüler bir yazılım yapısına sahiptir.

```text
                NUCLEO-F439ZI
                     |
                  Ethernet
                     |
                  LwIP TCP/IP
                     |
          +----------+----------+
          |                     |
      TCP Client            MQTT Driver
          |                     |
          |                     |
      Ethernet Driver      MQTT Broker
          |
          v
    Application Commands
          |
          v
      LED Control