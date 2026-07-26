/*
 Sending Data to Blender -  Arduino Sketch

  Author: Usman 
  Date: 26-JULY-2025
  Website: www.electronicstree.com
  Email: help@electronicstree.com


 Arduino to Blender : Sends Coordinate and Text Data to Blender via Serial.
 --------------------------------------------
 This Arduino code demonstrates how to use the blendixserialCSV library to send
 coordinate data and a text message to a Blender addon via serial communication.
 The code initializes serial communication, configures the coordinate data type and 
 number of coordinate sets, and generates random floating-point coordinates. 
 Each set now carries 9 values (v0 .. v8) instead of 3 (x, y, z), matching the
 addon's CSV layout. These values, along with a text message, are then formatted
 into a string using the blendixserialCSV library and sent over the serial port.


 If you encounter any errors or bugs while using the blendixserialCSV library or this code,
  please feel free to report them. Your feedback is valuable for improvement!

 Thank you for your help!


*/


#include "blendixserial_csv_ard.h"

blendixserialCSV blendix;  // Create an instance of blendixserialCSV

void setup() {
    Serial.begin(9600);  // Start Serial communication

    // Set coordinate type to FLOAT (or COORD_TYPE_INT for integers)
    blendix.setCoordinateType(COORD_TYPE_FLOAT);

    // Set number of transmitted coordinate sets
    blendix.setTxSets(3);

    // Seed random generator with an unpredictable value.
    randomSeed(analogRead(0));  
}

void loop() {
    // Generate random float values (scaled to a range, e.g., 0.0 to 100.0)
    for (int i = 1; i <= 3; i++) {
        float v0 = random(0, 100) / 10.0;  // Convert to float by dividing by 10
        float v1 = random(0, 100) / 10.0;
        float v2 = random(0, 100) / 10.0;
        float v3 = random(0, 100) / 10.0;
        float v4 = random(0, 100) / 10.0;
        float v5 = random(0, 100) / 10.0;
        float v6 = random(0, 100) / 10.0;
        float v7 = random(0, 100) / 10.0;
        float v8 = random(0, 100) / 10.0;

        blendix.setCoordinates(i, v0, v1, v2, v3, v4, v5, v6, v7, v8);
    }

    // Set a dynamic text message
    blendix.setText("blendixserial blender addon");

    // Prepare and send formatted data
    // (buffer enlarged to fit 3 sets x 9 values + text comfortably)
    uint8_t outputBuffer[256];
    blendix.getFormattedOutput(outputBuffer, sizeof(outputBuffer));

    Serial.println((char*)outputBuffer);

    // Send new random values every 1 second
    delay(1000);
}
