

// DWM1000 Libraries
#include <DW1000Ng.hpp>
#include <DW1000NgUtils.hpp>
#include <DW1000NgRanging.hpp>
#include <DW1000NgRTLS.hpp>

// connection pins
const uint8_t PIN_DWM_SS = 15;

const uint8_t PIN_IN1 = 3; // GPIO3, also RXD, switch
const uint8_t PIN_IN2 = 10; // GPIO10, also Vibration sensor, switch

const uint8_t PIN_OUT1 = 1; // GPIO1, LED 1
const uint8_t PIN_OUT2 = 9; // GPIO9, LED 2

const uint8_t PIN_ANALOG = A0;

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
	Serial.println(F("\n### UWBLS-DWM1000-RTLS-TestComponents ###"));

	// GPIO input and output setup
	pinMode(PIN_IN1, INPUT);
	pinMode(PIN_IN2, INPUT);
	pinMode(PIN_OUT1, OUTPUT);
	pinMode(PIN_OUT2, OUTPUT);
	
	#if defined(DWM1000Testing)
	// Setup the DWM1000
	setupDWM();
	#endif
}

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


void loop() {
	
	Serial.println("Starting Test");
	delay(500);
	Serial.println("LED 1 ON");
	digitalWrite(PIN_OUT1, HIGH);
	delay(500);
	digitalWrite(PIN_OUT1,LOW);
	
	Serial.println("LED 2 ON");
	digitalWrite(PIN_OUT2, HIGH);
	delay(500);
	digitalWrite(PIN_OUT2, LOW);
	
	Serial.println("Testing Button 1");
	for(int i = 0; i < 15; i++)
	{
		int Button1 = digitalRead(PIN_IN1);
		if(Button1 == 1)
			Serial.println("Button 1 PRESSED");
		else
			Serial.println("Button 1 NOT PRESSED");
		delay(250);
	}
	
	Serial.println("Testing Button 2");
	for(int i = 0; i < 15; i++)
	{
		int Button2 = digitalRead(PIN_IN2);
		if(Button2 == 1)
			Serial.println("Button 2 PRESSED");
		else
			Serial.println("Button 2 NOT PRESSED");
		delay(250);
	}
	
	int sensorValue = analogRead(PIN_ANALOG);
	float voltage = (float)sensorValue * 80.0 / 1024.0 / 5.0;
	
}