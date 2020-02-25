# DWM1000-Positioning-System
DWM1000 UWB Location Positioning System for ENEL400, Winter 2020, UCalgary

## Communication Protocol (Wifi Parameters)
This section describes the parameters, sent by GET query, to the web server. Each request is sent from the associated anchor.


### Successful Ranging Request
1. Range
2. Time (not implemented)
3. AnchorNumber (needs better implementation)
4. Success
	* Value of 0 for fail, 1 for success
5. ReceivePower

## Major To Dos
1. Implement anchor-side timing system, maybe with NTP
2. Anchor numbering system, with either P2P or a configuration or something better
3. Set all Wifi credentials from main anchor by DWM communication
4. Remove hardcoding of Wifi credentials
5. Add division of bands
6. Consider adding a watchdog

## Next To Do
* Fix issue with sending query string to server (it's giving httpCode = -1)