#include <SPI.h>
#include <SD.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "Sensor.h"

Sensor sensor;

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// for sd card
const int chipSelect = 53; 

// relay pins
const int relayPinPump  = 41;
const int relayPinA     = 42;
const int relayPinB     = 43;
const int relayPinPlus  = 44;
const int relayPinMinus = 45;

unsigned long relayALastOpenTime      = 0;
unsigned long relayBLastOpenTime      = 0;
unsigned long relayPlusLastOpenTime   = 0;
unsigned long relayMinusLastOpenTime  = 0;
bool relayAOpen     = false;
bool relayBOpen     = false;
bool relayPlusOpen  = false;
bool relayMinusOpen = false;

// variables
float ec = 0.00;
float ph = 0.00;
float wl = 0.00;
float wf = 0.00;
float tp = 0.00;
float hm = 0.00;
float co = 0.00;

String ecUnit = " mS/cm";
String phUnit = " pH";
String wlUnit = " %";
String wfUnit = " L/m";
String tpUnit = " C";
String hmUnit = " %";
String coUnit = " ppm";
String floatString = "";

// heating variables
float timeHeatingMin = 2;
int timeHeatingSec = int(timeHeatingMin*60);
float heatingPercentage = 0.00;

// loop variables
float timeStoreDataMin    = 30;
int timeStoreDataSec      = 0;
int timePcdDisplayDataSec = 5;

// EC Pump Variables 
bool ecReadyForSolutionB = false;
float ecThresholdMin = 1.4;
float ecThresholdMax = 1.8;
int ecPumpTimeSecondsSolutionA = 40;
int ecPumpTimeSecondsSolutionB = 40;
float ecPumpAtoBOpenTriggerDelayMin = 0.133; // 20s
float ecPumpBtoAOpenTriggerDelayMin = 0.25; // 15s

// PH pump Variables 
float phThresholdMin = 6.0;
float phThresholdMax = 7.0;
int phPumpTimeSecondsSolutionPlus   = 1.25;
int phPumpTimeSecondsSolutionMinus  = 1.25;

volatile int Sensor::wfPulseCount = 0;

void setup()
{

  Serial.begin(9600);
  sensor.begin();

  // initialize the LCD
	lcd.begin();
	lcd.backlight();

  pinMode(relayPinPump, OUTPUT);
  pinMode(relayPinA, OUTPUT);
  pinMode(relayPinB, OUTPUT);
  pinMode(relayPinPlus, OUTPUT);
  pinMode(relayPinMinus, OUTPUT);

  // default relays as off
  digitalWrite(relayPinPump, HIGH);
  digitalWrite(relayPinA, HIGH);
  digitalWrite(relayPinB, HIGH);
  digitalWrite(relayPinPlus, HIGH);
  digitalWrite(relayPinMinus, HIGH);

  digitalWrite(relayPinPump, LOW);

	// title
  lcd.clear();
  lcd.setCursor(2, 0);
	lcd.print("HYDROPONICS");

  // check if SD card Exist
  if (!SD.begin(chipSelect)) {
    _sdCardNotFound();
  }
  Serial.println("SD card initialized.");

  // Check if the file exists
  if (!SD.exists("data.csv")) {
    File dataFile = SD.open("data.csv", FILE_WRITE);
  }

  // Open the file
  File file = SD.open("data.csv");
  if (!file) {
    Serial.println("Error opening file.");
    return;
  }

  String header = file.readStringUntil('\n');
  file.close();
  
  // Check if the first column of the first row is "Time"
  if (header.substring(0, 4) != "Time") {
    File dataFile = SD.open("data.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print("Time");
      dataFile.print(",");
      dataFile.print("Electrical Conductivity(mS/cm)");
      dataFile.print(",");
      dataFile.print("pH");
      dataFile.print(",");
      dataFile.print("Water Level(%)");
      dataFile.print(",");
      dataFile.print("Water Flow(L/m)");
      dataFile.print(",");
      dataFile.print("Temperature(C)");
      dataFile.print(",");
      dataFile.print("Humidity(%)");
      dataFile.print(",");
      dataFile.println("Carbon Dioxide(ppm)");
      dataFile.close();
    }
  }

  delay(2000);
  lcd.clear();
  lcd.print("SYSTEM HEATING");
  lcd.setCursor(0, 1);
  lcd.print("0.00%");

  // show heating percentage
  for(int i=1; i<=timeHeatingSec; i++) {
    delay(1000);
    heatingPercentage = float(i)/timeHeatingSec*100;
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(heatingPercentage);
    lcd.print("%");
  }
  delay(1000);
}

