/*************************************************************
  This example shows how to use Arduino Ethernet shield (W5100)
  to connect your project to Blynk.

  NOTE: Pins 10, 11, 12 and 13 are reserved for Ethernet module.
        DON'T use them in your sketch directly!

  WARNING: If you have an SD card, you may need to disable it
        by setting pin 4 to HIGH. Read more here:
        https://www.arduino.cc/en/Main/ArduinoEthernetShield

  This is a simple demo of sending and receiving some data.
  Be sure to check out other examples!
 *************************************************************/

// Blynk App*******************************************************************
/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID   "TMPL522N__o9-"
#define BLYNK_TEMPLATE_NAME "MatixCar Heater"
#define BLYNK_AUTH_TOKEN    "aPsLY1cMHVdemYdgPXq9OGMBg8rehlsY"
#define BLYNK_PRINT Serial
#define W5100_CS  10
#define SDCARD_CS 4
#include <BlynkSimpleEthernet.h>

/*
BlynkTimer timer;
// This function sends Arduino's up time every second to Virtual Pin (5).
void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V5, millis() / 1000);
}
*/

// Ethernet Shield*******************************************************************
#include <SPI.h>
#include <Ethernet.h>
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
char server[] = "www.google.com";    // name address for Google (using DNS) for tests

EthernetClient client;

// TempSensors*****************************************************************
#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 9 //Digital Pin
#define TEMPERATURE_PRECISION 9 //Bits
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// arrays to hold device addresses
DeviceAddress TempSensorInsideFront, TempSensorInsideBack, TempSensorOutside;

//VOLTSENSOR*******************************************************************
const int analogInputPin = A7;
const float R1 = 30000.0;
const float R2 = 7500.0;
const float TriggerVolt = 13;
float VoltSensor = 0.0;
float vArduino = 0.0;
float vActual = 0.0;
int rawValueRead= 0;

// RELAYS def******************************************************************
const int consumer_power =  8;
const int heaterpower    =  7;
const int button_set     =  6;
const int button_ok      =  5;
const int button_down    =  4;
const int button_up      =  3;
const int button_power   =  2;

// VARS************************************************************************
bool blynk_started            = false;
int app_heatervalue           = 0;
int heater_stepvalue          = 0;
float temp_inside_front       = 0;
float temp_inside_back        = 0;
float temp_outside            = 0;
unsigned long previousMillis  = 0;        // Stores the time when the event was last triggered
unsigned long heaterStartTime = 0;        // Variable to store the time once heater got started
unsigned long heaterStopTime  = 0;        // Variable to store the time once stop impulse has sent
const long temp_interval      = 600000;   // 10 minutes in milliseconds
const long sensor_interval    = 10000;    // Interval in milliseconds (10 seconds)
const long cooldownDuration   = 600000;   // 10 min needed to cooldown the heater
bool heater_cooldown          = false;
bool heater_active            = false;

// SETUP************************************************************************
void setup()
{
  Serial.begin(9600);

  // TEMP
  sensors.begin(); // Start up the library
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  if (!sensors.getAddress(TempSensorInsideFront, 0)) Serial.println("Unable to find address for TempSensor Inside Front");
  if (!sensors.getAddress(TempSensorInsideBack, 1)) Serial.println("Unable to find address for TempSensor Inside Back");
  if (!sensors.getAddress(TempSensorOutside, 2)) Serial.println("Unable to find address for TempSensor Outside");

  // Set the resolution to 9 bit per device
  sensors.setResolution(TempSensorInsideFront, TEMPERATURE_PRECISION);
  sensors.setResolution(TempSensorInsideBack, TEMPERATURE_PRECISION);
  sensors.setResolution(TempSensorOutside, TEMPERATURE_PRECISION);

  // Relais
  pinMode(consumer_power, OUTPUT);
  pinMode(heaterpower, OUTPUT);
  pinMode(button_set, OUTPUT);
  pinMode(button_ok, OUTPUT);
  pinMode(button_up, OUTPUT);
  pinMode(button_down, OUTPUT);
  pinMode(button_power, OUTPUT);
  digitalWrite(consumer_power, HIGH);
  digitalWrite(heaterpower, HIGH);
  digitalWrite(button_set, HIGH);
  digitalWrite(button_ok, HIGH);
  digitalWrite(button_up, HIGH);
  digitalWrite(button_down, HIGH);
  digitalWrite(button_power, HIGH);

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  
  // Blynk
  pinMode(SDCARD_CS, OUTPUT); 
  digitalWrite(SDCARD_CS, HIGH);            // Deselect the SD card
}

