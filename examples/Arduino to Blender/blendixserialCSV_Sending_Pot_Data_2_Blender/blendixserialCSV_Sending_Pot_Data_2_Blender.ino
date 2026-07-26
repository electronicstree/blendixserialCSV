/*
  Sending Potentiometer Data to Blender - Arduino Sketch

  Author: Usman  
  Date: 26-JULY-2025  
  Website: www.electronicstree.com  
  Email: help@electronicstree.com  

  Arduino to Blender : Sending Mapped Potentiometer Data to Blender.
  --------------------------------------------
  This Arduino code reads values from a potentiometer, maps them to a 
  floating-point range (0.0 to 2.7) in increments of 0.3, and sends the 
  formatted data to Blender via serial communication using the blendixserialCSV 
  library. Each set now carries 9 values (v0 .. v8) instead of 3 (x, y, z); the
  mapped value is sent as v0, with v1 .. v8 left at zero.

  If you encounter any errors or bugs while using the blendixserialCSV library 
  or this code, please feel free to report them. Your feedback is valuable 
  for improvement!  

  Thank you for your help!
*/


#include "blendixserial_csv_ard.h"

blendixserialCSV serialSender;  // Create blendixserialCSV instance

int potPin = A0;  // Potentiometer connected to A0
float mappedValue = 0.0;  
float previousValue = -1.0;  

void setup() {
    Serial.begin(9600);  // Start serial communication
    serialSender.setCoordinateType(COORD_TYPE_FLOAT);  
    serialSender.setTxSets(1);  // Only sending 1 set of values
}

void loop() {
    int potValue = analogRead(potPin);  
      mappedValue = round(((potValue / 1023.0) * 2.7) * 10 / 3) * 0.3;

    if (mappedValue != previousValue) {
        // Update previous value
        previousValue = mappedValue;  

        // Send mappedValue as v0, keeping v1 .. v8 as zero
        serialSender.setCoordinates(1, mappedValue, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        
        // Send formatted output to Blender
        // (buffer enlarged slightly to comfortably fit 9 values)
        uint8_t outputBuffer[64];  
        serialSender.getFormattedOutput(outputBuffer, sizeof(outputBuffer));

        Serial.println((char*)outputBuffer);  
    }

}
