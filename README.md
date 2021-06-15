# Sully Tracker!


## Overview

Sully is our pup and he's a good boy.

Sully has a big fenced in back yard and lots of room to run and play.

However, sometimes he misses his friend Abby and decides he should go visit her, and having a 4 foot tall fence seems only a minor inconvenience for Sully.

I decided to see if I could use technology to help keep an eye on Sully until we figure out better fencing solutions:



## How it works

Using an Arduino board with a cellular modem and attached GPS module, the tracker will get the current location and upload it over the cellular connection to Grafana Cloud's hosted Loki where I can see Sully's location in near real-time.

## GSM (Cellular Connection)

For a cellular connection I am currently using [Hologram](https://www.hologram.io/) as it has a reasonably cost effective IoT type pricing model where the monthly cost is low and you pay for bytes sent.

The connection is made using `https` which is not really a "low bandwidth" protocol, so this does contrain us a little on how often we send updates.

The HTTP header overhead on every request is around 130 bytes, the payload varies but is usually 150-175 bytes, you can see that the HTTP overhead is pretty significant and this is why we batch a few requests as well as limit the update interval to every 2seconds.

The biggest and most unpredictable consumer of data however is the initial connection and SSL negotiation, if you have very good cellular signal than this usually only happens once as the connection is kept open and reused, however if the cellular signal is week and the connection is frequently remade this can make a big difference on data consumption and cost.

In my testing I typically see an average of aboug 4kB/minute of data usage, however in the worst cases I've seen that as high a 10kB/minute.

In the US Hologram charges $0.40/MB, so this works out to be about ~$0.10/hour on average but as high as ~$0.25/hour if a lot of reconnections are made.

This approach is nice because it makes it simple to send data directly to a hosted service but it's not the most cost effective using https, in the future I may add an intermediate server option to communicate using data encrypted by a pre-shared key over UDP or TCP directly to make the costs predictable.

There will also be another version of this project someday which uses point to point radios and the 900MHz LoRa radios to get local location up to 1-2km line of sight, more to come on this :)

## Parts

* [Arduino MKRGSM1400](https://store.arduino.cc/usa/mkr-gsm-1400)
* [Arduino MKRGPS](https://store.arduino.cc/usa/mkr-gps-shield)
* [Battery](https://www.amazon.com/gp/product/B07TXLJNJM/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1) **NOTE: THIS BATTERY CAME WITH THE POSITIVE AND NEGATIVE WIRES BACKWARDS FOR WHAT THE ARDUINO REQUIRES** Also any li-po battery should work this was one that the size was close to what I wanted (haven't done runtime calcs on this yet but it runs for a few hours without issue so far).
* [Antenna](https://www.adafruit.com/product/1991) (this is good for the US and LTE bands in the US, not sure if something different might be required for other countries)

## Dependencies

* **Arduino_MKRGPS**
* **GrafanaLoki**
  * **SnappyProto** 
  * **PromLokiTransport** 
    * **ArduinoBearSSL** 
    * **ArduinoECCX08** 
    * **MKRGSM** 
  * **ArduinoHttpClient**

## Dashboards

Comming soon!!! (Sorry ran out of time before the talk)

## Enclosure

Coming soon!!! So far I just have a very basic enclosure I made and printed but I will upload the STL for it soon.