#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* * * * * * * * * * * * * * * *
 * constants and settings
 */
// ports
const int RED_MOISTURE_SENSOR = A0;
const int TEMPERATURE_SENSOR = A1;
const int BLACK_MOISTURE_SENSOR = A2;
const int LIGHT_SENSOR = A3;

const int LIGHTS_RED = 5; // lights pins MUST come one after each other
const int LIGHTS_YELLOW = 6;
const int LIGHTS_GREEN = 7;

const int ULTRASONIC_TRIGGER = 9;
const int ULTRASONIC_ECHO = 10;

const int PUMP_PORT = 8;

// limits
const int RED_SENSOR_LIMITS[]         = {300, 350, 625, 675};
const int BLACK_SENSOR_LIMITS[]       = {-500, -450, -400, -350};
const int LIGHT_SENSOR_LIMITS[]       = {400, 500, 1100, 1200};
const int TEMPERATURE_SENSOR_LIMITS[] = {15, 18, 24, 27};
const float WATER_LEVEL_LIMITS[]      = {-13, -8, -3, -1};

// settings
const int SERIAL_DATA_RATE = 9600;
const int DELAY_MS_DEFAULT = 1000;
const int DELAY_MS_LONG = 2000;

// utils
const float SOUND_VELOCITY = 0.03432;

/* * * * * * * * * * * * * * * *
 * global statements and variables
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned int counter = 0;
unsigned int pumpWaitUntil = 0;

/* * * * * * * * * * * * * * * *
 * function definitions
 */
// pump
void pumpOn(){
  digitalWrite(PUMP_PORT, HIGH);
}
void pumpOff(){
  digitalWrite(PUMP_PORT, LOW);
}

// sensor value, limits --> status (too low, low, ok, high, too high)
int getStatus(int value, const int limits[]){
  if (value <= limits[0]) { return -2; }
  if (value <= limits[1]) { return -1; }
  if (value >= limits[3]) { return 2; }
  if (value >= limits[2]) { return 1; }
  return 0;
}
int getStatusFloat(float value, const float limits[]){
  if (value <= limits[0]) { return -2; }
  if (value <= limits[1]) { return -1; }
  if (value >= limits[3]) { return 2; }
  if (value >= limits[2]) { return 1; }
  return 0;
}

// LEDs
void ledOn(int pinNumber){
    digitalWrite(pinNumber, HIGH);
}
void allLedsOff(){
  digitalWrite(LIGHTS_GREEN, LOW);
  digitalWrite(LIGHTS_RED, LOW);
  digitalWrite(LIGHTS_YELLOW, LOW);
}

// by ultrasonic sensor (water level)
float measureDistance(){
		digitalWrite(ULTRASONIC_TRIGGER, LOW); delay(5); // off
		digitalWrite(ULTRASONIC_TRIGGER, HIGH);
		delay(10);
		digitalWrite(ULTRASONIC_TRIGGER, LOW); // on -> off
		float time = pulseIn(ULTRASONIC_ECHO, HIGH); // time
		float distance =  (time / 2) * SOUND_VELOCITY; // calculate distance in cm
		if (distance >= 500 || distance <= 0){
				return 0;
		}
		return distance;
}

// serial, display, lights
void outputData(String firstLine, String secondLine, int status){
		switch(status){
				case -2: ledOn(LIGHTS_RED);
				case -1: ledOn(LIGHTS_YELLOW);
				case 0: ledOn(LIGHTS_GREEN);
				case 1: ledOn(LIGHTS_YELLOW);
				case 2: ledOn(LIGHTS_RED);
		}

		lcd.clear();
		lcd.setCursor(0, 0); lcd.print(firstLine);
		lcd.setCursor(0, 1); lcd.print(secondLine);

		Serial.println(firstLine);
		Serial.println(secondLine);

		delay(DELAY_MS_LONG);
		counter += 2;

		allLedsOff();
}

// word for every status number
String statusWord(int status){
		switch(status){
				case -2: return "VZN";
				case -1: return "ZN";
				case 0:  return "OK";
				case 1:  return "ZH";
				case 2:  return "VZH";
		}
		return "INV";
}

// work with the red and black moisture sensors
int *handleMoistureSensors(){
		int statusList[2];
		int redSensorValue = analogRead(RED_MOISTURE_SENSOR);
		statusList[0] = getStatus(redSensorValue, RED_SENSOR_LIMITS);
		outputData(
				"red sensor: " + String(redSensorValue),
				"status: " + statusWord(statusList[0]),
				statusList[0]
		);

		int blackSensorValue = -analogRead(BLACK_MOISTURE_SENSOR);
		statusList[1] = getStatus(blackSensorValue, BLACK_SENSOR_LIMITS);
		outputData(
				"black sensor: " + String(blackSensorValue),
				"status: " + statusWord(statusList[1]),
				statusList[1]
		);
		
		return statusList;
}

// get data from sensors, water level and turn on pump if necessary
void pumpRoutine(){
		int *moistureSensorStatus = handleMoistureSensors();
		if (moistureSensorStatus[0] != -2 || moistureSensorStatus[1] != -2){
				outputData("genug Feuchtigkeit", "Pumpe wird nicht angemacht", 0);
				return;
		}
		float dist = measureDistance();
		int waterLevelStatus = getStatusFloat(-dist, WATER_LEVEL_LIMITS);
		if (waterLevelStatus == -2){
				outputData("ERROR", "Wasserstand VZN", 2);
				return;
		}
		if (pumpWaitUntil >= counter){
				pumpOn(); delay(DELAY_MS_LONG); pumpOff();
				counter += 2;
				pumpWaitUntil = counter + 300;
				return;
		}
		outputData("INFO", "pumpe wartet", 0);
}

/* * * * * * * * * * * * * * * *
 * setup function
 */
void setup(){
		Serial.begin(SERIAL_DATA_RATE);
		lcd.init();
		lcd.backlight();
		pinMode(LIGHTS_RED, OUTPUT);
		pinMode(LIGHTS_YELLOW, OUTPUT);
		pinMode(LIGHTS_GREEN, OUTPUT);
		pinMode(ULTRASONIC_TRIGGER, OUTPUT);
		pinMode(ULTRASONIC_ECHO, INPUT);
		pinMode(PUMP_PORT, OUTPUT);
		outputData("INFO", "setup abgeschlossen", 0);
}

/* * * * * * * * * * * * * * * *
 * loop function
 */
void loop(){
		pumpRoutine();

		int lightSensorValue = analogRead(LIGHT_SENSOR);
		int lightStatus = getStatus(lightSensorValue, LIGHT_SENSOR_LIMITS);
		outputData(
				"light sensor: " + String(lightSensorValue),
				"status: " + String(lightStatus),
				lightStatus
		);
		
		int temperatureSensorValue = map(analogRead(TEMPERATURE_SENSOR), 0, 410, -50, 150);
		int temperatureStatus = getStatus(lightSensorValue, TEMPERATURE_SENSOR_LIMITS);
		outputData(
				"temperature sensor: " + String(temperatureSensorValue),
				"status: " + String(temperatureStatus),
				temperatureStatus
		);

		delay(DELAY_MS_DEFAULT);
		counter += 1;
}

