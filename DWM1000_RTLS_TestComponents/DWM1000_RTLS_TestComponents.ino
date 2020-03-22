// DWM1000_RTLS_TestComponents.ino

// This program tests all components on the board


// Optional: Define Testing
//#define ENABLE_DWM1000_TESTING 1
#define ENABLE_PIN_COPY 1
#define ENABLE_SWITCH_TESTING 1 // always leave on
//#define ENABLE_SERIAL 1


// Currently Working: ENABLE_SWITCH_TESTING+ENABLE_SERIAL
// Currently Working: ENABLE_SWITCH_TESTING+ENABLE_PIN_COPY for Out1 only

// Disable Wifi
#include "ESP8266WiFi.h"
/*
extern "C" {
	#include "user_interface.h"
}
*/

// DWM1000 Libraries
#ifdef ENABLE_DWM1000_TESTING
#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>
#include <DW1000NgRTLS.hpp>
#endif

// connection pins
const uint8_t PIN_DWM_SS = 15;

const uint8_t PIN_IN0 = 0; // GPIO0, FLASH switch
const uint8_t PIN_IN1 = 3; // GPIO3/RXD, switch
const uint8_t PIN_IN2 = 10; // GPIO10, also Vibration sensor, switch

const uint8_t PIN_BUILTIN_LED = 2; // or LED_BUILTIN macro
const uint8_t PIN_OUT1 = 1; // GPIO1/TXD, LED 1
const uint8_t PIN_OUT2 = 9; // GPIO9, LED 2 (Requires DIO) - BREAKS EVERYTHING, LEAVE COMMENTED

const uint8_t PIN_ANALOG = A0;

#ifdef ENABLE_DWM1000_TESTING
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
#endif

void setup() {
	// Init Debug
	#ifdef ENABLE_SERIAL
	//Serial.begin(115200);
	Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
	Serial.println("\n### UWBLS-DWM1000-RTLS-TestComponents ###");
	#endif

	// Disable Wifi
	WiFi.forceSleepBegin();
	delay(1);

	// Set the Watchdog
	//ESP.wdtEnable(12000);

	// GPIO input and output setup
	#ifdef ENABLE_SWITCH_TESTING
	pinMode(PIN_IN0, INPUT_PULLUP);
	pinMode(PIN_IN1, INPUT_PULLUP);
	pinMode(PIN_IN2, INPUT_PULLUP);
	#endif

	#ifdef ENABLE_PIN_COPY
	pinMode(PIN_OUT1, OUTPUT);
	//pinMode(PIN_OUT2, OUTPUT);
	#endif
	pinMode(PIN_BUILTIN_LED, OUTPUT);

	#ifdef ENABLE_DWM1000_TESTING
	// Setup the DWM1000
	setupDWM();
	#endif

}

#ifdef ENABLE_DWM1000_TESTING
void setupDWM() {
  
	// initialize the driver
	DW1000Ng::initializeNoInterrupt(PIN_DWM_SS);
	
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
#endif

void loop() {
	// Call Main Sensor/Output Test
	runSingleTestCycle();

	// Reset the Watchdog (must be done minimum every 3.2s)
	//feedWatchdog();
}

long startTime = 0;

void runSingleTestCycle() {
	#ifdef ENABLE_SERIAL
	Serial.println("Starting Test Cycle");
	#endif

	// Do Pin Test, including output testing and input testing
	doPinTest();
	
	doAnalogVoltageTest();

	doSwitchTest();
}

void doAnalogVoltageTest() {
	// Read analog values
	int sensorValue = analogRead(PIN_ANALOG);
	float voltage = (float)sensorValue * 80.0 / 1024.0 / 5.1;
	
	// Print the analog values, if enabled
	#ifdef ENABLE_SERIAL
	Serial.print("# Battery Level: "); Serial.print(sensorValue); Serial.print("/1024, or "); Serial.print(voltage); Serial.print(" volts");
	Serial.println();
	#endif
}

void doPinTest() {

	// Always do the built in LED flash
	#ifdef ENABLE_SERIAL
	Serial.println("# Built In LED ON/OFF Test");
	#endif

	digitalWrite(PIN_BUILTIN_LED, HIGH);
	#ifdef ENABLE_PIN_COPY
	digitalWrite(PIN_OUT1, HIGH);
	//digitalWrite(PIN_OUT2, HIGH);
	#endif
	delay(1000);

	digitalWrite(PIN_BUILTIN_LED, LOW);
	#ifdef ENABLE_PIN_COPY
	digitalWrite(PIN_OUT1, LOW);
	//digitalWrite(PIN_OUT2, LOW);
	#endif
	delay(1000);

}

void doSwitchTest() {
	startTime = millis();
	while (millis() - startTime < 5000) {
		#ifdef ENABLE_PIN_COPY
		digitalWrite(PIN_BUILTIN_LED, digitalRead(PIN_IN2));
		digitalWrite(PIN_OUT1, digitalRead(PIN_IN1));
		#endif

		#ifdef ENABLE_SERIAL
		Serial.print("Button States (0/1/2): "); Serial.print(digitalRead(PIN_IN0)); Serial.print("/"); Serial.print(digitalRead(PIN_IN1)); Serial.print("/"); Serial.print(digitalRead(PIN_IN2));
		Serial.println();
		#endif

		delay(100);
	}
}


