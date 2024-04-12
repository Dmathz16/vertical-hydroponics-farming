#include "HardwareSerial.h"
#ifndef SENSOR_H
#define SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include "DFRobot_EC10.h"

class Sensor {
  private:

    // EC
    const int ecPin = A1;
    float conductivityValue; 
    float calibrationFactor = 92; 

    // PH
    const int pHPin = A0;
    int phRawRounds = 50;
    const int phRaw400 = 338;
    const int phRaw686 = 382;
    const int phRaw918 = 417;

    // WL
    const int wlTriggerPin  = 11;
    const int wlEchoPin     = 12;
    unsigned long duration;
    float distance_cm;
    const float wlCmAtWaterNone = 33.19;
    const float wlCmAtWaterFull = 9.35;

    // WF
    const int wfPin = 3; 
    static volatile int wfPulseCount;
    unsigned long wfPreviousMillis = 0; 
    unsigned long wfElapsedTime = 0; 
    float wfFlowRate = 0.0; 

    // TP
    const int tpPin = 2;
    OneWire oneWire{tpPin};
    DallasTemperature tempSensor{&oneWire};

    // HM
    const int hmPin = 10;
    const int hmType = DHT11;
    DHT dht{hmPin, hmType};

    // CO
    const int coPin = A2;
    const int coZero = 127; // for calibration

  public:
  Sensor() {}

  void begin() {
    pinMode(wlTriggerPin, OUTPUT);
    pinMode(wlEchoPin, INPUT);
    pinMode(wfPin, INPUT);
    pinMode(coPin, INPUT);
    tempSensor.begin();
    dht.begin();
  }

  // float mapping
  float _mapFloat(float x, float in_min, float in_max, float out_min, float out_max) 
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  static void _pulseCounter() 
  {
    wfPulseCount++;
  }

  float _calculateFlowRate(int pulses) 
  {
    float wfFlowRate = pulses / 7.5; 
    return wfFlowRate;
  }

  float readEC() 
  {
    int sensorValue = analogRead(ecPin); 
    conductivityValue = map(sensorValue, 0, 1023, 0, 2000); 
    conductivityValue *= calibrationFactor; 
    conductivityValue /= 1000; 
    return conductivityValue;
  }

  float readPH() 
  {

    float slope       = (phRaw686-phRaw400)/(6.86-4.0); // get slope
    float phVoltLow   = phRaw400-(4.0*slope); // get Raw at 0
    float phVoltHigh  = phRaw400+((14-4.0)*slope); // get Raw at 14

    float rawValue = 0.00;
    for (int i=0; i<phRawRounds; i++) {
      rawValue += analogRead(pHPin);
    }
    rawValue = rawValue/phRawRounds;
    
    float pHValue = _mapFloat(rawValue, phVoltLow, phVoltHigh, 0.0, 14.0); // Map voltage to pH range (0-14)

    if (rawValue < phVoltLow) pHValue = 0.0;
    if (rawValue > phVoltHigh) pHValue = 14.0;
    
    // // Print the pH value to the serial monitor
    // Serial.print("pH: ");
    // Serial.print(rawValue);
    // Serial.print("    ");
    // Serial.println(pHValue);

    return pHValue;
  }

  float readWL() 
  {

    // Clear the trigger pin
    digitalWrite(wlTriggerPin, LOW);
    delayMicroseconds(2);

    // Send a 10 microsecond pulse to trigger the sensor
    digitalWrite(wlTriggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(wlTriggerPin, LOW);

    // Measure the duration of the echo pulse
    duration = pulseIn(wlEchoPin, HIGH);

    // Calculate distance in centimeters
    distance_cm = duration * 0.034 / 2;
    float level = _mapFloat(distance_cm, wlCmAtWaterNone, wlCmAtWaterFull, 0.0, 100.0);

    if (level < 0) level = 0.00;
    if (level > 100) level = 100.00;
    return level;
  }

  float readWF(int msTime)
  {
    attachInterrupt(digitalPinToInterrupt(wfPin), _pulseCounter, FALLING); 
    wfPreviousMillis = millis(); 
    while (wfElapsedTime < msTime) {
      unsigned long currentMillis = millis(); 
      wfElapsedTime = currentMillis - wfPreviousMillis; 
      if (wfElapsedTime >= msTime) {
        wfFlowRate = _calculateFlowRate(wfPulseCount); 
        wfFlowRate = wfFlowRate * 2; // x2 to get 1 sec 
      }
    }
    return wfFlowRate;
  }

  float readTP() 
  {
    tempSensor.requestTemperatures();
    float temperature = tempSensor.getTempCByIndex(0);
    return temperature;
  }

  float readHM() 
  {
    return dht.readHumidity();
  }

  float readCO() 
  {

    int coNow[10];
    int coRaw = 0;
    int coComp = 0;
    int coPpm = 0;
    int zzz = 0;

    for (int i=0; i<10; i++) {
      coNow[i] = analogRead(coPin);
      zzz = zzz + coNow[i];
    }

    coRaw = zzz/10;
    coComp = coRaw - coZero;
    coPpm = _mapFloat(coComp, 0, 1023, 400, 5000);
    return coPpm;
  }

};

#endif