// *******************************************************************************

void BlynkLoop()
{
  //Check if connected with Blynk
  //Programm should work also without Blynk
  if (!Blynk.connected() && client.connect(server, 80))
  {
    client.stop();                                          //Internet is connected
    Serial.println("Internet ok");
    if (!blynk_started)
    {
      Blynk.config(BLYNK_AUTH_TOKEN);
      blynk_started = true;
    }
    Serial.println("Connecting to cloud!");
    Blynk.connect();
    //sync current values
    Blynk.virtualWrite(V0, app_heatervalue);                //Heater switch
    Blynk.virtualWrite(V1, heater_stepvalue);               //Heater steps
    Blynk.virtualWrite(V2, temp_inside_front);
    Blynk.virtualWrite(V3, temp_inside_back);
    Blynk.virtualWrite(V4, temp_outside);
  }
  else if (Blynk.connected())
  {
    //Blynk.syncAll();                         //optional for sync
    Blynk.run(); //runs all Blynk stuff
    //timer.run(); //runs Blynk timer
  }
}

float readVoltage()
{
  rawValueRead = analogRead(analogInputPin);
  vArduino = (rawValueRead * 5.0) / 1024.0;
  vActual = vArduino / (R2/(R1+R2));
  return vActual;
}

void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
    // Serial.print("Temp °C: ");
    // Serial.print(tempC);
    // Serial.println("");
  if(deviceAddress == TempSensorInsideFront)
  {
    temp_inside_front = tempC;
  }
  if(deviceAddress == TempSensorInsideBack)
  {
    temp_inside_back = tempC;
  }
  if(deviceAddress == TempSensorOutside)
  {
    temp_outside = tempC;
  }
}

void printTempData(DeviceAddress deviceAddress)
{
  printTemperature(deviceAddress);
}

// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0) //Heater ON/OFF
{
  // Set incoming value from pin V0 to a variable
  app_heatervalue = param.asInt();
  // Update state in app
  if(app_heatervalue == 1 && !heater_active)
  {
    Serial.println("APP: Start heating");
    if(digitalRead(heaterpower) == HIGH) heater_powerON(); //Check if heater have power and connect it.
    heater_start(); 
  }
  else if(app_heatervalue == 0 && heater_active) heater_stop();

}

BLYNK_WRITE(V1) //Get Heater steps
{
  int app_stepvalue = param.asInt();
  if (app_stepvalue > heater_stepvalue) heater_stepup();
  if (app_stepvalue < heater_stepvalue) heater_stepdown();
}

void heater_start()
{
  digitalWrite(button_power, LOW);
  delay(2500);
  digitalWrite(button_power, HIGH);
  heaterStartTime = millis();
  heater_active = true;
  Blynk.virtualWrite(V0, 1);
  Serial.println("Heater Start");
}

void heater_stop()
{
  digitalWrite(button_power, LOW);
  delay(2500);
  digitalWrite(button_power, HIGH);
  heater_cooldown = true;
  heaterStopTime = millis();
  heaterStartTime = 0;
  heater_active = false;
  Serial.println("Heater Stop / Cooldown");
}

void heater_cooldown_check()
{
  if (heater_cooldown && !heater_active)
  {
    // Check if cooldown duration has been reached
    if ((millis() - heaterStopTime) >= cooldownDuration) 
    {
      heater_cooldown = false;
      Serial.println("Cooldown finish!"); 
    }
  }
}

