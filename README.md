# Smart Scale

## Design principles

The smart scale has to meet the following requirements
- "Smart": It can be accessed remotely (wireless) by a client and integrated into a smart home network
  (e.g. [Home Assistant](https://www.home-assistant.io/))
- Low power: The device should operate at least 1 year from battery
- Low cost: the device should be in the range of 20-30 USD
- Measurement range: 0-3 kg, 1 g resolution
- Small form factor: although smart bathroom scales are common, smart kitchen scales are not

## Reference hardware

To meet the above requirements, the reference hardware is based on a second-hand kitchen scale,
namely the Soehnle 65106 Fiesta (~6.5 USD). This kitchen scale comes with a bar-type load cell.
The signal from the load cell is processed by an [HX711 ADC](https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf).
[Here](https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide) is a
nice article about different load cell types and how to connect them to the HX711 breakout board
(11 USD at Sparkfun, but similar board with HX711 is available for 1-2 USD).
The connectivity is provided by Silicon Labs' [BGM220 Bluetooth Module Explorer Kit](
    https://www.silabs.com/development-tools/wireless/bluetooth/bgm220-explorer-kit?tab=overview)
(12 USD). This board reads the digital signal from the HX711 and transmits via BLE. The 2 buttons
(TARE and ON-OFF) of the kitchen scale are also connected to the Bluetooth module. The kitchen scale
operates from 2 AA batteries.

TODO: add block diagram and inside/outside photos.