void loop()
{
  
  int count = 0;
  for (int i=0; i<int(timeStoreDataMin*60); i++) {

    // peristaltic pumps triggers
    _triggerRelayA(false);
    _triggerRelayB();
    _triggerRelayPlus(false); 
    _triggerRelayMinus(false); 

    if ((i+1) == 1) {

      // EC
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting EC Data");
      ec = sensor.readEC();
      if (ec < ecThresholdMin) _triggerRelayA(true); 
      delay(500);

      // PH
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting PH Data");
      ph = sensor.readPH();
      if (ph < phThresholdMin) _triggerRelayPlus(true); 
      if (ph > phThresholdMax) _triggerRelayMinus(true); 
      delay(500);

    } else if ((i+1) == 2) {

      // WL
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting WL Data");
      wl = sensor.readWL();
      delay(500);
      // WF
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting WF Data");
      wf = sensor.readWF(500);
      delay(500);

    } else if ((i+1) == 3) {

      // TP
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting TP Data");
      tp = sensor.readTP();
      delay(500);

      // HM
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting HM Data");
      hm = sensor.readHM();
      delay(500);

    } else if ((i+1) == 4) {
      // CO
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Getting CO Data");
      co = sensor.readCO();
      delay(500);
      // Saving 
      if (!SD.begin(chipSelect)) _sdCardNotFound();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Saving data...");

      File dataFile = SD.open("data.csv", FILE_WRITE);
      if (dataFile) {
        dataFile.print(millis());
        dataFile.print(",");
        dataFile.print(ec);
        dataFile.print(",");
        dataFile.print(ph);
        dataFile.print(",");
        dataFile.print(wl);
        dataFile.print(",");
        dataFile.print(wf);
        dataFile.print(",");
        dataFile.print(tp);
        dataFile.print(",");
        dataFile.print(hm);
        dataFile.print(",");
        dataFile.println(co);
        dataFile.close();
      }

      Serial.print("EC: ");
      Serial.print(ec);
      Serial.println("    ");
      
      Serial.print("PH: ");
      Serial.print(ph);
      Serial.println("    ");

      Serial.print("WL: ");
      Serial.print(wl);
      Serial.println("    ");

      Serial.print("WF: ");
      Serial.print(wf);
      Serial.println("    ");

      Serial.print("TP: ");
      Serial.print(tp);
      Serial.println("    ");

      Serial.print("HM: ");
      Serial.print(hm);
      Serial.println("    ");

      Serial.print("CO: ");
      Serial.print(co);
      Serial.println("    ");

      delay(500);
    } else {
      if ((i+1)%timePcdDisplayDataSec == 0) {
        count++;
        lcd.clear();

        // EC and PH
        if (count == 1) {
          // EC
          lcd.setCursor(0, 0);
          lcd.print("EC:");
          floatString = String(ec);
          lcd.setCursor((16-floatString.length()-ecUnit.length()), 0);
          lcd.print(ec);
          lcd.print(ecUnit);

          // PH
          lcd.setCursor(0, 1);
          lcd.print("PH:");
          floatString = String(ph);
          lcd.setCursor((16-floatString.length()-phUnit.length()), 1);
          lcd.print(ph);
          lcd.print(phUnit);
        }

        // WL and WF
        if (count == 2) {
          // WL
          lcd.setCursor(0, 0);
          lcd.print("WL:");
          floatString = String(wl);
          lcd.setCursor((16-floatString.length()-wlUnit.length()), 0);
          lcd.print(wl);
          lcd.print(wlUnit);

          // WF
          lcd.setCursor(0, 1);
          lcd.print("WF:");
          floatString = String(wf);
          lcd.setCursor((16-floatString.length()-wfUnit.length()), 1);
          lcd.print(wf);
          lcd.print(wfUnit);
        }

        // TP and HM
        if (count == 3) {
          // TP
          lcd.setCursor(0, 0);
          lcd.print("TP:");
          floatString = String(tp);
          lcd.setCursor((16-floatString.length()-tpUnit.length()), 0);
          lcd.print(tp);
          lcd.print(tpUnit);

          // HM
          lcd.setCursor(0, 1);
          lcd.print("HM:");
          floatString = String(hm);
          lcd.setCursor((16-floatString.length()-hmUnit.length()), 1);
          lcd.print(hm);
          lcd.print(hmUnit);
        }

        // CO
        if (count == 4) {
          // CO
          lcd.setCursor(0, 0);
          lcd.print("CO:");
          floatString = String(co);
          lcd.setCursor((16-floatString.length()-coUnit.length()), 0);
          lcd.print(co);
          lcd.print(coUnit);
        }

        if (count == 4) count = 0;
      }
      delay(1000);
    }

  }
  
}

