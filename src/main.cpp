#include <Arduino.h>
#include "capsule.h"
#include "teleFile.h"
#include "SD.h"

#define DEVICE1_PORT Serial3
#define DEVICE1_BAUD 115200

#define SENDER  true
#define DEBUG   true

void handlePacketDevice1(byte, byte [], unsigned);
void handleFileTransfer1(byte dataIn[], unsigned dataSize);

// Stuff that's only useful for the demo 
void sendFile();
void readFile(byte []);
unsigned findImageSize();

const int chipSelect = BUILTIN_SDCARD;

// PRA and PRB are the preamble bytes used by Capsule to detect the start of a packet.
byte PRA = 0xFF;
byte PRB = 0xFA;

// The fragment size is the size in bytes of each fragment of the file that will be sent using teleFile. 
// The coding rate is the degree of redundancy used to encode the file.
int fragmentSize = 200;
double codingRate = 2.0;

Capsule device1(PRA,PRB,handlePacketDevice1);
TeleFile fileTransfer1(fragmentSize,codingRate,handleFileTransfer1);

void setup() {
  DEVICE1_PORT.begin(DEVICE1_BAUD);
  Serial.begin(115200);
  SD.begin(chipSelect);
}

void loop() {
  while(DEVICE1_PORT.available()) {
    byte data = DEVICE1_PORT.read();
    device1.decode(data);
  }
  // Sending the file 5s after startup and again every 45 seconds.. 
  static int lastFileSent = -40000;
  if ((millis()-lastFileSent)>45000) {
    lastFileSent = millis();
    if (SENDER) {
      sendFile();
    }
  } 
}

// This function is called everytime a packet has been received by the Capsule class.
// Every packet with prefix 0x00 is a fragment of the file, we pass this fragment to teleFile class for decoding. 
// We drop one packet every 10 to simulate a packet loss...
void handlePacketDevice1(byte packetId, byte dataIn[], unsigned len) {
  switch (packetId) {
    case 0x00:
      if (random(0,100) < 10) {
        if (DEBUG) {
          Serial.println("RX correct but dropping packet for fun :)");
        }
      }
      else {
        if (DEBUG) {
          Serial.print("RX correct frag number: "); Serial.println(dataIn[0] << 8 | dataIn[1]);
          fileTransfer1.decode(dataIn, len);
        }
      }
    break;
    case 0x01:
    break;
    default:
    break;
  }
}

// This function is called everytime a full file has been received by teleFile class. 
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

// This function returns the size of image.jpeg to allocate memory to then copy the content of the file in it.
unsigned findImageSize() {
  File file = SD.open("image.jpeg");
  if (file) {
    unsigned result = file.size();
    file.close();
    return result;
  } else {
    if (DEBUG) {
      Serial.println("Error opening image.jpg");
    }
    return 0;
  }
}

// This function reads the file image.jpeg on the SD cards and feed the array given in input with it. 
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
    if (DEBUG) {
      Serial.println("Error opening image.jpg");
    }
  }
}

// This function is an example of how to send a picture with teleFile. It uses the main teleFile function: encode(input, size, output)
// This function will give the user the encoded data (fetched in output) and the user can send it over the air. 
// Because we have a fixed minimum fragment size, the length of the concatenation of fragments to send over the air will probably never 
// be exactly the initial size of the data. Internally, a stop byte is added at the end of the user data when transmitted to detect the end of file
// when it is re-constructed at the end of the decode() function. 
//
// The computeCodedSize(inputSize) function will give the size of the output buffer needed to store the encoded data. This is required to 
// allocate the output buffer memory before giving it to be encoded in encode(input, size, output) function. The reason it's not required to give
// the output size of the buffer to the encode function is that it's using the same computeCodedSize function internally to compute it. 
void sendFile() {
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
  static unsigned timeBefore = millis();

  byte *output;
  output = new byte[outputLen];
  fileTransfer1.encode(imageBuffer, imageBufferSize, output);

  if (DEBUG) {
    Serial.print("Done encoding with time = "); Serial.println(millis()-timeBefore);
    Serial.print("Fragment cut size = "); Serial.println(fragmentCutSize);
    Serial.print("Number of coded fragments = "); Serial.println(numberOfCodedFragments);
    Serial.print("Output len = "); Serial.println(outputLen);
    Serial.print("Number of uncoded fragments = "); Serial.println(numberOfUncodedFragments);
  }

  // We need to send each fragment one by one, because this will take some time, this is not integrated
  // in the teleFile class, this is something the user has to do himself. But it's quite easy. Cut the fragment 
  // to lenght and send it. 
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
    // This delay is here to simulate how it will be hapenning in real life with telemetry packets. 
    delay(10);
  }
  delete[] output;
  Serial.println("Done sending data");
}