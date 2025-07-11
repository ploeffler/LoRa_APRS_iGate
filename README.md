#### This Fork of richonguzman/LoRa_APRS_iGate/ is based on V2.3 of his firmware.

# You will see a more bi-directional view on the iGate-functionality, including:

* ### forward all position packets
  all position pakets that come form IS are gated to the LoRa interface (not only objects). This leads to a much better useability for stations that have their tracker connected to a device with map(s) and is therefore ideal for blackout, offgrid and offline (internet) use and for users in the field.

* ### dynamic filtering:
  if you have a (e.g.) 150km-range covered by your iGate, then there could be a lot of "useless" over-the-air-traffic if no client is around. With dynamic filtering there will be a 15km range around each lora-station applied to the IS-filter (see [Serverside Filtering](https://www.aprs-is.net/javAPRSFilter.aspx)).
  
### Attention
The above options automatically kick in, if you have: 

* the APRS-IS connectivity configured 
* a spreading-factor <= 9 (which means an LoRa-speed of faster than 1k2)
* gateing IS objects to RF enabled in the config


# coming next

* Websocket packet-display/logging of the iGate
* more useability of the time-offset --> timezones-implementation
* region-based selection of APRS-IS servers to prevent useless internet traffic by randomly selected server from rotate.aprs.net


