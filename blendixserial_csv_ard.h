/*
  blendixserial_csv_ard.h  –  BlendixSerial CSV Edition for Arduino
  ==================================================================
  A single-header, CSV-protocol companion to the BlendixSerial binary
  library. Safe to use alongside the original "blendixserial.h" — no
  name collisions (this file uses the class name "blendixserialCSV").

  Supports switchable coordinate storage: int or float (see
  setCoordinateType / COORD_TYPE_INT / COORD_TYPE_FLOAT below).
  Received values are always returned as float, regardless of the
  transmit storage type.

  Each coordinate set holds 9 values (v0 .. v8) — 3 Location + 3
  Rotation + 3 Scale, matching the BlendixSerial addon's CSV layout.

  TRANSMIT FORMAT  (getFormattedOutput):
      "v0,v1,v2,v3,v4,v5,v6,v7,v8,...;OptionalText"
       9 values per set, comma-separated, semicolon at the end.

  RECEIVE FORMAT  (parseReceivedData):
      "v0,v1,v2,v3,v4,v5,v6,v7,v8,...;"
       Total values must be a multiple of 9, must end with ';'.

  NOTE ON PERFORMANCE:
      CSV is plain text and always sends all 9 values per set, so it
      is slower and heavier on bandwidth than the binary protocol.
      Best suited for one or two objects. For more objects or higher
      update rates, use a high baud rate (115200+), or switch to the
      binary BlendixSerial-Arduino library instead.

  USAGE:
      #include "blendixserial_csv_ard.h"
      blendixserialCSV bs;

  Author:     Usman
  Date:       2026-07-26
  Modified:   9 values per set, single-header, CSV protocol, int/float
              switchable transmit storage.
*/

#ifndef blendixserial_csv_ard_h
#define blendixserial_csv_ard_h

#include <Arduino.h>


#ifndef BLENDIX_MAX_SETS
#define BLENDIX_MAX_SETS 5
#endif


#ifndef BLENDIX_TEXT_BUFFER_SIZE
#define BLENDIX_TEXT_BUFFER_SIZE 50
#endif

// Number of values (v0..v8) stored per coordinate set
#ifndef BLENDIX_VALUES_PER_SET
#define BLENDIX_VALUES_PER_SET 9
#endif

#define COORD_TYPE_INT "int"
#define COORD_TYPE_FLOAT "float"

// Since all methods are defined inside the class body, they are implicitly
// inline, so this header can be safely included from multiple .cpp files
// without needing a separate .cpp / linking a library.
class blendixserialCSV {
private:

  enum CoordinateType { INT_TYPE, FLOAT_TYPE };


  struct CoordinatesInt {
    int v[BLENDIX_VALUES_PER_SET];
  };


  struct CoordinatesFloat {
    float v[BLENDIX_VALUES_PER_SET];
  };


  struct ReceivedCoordinates {
    float v[BLENDIX_VALUES_PER_SET];
  };

  void* coordinates;
  CoordinateType coordType;
  int numSets;
  char* text;
  size_t textBufferSize;
  int receiveSets;
  ReceivedCoordinates* receivedCoordinates;
  int receivedSets;

  bool setCoordinateTypeInternal(CoordinateType type) {
    // Only proceed if the new type is different
    if (type != coordType) {
      // Free the memory from the old coordinate array
      delete[] static_cast<CoordinatesInt*>(coordinates);

      // Update the coordinate type
      coordType = type;

      // Allocate a new array based on the updated type
      if (coordType == INT_TYPE) {
        coordinates = new CoordinatesInt[BLENDIX_MAX_SETS];
      } else {
        coordinates = new CoordinatesFloat[BLENDIX_MAX_SETS];
      }

      // Reset all coordinate values to zero
      resetCoordinates();
      return true;
    }
    return false; // No change needed
  }

