/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* 
 * StandardRTLSAnchorMain_TWR.ino
 * 
 * This is an example master anchor in a RTLS using two way ranging ISO/IEC 24730-62_2013 messages
 */

// ESP8266 Libraries (Hard Coded)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
 
 /*
// ESP8266 Libraries (Using AutoConnect)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WEBServer;
#include <FS.h>
#include <AutoConnect.h>
// For ESP32 Architecture (?)
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
 
 
#include <PageBuilder.h>*/

 #include <ArduinoJson.h>
 
 
// DWM1000 Libraries
#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>
#include <DW1000NgRTLS.hpp>

// NTP Client
#include <ezTime.h>

// Wifi Configuration Options
const char* ssid = "UWBLS Net"; // Will's Settings
const char* password = "123456789"; // Will's Settings


// Wifi Sending Options
const char* serverAddress = "http://enqb0w8a2ni1j.x.pipedream.net";
#define JSON_MESSAGE_BUFFER_LENGTH 1024
unsigned int thisAnchorNumber = 1;
const bool printDebugMessages = true;
const long utcOffsetInSeconds = 0;
const char* dateFormat = "Y-m-d H:i:s.v";

// Python Datetime Parsing
//date_time_str = '2018-06-29 08:15:27.243860'
//date_time_obj = datetime.datetime.strptime(date_time_str, '%Y-%m-%d %H:%M:%S.%f')

unsigned long startTime; // Keeping time for periodic JSON requests to web server.

StaticJsonDocument<JSON_MESSAGE_BUFFER_LENGTH> JSONdoc;


typedef struct Position {
	double x;
	double y;
} Position;

// connection pins
#if defined(ESP8266)
const uint8_t PIN_SS = 15;
#else
const uint8_t PIN_RST = 9;
const uint8_t PIN_SS = SS; // spi select pin
#endif


Position position_self = {0,0};
Position position_B = {3,0};
Position position_C = {3,2.5};

double range_self;
double range_B;
double range_C;

boolean received_B = false;

byte target_eui[8];
byte tag_shortAddress[] = {0x05, 0x00};

byte anchor_b[] = {0x02, 0x00};
uint16_t next_anchor = 2;
byte anchor_c[] = {0x03, 0x00};

device_configuration_t DEFAULT_CONFIG = {
	false,
	true,
	true,
	true,
	false,
	SFDMode::STANDARD_SFD,
	Channel::CHANNEL_5,
	DataRate::RATE_850KBPS,
	PulseFrequency::FREQ_16MHZ,
	PreambleLength::LEN_256,
	PreambleCode::CODE_3
};

frame_filtering_configuration_t ANCHOR_FRAME_FILTER_CONFIG = {
	false,
	false,
	true,
	false,
	false,
	false,
	false,
	true /* This allows blink frames */
};

void setup() {
	// Init Debug
	Serial.begin(115200);
	Serial.println(F("\n### UWBLS-DWM1000-RTLS-AnchorMain ###"));

	// Setup the Wifi Connection
	setupWifi();
	
	// Setup the DWM1000
	setupDWM();
	
	startTime = millis();

}

void setupWifi() {
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		Serial.println("Connecting...");
	}

	Serial.print("\nConnected, IP Address: ");
	Serial.println(WiFi.localIP());

	initNTPTime();

	Serial.print("NTP Time Updated to: ");
	Serial.println(UTC.dateTime(dateFormat));
}

// Inits the ezTime NTP client, which updates itself automatically
void initNTPTime() {
	waitForSync();

	// Optionally set update interval and server (defaults to ntp.pool.org, 30 minutes)
	setInterval(15*60); // number of seconds to update (default: 30*60)
	//setServer(); // char* of the server name (default: "ntp.pool.org")

}


