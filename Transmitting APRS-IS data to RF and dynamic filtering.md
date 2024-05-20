# Transmitting APRS-IS data to RF and dynamic filtering

Being able to see at your computer at home what is going on on APRS is quite easy. With an internet-connection to your closest APRS-IS server and potentially a wisely configured filter command you will see anything you like. But things can get more comlicated, when you rely on the information that only comes from RF. Specially in the slow LoRa modes (SPF12) there could be rather fast a situation, where the iGate is permanently transmittig and there is no more time to listen for incoming traffic. __A very bad situation that def. has to be avoided.__ On the other hand, if your IGate works in the fatser 1k2 (SPF9) mode, you will have the same opportunities as you would have on 2m AFSK 1k2, at least when we focus on the on-air-time.

For the upcoming examples/usecases let's have users in mind, that have e.g. the [LoRa_APRS_Tracker](https://github.com/richonguzman/LoRa_APRS_Tracker) as a TNC for his APRS-software with them.

## High mountain, SPF12

As already mentioned, the over all on-air-time is a critical issue. With dynamic filtering set to "single" every user out there will get position packets from APRS-IS transmitted on RF within the given range (see config below). Due to limitations on the APRS-IS-Server-side a max. of 9 user-surroundings will take effect. To limit the over all on-air-time the iGate can be throttled down to transmit a max. of x-packets in y-minutes. (Server-side filters f/call/distance are in place).

## Mid range, SPF9

In the 1k2 speed domain, we are a bit more open in the  over all on-air-time question. So the throttle-values can be way less restrictive. If you see no more than 9 concurrent users on your IGate you are fine to go with the dynamic filtering  set to "single". (Server-side filters f/call/distance are in place).

## Urban, SPF9 - LoRa Hotspot at an event - Mid range with many users

The "9 user limit" can rather fast be reached, here kicks in the "group" setting in the dynamic filter setup. Users within a given distance to each other ("inrange") are combined to a "group" (or cluster) with the center of the group being the point in a p/lat/lon/dist filter that will apply towards the APRS-IS server. If there are less than "lowerlimit"-users, the iGate will process them as "single". A max of 9 such groups can be applied to the APRS-IS server. 

## Configuration

```shell
{
"mode" = "single",    #must be "single" or "group"
"testing" = "false",  #generates circle-objects 
"throttle" = {
    "packets" = "20",
    "minutes" = "15"
    },
"single" = {
    "radius" = "10" # kilometers
    }
"group" = {
    "radius" = "10", # kilometers
    "inrange" = "3", # kilometers
    "lowerlimit" = "3"
    }
}
```