  bool validateAndParseData(const char* inputData, ReceivedCoordinates*& tempCoords, int& tempNumSets) {
    if (!inputData) return false;

    // Make a copy for safe tokenization
    char* dataCopy = strdup(inputData);
    // Tokenize by commas and semicolons
    char* token = strtok(dataCopy, ",;");

    // Allocate an array for float values (up to receiveSets * BLENDIX_VALUES_PER_SET)
    float* values = new float[receiveSets * BLENDIX_VALUES_PER_SET];
    int valueIndex = 0;

    // Parse tokens
    while (token) {
      // If we exceed our expected max, stop
      if (valueIndex >= receiveSets * BLENDIX_VALUES_PER_SET) {
        break;
      }
      // Convert token to float
      values[valueIndex++] = atof(token);
      // Move to the next token
      token = strtok(nullptr, ",;");
    }

    // Clean up temporary copy
    free(dataCopy);

    // Check if the total number of floats is a multiple of BLENDIX_VALUES_PER_SET
    if (valueIndex % BLENDIX_VALUES_PER_SET != 0) {
      delete[] values;
      return false;
    }

    // Calculate how many sets were found
    tempNumSets = valueIndex / BLENDIX_VALUES_PER_SET;

    // If the data has more sets than we can store, limit to receiveSets
    if (tempNumSets > receiveSets) {
      tempNumSets = receiveSets;
    }

    // Allocate a new array of ReceivedCoordinates
    tempCoords = new ReceivedCoordinates[tempNumSets];

    // Fill the temporary coordinate array
    for (int i = 0, coordIndex = 0; i < valueIndex && coordIndex < tempNumSets; i += BLENDIX_VALUES_PER_SET, coordIndex++) {
      for (int k = 0; k < BLENDIX_VALUES_PER_SET; k++) {
        tempCoords[coordIndex].v[k] = values[i + k];
      }
    }

    // Free the float array
    delete[] values;

    return true;
  }

public:

  blendixserialCSV()
      : numSets(1),            // Default to 1 transmit set
        receiveSets(0),        // Default to 0 receive sets
        textBufferSize(BLENDIX_TEXT_BUFFER_SIZE),
        coordinates(nullptr),
        coordType(INT_TYPE),
        text(nullptr),
        receivedCoordinates(nullptr),
        receivedSets(0)
  {
    coordinates = new CoordinatesInt[BLENDIX_MAX_SETS];
    receivedCoordinates = new ReceivedCoordinates[BLENDIX_MAX_SETS];
    resetCoordinates();
    text = new char[BLENDIX_TEXT_BUFFER_SIZE];
    text[0] = '\0';
  }

  bool setCoordinateType(const char* type) {
    if (strcmp(type, COORD_TYPE_INT) == 0) {
      return setCoordinateTypeInternal(INT_TYPE);
    } else if (strcmp(type, COORD_TYPE_FLOAT) == 0) {
      return setCoordinateTypeInternal(FLOAT_TYPE);
    }
    return false; // Invalid string
  }

  bool setTxSets(int sets) {
    if (sets < 0) {
      return false;
    }
    // Check combined sets
    if (sets + receiveSets > BLENDIX_MAX_SETS) {
      return false;
    }
    numSets = sets;
    return true;
  }

  bool setRxSets(int sets) {
    if (sets < 0) {
      return false;
    }
    // Check combined sets
    if (sets + numSets > BLENDIX_MAX_SETS) {
      return false;
    }
    receiveSets = sets;
    return true;
  }

  // Set a full set of 9 int values (v0..v8) for setNum (1-indexed).
  bool setCoordinates(int setNum, int v0, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8) {
    // Ensure the setNum is within range and we're using int-based coordinates
    if (setNum >= 1 && setNum <= numSets && coordType == INT_TYPE) {
      auto coords = static_cast<CoordinatesInt*>(coordinates);
      coords[setNum - 1].v[0] = v0;
      coords[setNum - 1].v[1] = v1;
      coords[setNum - 1].v[2] = v2;
      coords[setNum - 1].v[3] = v3;
      coords[setNum - 1].v[4] = v4;
      coords[setNum - 1].v[5] = v5;
      coords[setNum - 1].v[6] = v6;
      coords[setNum - 1].v[7] = v7;
      coords[setNum - 1].v[8] = v8;
      return true;
    }
    return false;
  }

  // Set a full set of 9 float values (v0..v8) for setNum (1-indexed).
  bool setCoordinates(int setNum, float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8) {
    // Ensure the setNum is within range and we're using float-based coordinates
    if (setNum >= 1 && setNum <= numSets && coordType == FLOAT_TYPE) {
      auto coords = static_cast<CoordinatesFloat*>(coordinates);
      coords[setNum - 1].v[0] = v0;
      coords[setNum - 1].v[1] = v1;
      coords[setNum - 1].v[2] = v2;
      coords[setNum - 1].v[3] = v3;
      coords[setNum - 1].v[4] = v4;
      coords[setNum - 1].v[5] = v5;
      coords[setNum - 1].v[6] = v6;
      coords[setNum - 1].v[7] = v7;
      coords[setNum - 1].v[8] = v8;
      return true;
    }
    return false;
  }

  void resetCoordinates() {
    if (coordType == INT_TYPE) {
      // Cast to int array
      auto coords = static_cast<CoordinatesInt*>(coordinates);
      for (int i = 0; i < numSets; i++) {
        for (int k = 0; k < BLENDIX_VALUES_PER_SET; k++) {
          coords[i].v[k] = 0;
        }
      }
    } else {
      // Cast to float array
      auto coords = static_cast<CoordinatesFloat*>(coordinates);
      for (int i = 0; i < numSets; i++) {
        for (int k = 0; k < BLENDIX_VALUES_PER_SET; k++) {
          coords[i].v[k] = 0.0f;
        }
      }
    }
  }