void setupDWM() {
  
	// initialize the driver
	#if defined(ESP8266)
	DW1000Ng::initializeNoInterrupt(PIN_SS);
	#else
	DW1000Ng::initializeNoInterrupt(PIN_SS, PIN_RST);
	#endif
	Serial.println(F("DW1000Ng initialized ..."));
	// general configuration
	DW1000Ng::applyConfiguration(DEFAULT_CONFIG);
	DW1000Ng::enableFrameFiltering(ANCHOR_FRAME_FILTER_CONFIG);
	
	DW1000Ng::setEUI("AA:BB:CC:DD:EE:FF:00:01");

	DW1000Ng::setPreambleDetectionTimeout(64);
	DW1000Ng::setSfdDetectionTimeout(273);
	DW1000Ng::setReceiveFrameWaitTimeoutPeriod(5000);

	DW1000Ng::setNetworkId(RTLS_APP_ID);
	DW1000Ng::setDeviceAddress(1);
  
	DW1000Ng::setAntennaDelay(16436);
	
	Serial.println(F("Committed configuration ..."));
	// DEBUG chip info and registers pretty printed
	char msg[128];
	DW1000Ng::getPrintableDeviceIdentifier(msg);
	Serial.print("Device ID: "); Serial.println(msg);
	DW1000Ng::getPrintableExtendedUniqueIdentifier(msg);
	Serial.print("Unique ID: "); Serial.println(msg);
	DW1000Ng::getPrintableNetworkIdAndShortAddress(msg);
	Serial.print("Network ID & Device Address: "); Serial.println(msg);
	DW1000Ng::getPrintableDeviceMode(msg);
	Serial.print("Device mode: "); Serial.println(msg);    
}

/* using https://math.stackexchange.com/questions/884807/find-x-location-using-3-known-x-y-location-using-trilateration */
void calculatePosition(double &x, double &y) {

	/* This gives for granted that the z plane is the same for anchor and tags */
	double A = ( (-2*position_self.x) + (2*position_B.x) );
	double B = ( (-2*position_self.y) + (2*position_B.y) );
	double C = (range_self*range_self) - (range_B*range_B) - (position_self.x*position_self.x) + (position_B.x*position_B.x) - (position_self.y*position_self.y) + (position_B.y*position_B.y);
	double D = ( (-2*position_B.x) + (2*position_C.x) );
	double E = ( (-2*position_B.y) + (2*position_C.y) );
	double F = (range_B*range_B) - (range_C*range_C) - (position_B.x*position_B.x) + (position_C.x*position_C.x) - (position_B.y*position_B.y) + (position_C.y*position_C.y);

	x = (C*E-F*B) / (E*A-B*D);
	y = (C*D-A*F) / (B*D-A*E);
}

// Project's SUPER FUCKING LOOP
void loop() {
	// Run the DWM1000 Check for Ranging, do appropriate response to Wifi
	loopDWM();

	
	
	if(millis() - startTime > 5000)
	{
		startTime = millis();
		char JSONmessageBuffer[JSON_MESSAGE_BUFFER_LENGTH];
		serializeJson(JSONdoc, JSONmessageBuffer, sizeof(JSONmessageBuffer));
		Serial.println(JSONmessageBuffer);
		
		makeWifiRequestJSON(JSONmessageBuffer);
		JSONdoc.clear();
	}
	
	// Run the Test Wifi System (to a RequestBin server)
	//testWifiRequestBin();

	//delay(10000);
	
}

// Basic test to a RequestBin server
void testWifiRequestBin() {
	Serial.println("Beginning Request Send");

	// Check if Connected
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Not connected to WiFi.");
		return;
	}

	// Make the http client, send request
	HTTPClient http;
	http.begin(serverAddress); //  if this fails, try http instead of https
	int httpCode = http.GET();

	// Check the response
	if (httpCode > 0) {
		String payload = http.getString();
		Serial.println(payload);
	}
	else {
		Serial.print("Got httpCode!=0, httpCode=");
		Serial.println(httpCode);
	}
	http.end();
}

// Calls the main server with the query string
void makeWifiRequestGET(String queryString) {
	if (printDebugMessages) Serial.println("Beginning Request Send");

	// Check if Connected
	if (WiFi.status() != WL_CONNECTED) {
		// TODO Show an error LED, very bad
		Serial.println("Not connected to WiFi.");
		return;
	}

	// Prepare the request string
	String requestString = serverAddress;
	requestString += "?";
	requestString += queryString;
	if (printDebugMessages) Serial.println(requestString);

	// Make the http client, send request
	HTTPClient http;
	http.begin(requestString); //  if this fails, try http instead of https
	int httpCode = http.GET();

	// Check the response
	if (httpCode > 0) {
		String payload = http.getString();
		Serial.println(payload);
	}
	else {
		Serial.print("Got httpCode!=0, httpCode=");
		Serial.println(httpCode);
	}
	http.end();

}

