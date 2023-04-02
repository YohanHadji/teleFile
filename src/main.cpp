#include <Arduino.h>
#include "capsule.h"
#include "teleFile.h"
#include "SD.h"

#define DEVICE1_PORT Serial3
#define DEVICE1_BAUD 115200

#define SENDER true

void handlePacketDevice1(byte, byte [], unsigned);
void handleFileTransfer1(byte dataIn[], unsigned dataSize);

// Stuff that's only useful for the demo 
void sendFile();
void readFile(byte []);
unsigned findImageSize();

const int chipSelect = BUILTIN_SDCARD;

Capsule device1(0xFF,0xFA,handlePacketDevice1);

int fragmentSize = 200;
double codingRate = 2.0;

TeleFile fileTransfer1(fragmentSize,codingRate,handleFileTransfer1);

void setup() {
  DEVICE1_PORT.begin(DEVICE1_BAUD);
  Serial.begin(115200);
  SD.begin(chipSelect);

  //const unsigned imageBufferSize = findImageSize();
  //byte imageBuffer[imageBufferSize];
  //readFile(imageBuffer);  

  // Random data buffer, will be replaced by an actual file later
}

void loop() {
  while(DEVICE1_PORT.available()) {
    byte data = DEVICE1_PORT.read();
    device1.decode(data);
  }
  static int lastFileSent = -40000;
  if ((millis()-lastFileSent)>45000) {
    lastFileSent = millis();
    sendFile();
  } 
}

void handlePacketDevice1(byte packetId, byte dataIn[], unsigned len) {
  switch (packetId) {
    case 0x00:
      if (random(0,100) < 10) {
        Serial.println("RX correct but dropping packet :)");
      }
      else {
        Serial.print("RX correct frag number: "); Serial.println(dataIn[0] << 8 | dataIn[1]);
        fileTransfer1.decode(dataIn, len);
      }
    break;
    case 0x01:
    break;
    default:
    break;
  }
}

void handleFileTransfer1(byte dataIn[], unsigned dataSize) {
  // Do something with the file
  Serial.print("Received file of size "); Serial.println(dataSize);
  Serial.print("First 8 bytes: ");
  digitalWrite(LED_BUILTIN, HIGH);
  for (unsigned i = 0; i < min(dataSize,8); i++) {
    Serial.print(dataIn[i]); Serial.print(" ");
  }
  Serial.println();
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  File file = SD.open("imageWritten.jpeg", FILE_WRITE);
  if (file) {
    file.write(dataIn, dataSize);
    file.close();
    Serial.println("File write successful!!!!");
  }
  else {
    Serial.println("Error opening imageWritten.jpg");
  }
}

unsigned findImageSize() {
  File file = SD.open("image.jpeg");
  if (file) {
    unsigned result = file.size();
    file.close();
    return result;
  } else {
    Serial.println("error opening image.jpg");
    return 0;
  }
}

void readFile(byte dataOut[]) {
  File file = SD.open("image.jpeg");
  if (file) {
    unsigned i = 0;
    while (file.available()) {
      dataOut[i] = file.read();
      i++;
    }
    file.close();
  }
  else {
    Serial.println("error opening image.jpg");
  }
}

void sendFile() {
  if (SENDER) {
    const unsigned imageBufferSize = findImageSize();
    Serial.print("Image size = ");
    Serial.println(imageBufferSize);
    byte imageBuffer[imageBufferSize];
    readFile(imageBuffer);

    const unsigned outputLen = fileTransfer1.computeCodedSize(imageBufferSize);
    const unsigned fragmentCutSize = fileTransfer1.getFragmentSize();
    
    const unsigned numberOfCodedFragments = outputLen / fragmentCutSize;

    unsigned numberOfUncodedFragmentsTemp = int(imageBufferSize / fragmentSize);
    double lenDouble = imageBufferSize;
    double fragmentSizeDouble = fragmentSize;
    if (lenDouble / fragmentSizeDouble != int(lenDouble / fragmentSizeDouble)) {
        numberOfUncodedFragmentsTemp++;
    }
    const unsigned numberOfUncodedFragments = numberOfUncodedFragmentsTemp;

    Serial.print("Fragment cut size = "); Serial.println(fragmentCutSize);
    Serial.print("Number of coded fragments = "); Serial.println(numberOfCodedFragments);
    Serial.print("Output len = "); Serial.println(outputLen);
    Serial.print("Number of uncoded fragments = "); Serial.println(numberOfUncodedFragments);

    byte *output;
    output = new byte[outputLen];

    static unsigned timeBefore = millis();
    fileTransfer1.encode(imageBuffer, imageBufferSize, output);
    Serial.print("Done encoding with time = "); Serial.println(millis()-timeBefore);

    for (unsigned i = 1; i <= numberOfCodedFragments; i++) {
      Serial.print("Packet n° "); Serial.print(i); Serial.print("/"); Serial.println(numberOfCodedFragments);
      unsigned fragmentLen = fragmentCutSize+4;
      byte packetData[fragmentLen];
      byte packetId = 0x00;

      // The two first data bytes are the packet number (i) as a 16 bit integer
      packetData[0] = i >> 8;
      packetData[1] = i & 0xFF;
      // The two next data bytes are the total number of packets (numberOfCodedFragments) as a 16 bit integer
      packetData[2] = numberOfUncodedFragments >> 8;
      packetData[3] = numberOfUncodedFragments & 0xFF;

      for (unsigned j = 0; j < fragmentCutSize; j++) {
        packetData[j+4] = output[(i-1)*fragmentCutSize+j];
      }

      byte* packetToSend = device1.encode(packetId,packetData,fragmentLen);
      DEVICE1_PORT.write(packetToSend, getCodedLen(fragmentLen)); 
      delete[] packetToSend;
      delay(10);
    }
    delete[] output;
    Serial.println("Done sending data");
  }
}