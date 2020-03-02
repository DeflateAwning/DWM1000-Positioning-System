# DWM1000-Positioning-System
DWM1000 UWB Location Positioning System for ENEL400, Winter 2020, UCalgary

## Communication Protocol (Wifi Parameters)
This section describes the parameters, sent by GET query, to the web server. Each request is sent from the associated anchor.


### Successful Ranging Request
1. Range
2. Date
	* Format: `datetime.datetime.strptime(date_time_str, '%Y-%m-%d %H:%M:%S.%f')`
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

## Dependencies
* arduino-dw1000-ng (No Modifications Yet)
* ArduinoJson (v6)
* WiFiUdp (no install required, I think)
* ~NTPClient~ (Doesn't support milliseconds)
* ezTime

## Iterative Design Notes
* Does anything work?
	* Run the DWM1000 reference code
	* Get Anchors connected to hard coded Wifi
	* Get the Anchors submitting ranging information to a server
* Improve Web Communication
	* Add NTP Connection (anchors have the time)
	* Switch GET request to JSON POST request (but only one ranging per submission)
* Add queue design
	* Multiple rangings per JSON POST request to server