  void setText(const char* inputText) {
    if (text && inputText) {
      strncpy(text, inputText, textBufferSize - 1);
      text[textBufferSize - 1] = '\0'; // Ensure null termination
    }
  }

  void getFormattedOutput(uint8_t* outputBuffer, size_t bufferSize) {
    // If no valid buffer or size is zero, do nothing
    if (!outputBuffer || bufferSize == 0) return;

    // Keep track of our position in outputBuffer
    size_t offset = 0;

    // Temporary buffer for string conversions (size depends on int vs float)
    size_t tempBufferSize = (coordType == INT_TYPE) ? 12 : 16;
    char* tempBuffer = new char[tempBufferSize];

    // Format coordinates
    if (coordType == INT_TYPE) {
      // Cast the coordinates pointer to int-based structure
      auto coords = static_cast<CoordinatesInt*>(coordinates);
      for (int i = 0; i < numSets; i++) {
        for (int k = 0; k < BLENDIX_VALUES_PER_SET; k++) {
          // Convert value to string and append
          snprintf(tempBuffer, tempBufferSize, "%d", coords[i].v[k]);
          offset += snprintf((char*)outputBuffer + offset, bufferSize - offset, "%s", tempBuffer);

          // Add comma after every value except the very last one overall
          bool isLastValueOfLastSet = (i == numSets - 1) && (k == BLENDIX_VALUES_PER_SET - 1);
          if (!isLastValueOfLastSet && offset < bufferSize - 1) {
            outputBuffer[offset++] = ',';
          }
        }
      }
    } else {
      // Cast the coordinates pointer to float-based structure
      auto coords = static_cast<CoordinatesFloat*>(coordinates);
      for (int i = 0; i < numSets; i++) {
        for (int k = 0; k < BLENDIX_VALUES_PER_SET; k++) {
          // Convert value to string with 2 decimal places
          dtostrf(coords[i].v[k], 0, 2, tempBuffer);
          offset += snprintf((char*)outputBuffer + offset, bufferSize - offset, "%s", tempBuffer);

          // Add comma after every value except the very last one overall
          bool isLastValueOfLastSet = (i == numSets - 1) && (k == BLENDIX_VALUES_PER_SET - 1);
          if (!isLastValueOfLastSet && offset < bufferSize - 1) {
            outputBuffer[offset++] = ',';
          }
        }
      }
    }

    // Append semicolon and text at the end, if there's space
    if (offset < bufferSize - 1) {
      snprintf((char*)outputBuffer + offset, bufferSize - offset, ";%s", text ? text : "");
    }

    // Clean up
    delete[] tempBuffer;
    // Ensure the output is null-terminated
    outputBuffer[bufferSize - 1] = '\0';
  }

  bool parseReceivedData(const String& inputData) {
    // Convert Arduino String to C-style string
    const char* inputCStr = inputData.c_str();

    // We expect the data to end with a semicolon
    if (inputCStr == nullptr || inputCStr[strlen(inputCStr) - 1] != ';') {
      return false;
    }

    // Temporary pointers for parsed data
    ReceivedCoordinates* tempCoords = nullptr;
    int tempNumSets = 0;

    // Attempt to parse
    if (validateAndParseData(inputCStr, tempCoords, tempNumSets)) {
      // If successful, free the old array and store the new one
      delete[] receivedCoordinates;
      receivedCoordinates = tempCoords;
      receivedSets = tempNumSets;
      return true;
    }
    return false;
  }

  int getReceivedNumSets() const {
    return receivedSets;
  }

  // Copies the 9 received values (v0..v8) for the given set index into
  // the provided references.
  bool getReceivedCoordinates(int index, float& v0, float& v1, float& v2, float& v3, float& v4, float& v5, float& v6, float& v7, float& v8) const {
    // Check if the index is in range and we have a valid array
    if (index < 0 || index >= receivedSets || !receivedCoordinates) {
      return false;
    }

    // Copy the stored coordinates into the provided references
    v0 = receivedCoordinates[index].v[0];
    v1 = receivedCoordinates[index].v[1];
    v2 = receivedCoordinates[index].v[2];
    v3 = receivedCoordinates[index].v[3];
    v4 = receivedCoordinates[index].v[4];
    v5 = receivedCoordinates[index].v[5];
    v6 = receivedCoordinates[index].v[6];
    v7 = receivedCoordinates[index].v[7];
    v8 = receivedCoordinates[index].v[8];
    return true;
  }
};

#endif