void heater_stepup()
{
  digitalWrite(button_up, LOW);
  delay(300);
  digitalWrite(button_up, HIGH);
  //check max value
  if (heater_stepvalue == 6) heater_stepvalue = 1;
  else heater_stepvalue++;
  Serial.print("Heater step: ");
  Serial.println(heater_stepvalue);
  Blynk.virtualWrite(V1, heater_stepvalue);
}

void heater_stepdown() {
  digitalWrite(button_down, LOW);
  delay(300);
  digitalWrite(button_down, HIGH);
  if(heater_stepvalue == 1) //Check min value
  {
    heater_stepvalue = 6; //Chan
  }
  else
  {
    heater_stepvalue--;
  }
  Serial.print("Heater step: ");
  Serial.println(heater_stepvalue);
  Blynk.virtualWrite(V1, heater_stepvalue);
}

void heater_powerON() {
  //Connect heater to power and set mountain mode (less fuel consumption)
  digitalWrite(heaterpower, LOW);
  Serial.println("Heater PowerON");
  heater_stepvalue = 3;
  delay(4000); //delay to start to heaterprogramm
  digitalWrite(button_set, LOW);
  digitalWrite(button_ok, LOW);
  delay(2500); 
  digitalWrite(button_set, HIGH);
  digitalWrite(button_ok, HIGH);
  Blynk.virtualWrite(V1, heater_stepvalue);
}

void heater_powerOFF()
{
  if (!heater_cooldown) //Never poweroff during cooldown
  {
    digitalWrite(heaterpower, HIGH);
    Serial.println("Heater PowerOFF");
    heater_stepvalue = 0;
    Blynk.virtualWrite(V0, heater_stepvalue);
  }
}

void loop()
{
  //Trigger consumer relay if Voltage has reached (Batterie get charged by motor or photovoltaik)
  if ((readVoltage() >= TriggerVolt) && digitalRead(consumer_power) == HIGH)
  {
    delay(3000); //wait 3 sec to make sure that voltage is stable
    if (readVoltage() >= TriggerVolt)
    {
      digitalWrite(consumer_power, LOW);
      Serial.println("Consumer relay ON!");

    }
    else Serial.println("Volt unstable: Relay not triggert!");
  }
  //Turn off if Voltage is too low
  if ((readVoltage() < TriggerVolt) && digitalRead(consumer_power) == LOW)
  {
    delay(3000);
    if (readVoltage() < TriggerVolt)
    {
      digitalWrite(consumer_power, HIGH);
      Serial.println("Consumer relay OFF!");
    }
  }

  //secruity function to check if the heater is in the cooldown process
  heater_cooldown_check();

  // TEMP sensors
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures();
  printTempData(TempSensorInsideFront);
  printTempData(TempSensorInsideBack);
  printTempData(TempSensorOutside);
  //Send temp data to Cloud
  if ((millis() - previousMillis) >= sensor_interval)
  {
    Blynk.virtualWrite(V2, temp_inside_front);
    Blynk.virtualWrite(V3, temp_inside_back);
    Blynk.virtualWrite(V4, temp_outside);
    previousMillis = millis(); //safe time for next event
  }

  // HEATER Conditions
  //Connect heater to powersupply if outside temp is below 10°C
  if (digitalRead(heaterpower) == HIGH && temp_outside < 10)
  {
    heater_powerON();
  }
  else if (digitalRead(heaterpower) == LOW && temp_outside >= 10 && !heater_active && !heater_cooldown)
  {
    heater_powerOFF();
  }

  //Roomcontrol heating start heating below 16°C or stop above 17°C
  if (app_heatervalue == 1)
  {
    if ((temp_inside_front < 16 && temp_inside_back < 15) && !heater_active)
    {
      Serial.println("Room too cold!");
      heater_start();

    }
    if ((temp_inside_front >= 17 || temp_inside_back >= 16) && heater_active)
    {
      // Check if the time elapsed since the temperature reached is more than X minutes
      if ((millis() - heaterStartTime) > temp_interval)
      {
        Serial.println("Room Temp reached!");
        heater_stop();  
      }
    }
  }

  //Check Blynk connection and send data
  BlynkLoop();
}