// Calls the main server with the json query string
void makeWifiRequestJSON(const char* jsonStr) {
	HTTPClient http; //Declare object of class HTTPClient
	http.begin(serverAddress); //Specify request destination
	http.addHeader("Content-Type", "application/json"); //Specify content-type header

	int httpCode = http.POST(jsonStr); //Send the request
	String payload = http.getString(); //Get the response payload
	if (httpCode > 0) {
		if (printDebugMessages) Serial.print("Successful JSON POST, httpCode="); Serial.println(httpCode);
		if (printDebugMessages) Serial.println(payload); //Print request response payload
	}
	else {
		if (printDebugMessages) Serial.print("Failed JSON POST, httpCode="); Serial.println(httpCode);
		if (printDebugMessages) Serial.println(payload); //Print request response payload (maybe not present for fails)
	}
	http.end(); //Close connection

}



void loopDWM() {
	// Literal Duplication of F-Army code
	if(DW1000NgRTLS::receiveFrame()) {
		size_t recv_len = DW1000Ng::getReceivedDataLength();
		byte recv_data[recv_len];
		DW1000Ng::getReceivedData(recv_data, recv_len);


		if(recv_data[0] == BLINK) {
			// Received an "I want to start ranging" from a tag
			DW1000NgRTLS::transmitRangingInitiation(&recv_data[2], tag_shortAddress);
			DW1000NgRTLS::waitForTransmission();

			RangeAcceptResult result = DW1000NgRTLS::anchorRangeAccept(NextActivity::RANGING_CONFIRM, next_anchor);
			if(!result.success) {
				// TODO Blink an error LED or something
				// TODO send a wifi request, probably
				Serial.println("anchorRangeAccept failed (result.success = false)");
				return;
			}

			// result contains .success, .range

			// Make basic GET request query string
			String rangeString = "Range: "; rangeString += result.range; rangeString += " m";
			rangeString += "\t RX power: "; rangeString += DW1000Ng::getReceivePower(); rangeString += " dBm";

			String queryString;
			queryString = "Range=";
			queryString += result.range;

			queryString += "&Date="; // TODO implement some anchor-side timing system
			queryString += UTC.dateTime(dateFormat);

			queryString += "&AnchorNumber=";
			queryString += thisAnchorNumber;

			queryString += "&Success=";
			queryString += (int)result.success;

			queryString += "&ReceivePower=";
			queryString += DW1000Ng::getReceivePower();

			//Serial.println(rangeString);
			//makeWifiRequestGET(queryString);


			// Make JSON request 
			// Tutorial w/Python: https://techtutorialsx.com/2017/01/08/esp8266-posting-json-data-to-a-flask-server-on-the-cloud/)
			// Tutorial for v6!!: https://arduinojson.org/v6/example/generator/
			
			//StaticJsonDocument<JSON_MESSAGE_BUFFER_LENGTH> JSONdoc;
			
			JSONdoc["Range"] = result.range;
			JSONdoc["Date"] = UTC.dateTime(dateFormat);
			JSONdoc["AnchorNumber"] = thisAnchorNumber;
			JSONdoc["Success"] = result.success;
			JSONdoc["ReceivePower"] = DW1000Ng::getReceivePower();	
			
			/*
			char JSONmessageBuffer[JSON_MESSAGE_BUFFER_LENGTH];
			serializeJson(JSONdoc, JSONmessageBuffer, sizeof(JSONmessageBuffer));
			Serial.println(JSONmessageBuffer);

			makeWifiRequestJSON(JSONmessageBuffer);
			*/
		} 

 // This chunk is for receiving a range from another non-main anchor
		/*
		else if(recv_data[9] == 0x60) {
			double range = static_cast<double>(DW1000NgUtils::bytesAsValue(&recv_data[10],2) / 1000.0);
			String rangeReportString = "Range from: "; rangeReportString += recv_data[7];
			rangeReportString += " = "; rangeReportString += range;
			Serial.println(rangeReportString);
			if(received_B == false && recv_data[7] == anchor_b[0] && recv_data[8] == anchor_b[1]) {
				range_B = range;
				received_B = true;
			} else if(received_B == true && recv_data[7] == anchor_c[0] && recv_data[8] == anchor_c[1]){
				range_C = range;
				double x,y;
				calculatePosition(x,y);
				String positioning = "Found position - x: ";
				positioning += x; positioning +=" y: ";
				positioning += y;
				Serial.println(positioning);
				received_B = false;
			} else {
				received_B = false;
			}
		}
		*/
		
	}

	
}