void _sdCardNotFound()
{

  Serial.println("SD card initialization failed");
  lcd.clear();
  lcd.print("Opps, SD card");
  lcd.setCursor(0, 1);
  lcd.print("not found!");
  exit(0);
  return;

}

void _triggerRelayA(bool toOpen)
{

  if (!relayAOpen && toOpen && ((millis()-relayBLastOpenTime) > (int(ecPumpBtoAOpenTriggerDelayMin*60)*1000))) {
    digitalWrite(relayPinA, LOW);
    relayAOpen = true;
    ecReadyForSolutionB = true;
    relayALastOpenTime = millis();
  }

  if (relayAOpen && ((millis()-relayALastOpenTime) > (ecPumpTimeSecondsSolutionA*1000))) {
    Serial.println("Trigger Close Relay A");
    digitalWrite(relayPinA, HIGH);
    relayAOpen = false;
  }

}

void _triggerRelayB()
{

  if (!relayBOpen && ecReadyForSolutionB && ((millis()-relayALastOpenTime) > (int(ecPumpAtoBOpenTriggerDelayMin*60)*1000))) {
    digitalWrite(relayPinB, LOW);
    relayBOpen = true;
    ecReadyForSolutionB = false;
    relayBLastOpenTime = millis();
  }

  if (relayBOpen && ((millis()-relayBLastOpenTime) > (ecPumpTimeSecondsSolutionB*1000))) {
    Serial.println("Trigger Close Relay B");
    digitalWrite(relayPinB, HIGH);
    relayBOpen = false;
  }

}

void _triggerRelayPlus(bool toOpen)
{

  if (!relayPlusOpen && toOpen) {
    digitalWrite(relayPinPlus, LOW);
    relayPlusOpen = true;
    relayPlusLastOpenTime = millis();
  }

  if (relayPlusOpen && ((millis()-relayPlusLastOpenTime) > (phPumpTimeSecondsSolutionPlus*1000))) {
    Serial.println("Trigger Close Relay Plus");
    digitalWrite(relayPinPlus, HIGH);
    relayPlusOpen = false;
  }

}

void _triggerRelayMinus(bool toOpen)
{

  if (!relayMinusOpen && toOpen) {
    digitalWrite(relayPinMinus, LOW);
    relayMinusOpen = true;
    relayMinusLastOpenTime = millis();
  }

  if (relayMinusOpen && ((millis()-relayMinusLastOpenTime) > (phPumpTimeSecondsSolutionMinus*1000))) {
    Serial.println("Trigger Close Relay Minus");
    digitalWrite(relayPinMinus, HIGH);
    relayMinusOpen = false;
  }

}