# Teleinfo to MQTT

Code is AS-IS without any warranty of any kind...

Hardware used :
* Nodemcu 1.0 (but any ESP8266 breakout with enough pins should do)
* Optocoupler SFH620A (but LTV-814 could be even better)
* 2N7000 or BS170
* 2 * 10k + 1 * 4.7k resistors

![Schematics](teleinfo-esp8266_bb.png)

* Version "Historique" (i.e. compatible with older meters) only for the moment
* Version "Standard" (a.k.a new Linky version) to be written 

The python OTA helper can be used to upgrade the software on the ESP8266

Corresponding blog entry: http://www.guillier.org/blog/2017/10/teleinfo-with-wifi-on-the-linky/
