#include "teleFile.h"

TeleFile::TeleFile(unsigned fragmentSizeInput, double codingRateInput, void (*function)(byte [], unsigned)) 
: fragmentSize(fragmentSizeInput), codingRate(codingRateInput), functionCallBack(function), lastFrameNumber(2), currentStatus(WAITING)
{
}

TeleFile::~TeleFile() {
}

unsigned TeleFile::getFragmentSize() {
    return fragmentSize;
}

unsigned TeleFile::computeCodedSize(unsigned lenIn) {

    unsigned numberOfUncodedFragments = int((lenIn+1) / fragmentSize);
    double lenDouble = (lenIn+1);
    double fragmentSizeDouble = fragmentSize;
    if (lenDouble / fragmentSizeDouble != int(lenDouble / fragmentSizeDouble)) {
        numberOfUncodedFragments++;
    }

    unsigned numberOfCodedFragments = int(numberOfUncodedFragments * codingRate);
    double numberOfUncodedFragmentsDouble = numberOfUncodedFragments;
    double codingRateDouble = codingRate;
    if (numberOfUncodedFragmentsDouble * codingRateDouble != int(numberOfUncodedFragmentsDouble * codingRateDouble)) {
        numberOfCodedFragments++;
    }

    return numberOfCodedFragments*fragmentSize;
}

void TeleFile::encode(byte dataIn[], unsigned lenIn, byte dataOut[]) {

    // The following two blocs of code are calculating minimum number of uncoded
    // and coded fragments required to send the packet based on the fragment size
    // and the len of the packet.
    unsigned numberOfUncodedFragmentsTemp = int(lenIn / fragmentSize);
    double lenDouble = lenIn;
    double fragmentSizeDouble = fragmentSize;
    if (lenDouble / fragmentSizeDouble != int(lenDouble / fragmentSizeDouble)) {
        numberOfUncodedFragmentsTemp++;
    }
    const unsigned numberOfUncodedFragments = numberOfUncodedFragmentsTemp;

    unsigned numberOfCodedFragmentsTemp = int(numberOfUncodedFragments * codingRate);
    double numberOfUncodedFragmentsDouble = numberOfUncodedFragments;
    double codingRateDouble = codingRate;
    if (numberOfUncodedFragmentsDouble * codingRateDouble != int(numberOfUncodedFragmentsDouble * codingRateDouble)) {
        numberOfCodedFragmentsTemp++;
    }
    const unsigned numberOfCodedFragments = numberOfCodedFragmentsTemp;

    // Create a 2D array of numberOfUncodedFragments rows and fragmentSize columns
    byte** UNCODED_F; 
    UNCODED_F = new byte*[numberOfUncodedFragments];
    for (unsigned i = 0; i < numberOfUncodedFragments; i++) {
        UNCODED_F[i] = new byte[fragmentSize];
    }

    for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
        for (unsigned j(1); j <= fragmentSize; j++) {
            if (((i-1)*fragmentSize+(j-1)) < lenIn) {
                UNCODED_F[i-1][j-1] = dataIn[(i-1)*fragmentSize+(j-1)];
            }
            else if (((i-1)*fragmentSize+(j-1)) == lenIn) {
                UNCODED_F[i-1][j-1] = STOP_BYTE;
            }
            else {
                UNCODED_F[i-1][j-1] = 0;
            }
        }
    }

    // Create a 2D array of numberOfCodedFragments rows and fragmentSize columns
    byte** CODED_F;
    CODED_F = new byte*[numberOfCodedFragments];
    for (unsigned i = 0; i < numberOfCodedFragments; i++) {
        CODED_F[i] = new byte[fragmentSize];
    }

    for (unsigned y(1); y <= numberOfCodedFragments; y++) {
        byte s[fragmentSize]; 
        bool A[numberOfUncodedFragments];
        for (unsigned i(1); i <= fragmentSize; i++) {
            s[i-1] = 0;
        }
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            A[i-1] = 0;
        }

        matrixLine(y,numberOfUncodedFragments,A);

        for (unsigned x(1); x <= numberOfUncodedFragments; x++) {

            // If the fragment is selected by the random matrix
            if (A[x-1] == 1) {
                
                // Then each byte of the fragment is added to the linear combination
                for (unsigned z(1); z <= fragmentSize; z++) {
                    s[z-1] = s[z-1] ^ UNCODED_F[x-1][z-1];
                }
            }
        }
        // We then copy the linear combination vector to the coded fragment
        for (unsigned z(1); z <= fragmentSize; z++) {
            CODED_F[y-1][z-1] = s[z-1];
        }
    }

    for (unsigned i(1); i <= numberOfCodedFragments; i++) {
        for (unsigned j(1); j <= fragmentSize; j++) {
            dataOut[(i-1)*fragmentSize+(j-1)] = CODED_F[i-1][j-1];
        }
    }

    // Delete UNCODED_F and CODED_F
    for (unsigned i = 0; i < numberOfUncodedFragments; i++) {
        delete[] UNCODED_F[i];
    }
    delete[] UNCODED_F;  

    for (unsigned i = 0; i < numberOfCodedFragments; i++) {
            delete[] CODED_F[i];
        }
    delete[] CODED_F;
}

void TeleFile::decode(byte dataIn[], unsigned len) {

    static unsigned indexOrderLen = 0;
    static unsigned indexLen = 0;

    // Cast the first two bytes of the dataIn array as uint16_t in fragmentNumber
    uint16_t fragmentNumberRaw = dataIn[0] << 8 | dataIn[1];
    const unsigned fragmentNumber = unsigned(fragmentNumberRaw);
    // Cast the next two bytes of the dataIn array as uint16_t in numberOfCodedFragments
    uint16_t numberOfUncodedFragmentsRaw = dataIn[2] << 8 | dataIn[3];
    const unsigned numberOfUncodedFragments = unsigned(numberOfUncodedFragmentsRaw);

    // Serial.print("Progress : "); Serial.print(indexLen); Serial.print("/"); Serial.println(numberOfUncodedFragments);

    bool A[numberOfUncodedFragments];
    for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
        A[i-1] = 0;
    }

    matrixLine(fragmentNumber,numberOfUncodedFragments,A);

    byte fragMemory[fragmentSize];
    for (unsigned i(1); i<=fragmentSize; i++) {
        fragMemory[i-1] = dataIn[i+3];
    }

    if (fragmentNumber<lastFrameNumber) {
        // Serial.println("New file detected");
        currentStatus = RECEIVING_FRAMES;
        // Reset index to 0
        indexLen = 0;
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            index[i-1] = 0;
        }

        indexOrderLen = 0;
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            indexOrder[i-1] = 0;
        }
        // Reset the CODED_F_MEM array
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            for (unsigned j(1); j <= fragmentSize; j++) {
                CODED_F_MEM[i-1][j-1] = 0;
            }
        }

        // Reset the combinationMatrix
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            for (unsigned j(1); j <= numberOfUncodedFragments; j++) {
                combinationMatrix[i-1][j-1] = 0;
            }
        }

        index[indexLen++] = findFirstOne(A,numberOfUncodedFragments);

        // Copy the data contained in dataIn in the CODED_F_MEM array
        for (unsigned i(1); i <= fragmentSize; i++) {
            CODED_F_MEM[index[0]-1][i-1] = fragMemory[i-1];
        }

        // Copy A in the row index of the combinationMatrix
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            combinationMatrix[index[0]-1][i-1] = A[i-1];
        }

        indexOrder[indexOrderLen] = 1;
        lastFrameNumber = fragmentNumber;
    } 
    else if (currentStatus == RECEIVING_FRAMES) {

        static unsigned o;
        static unsigned col;

        for (unsigned i(1); i<=indexLen; i++) {

            o = indexOrder[i-1];
            col = index[o-1];

            if (A[col-1] == 1) {
                for (unsigned j(1); j <= numberOfUncodedFragments; j++) {
                    A[j-1] = A[j-1] ^ combinationMatrix[col-1][j-1];
                }

                for (unsigned j(1); j <= fragmentSize; j++) {
                    fragMemory[j-1] = fragMemory[j-1] ^ CODED_F_MEM[col-1][j-1];
                }
            }
        }
        if (!isEmpty(A, numberOfUncodedFragments)) {
            unsigned resultFind = findFirstOne(A,numberOfUncodedFragments);
            index[indexLen++] = resultFind;
            indexOrderLen++;

            for (unsigned i(1); i <= fragmentSize; i++) {
                CODED_F_MEM[index[indexLen-1]-1][i-1] = fragMemory[i-1];
            }

            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                combinationMatrix[index[indexLen-1]-1][i-1] = A[i-1];
            }

            sortIndex(index, indexLen, indexOrder);
    
            if (indexLen == numberOfUncodedFragments) {
                for (unsigned k(numberOfUncodedFragments-1); k >= 1; k--) {
                    for (unsigned i(1); i<=numberOfUncodedFragments; i++) {
                        A[i-1] = combinationMatrix[k-1][i-1];
                    }
                    for (unsigned i(1); i<=fragmentSize; i++) {
                        fragMemory[i-1] = CODED_F_MEM[k-1][i-1];
                    }
                    for (unsigned p(k+1); p<=numberOfUncodedFragments; p++) {
                        if (A[p-1]) {
                            for (unsigned i(1); i<=fragmentSize; i++) {
                                CODED_F_MEM[k-1][i-1] = CODED_F_MEM[k-1][i-1] ^ CODED_F_MEM[p-1][i-1];
                            }
                        } 
                    }
                }
                byte dataOutput[numberOfUncodedFragments*fragmentSize];
                static bool stopByteFound = false;
                unsigned realFileSize = 0;
                for (unsigned i(numberOfUncodedFragments); i>=1; i--) {
                    for (unsigned j(fragmentSize); j>=1; j--) {

                        byte currentByte = CODED_F_MEM[i-1][j-1];

                        if (!stopByteFound and currentByte == 0xFF) {
                            realFileSize = ((i-1)*fragmentSize+j-1)-1;
                            stopByteFound = true;
                        }
                        dataOutput[(i-1)*fragmentSize+j-1] = CODED_F_MEM[i-1][j-1];
                    }
                }
                currentStatus = SUCCESS_IDLE;
                Serial.println("");
                Serial.println("Success !");
                functionCallBack(dataOutput, realFileSize);
            }
        }
        lastFrameNumber = fragmentNumber;
    }
}


bool isEmpty(bool data[], unsigned len) {
    for (unsigned i(1); i <= len; i++) {
        if (data[i-1]) {
            return false;
        }
    }
    return true;
}

unsigned findFirstOne(bool data[], unsigned len) {
    for (unsigned i(1); i <= len; i++) {
        if (data[i-1]) {
            return i;
        }
    }
    return 0;
}

void sortIndex(unsigned dataArrayInRaw[], unsigned len, unsigned dataOrderInRaw[]) {

    unsigned temp(0);
    unsigned tempOrder(0);

    unsigned dataArrayIn[len];
    for (unsigned i(0); i<len; i++) {
      dataArrayIn[i] = dataArrayInRaw[i];
    }

    unsigned dataOrderIn[len];
    for (unsigned i(1); i<=len; i++) {
      dataOrderIn[i-1] = i;
    }
  
    for (unsigned i(1); i <= len; i++) {
        for (unsigned j(i+1); j <= len; j++) {
            if ((dataArrayIn[i-1] > dataArrayIn[j-1])) {
                temp = dataArrayIn[i-1];
                dataArrayIn[i-1] = dataArrayIn[j-1];
                dataArrayIn[j-1] = temp;
                tempOrder = dataOrderIn[i-1];
                dataOrderIn[i-1] = dataOrderIn[j-1];
                dataOrderIn[j-1] = tempOrder;
            }
        }
    }
    for (unsigned i(1); i <= len; i++) {
        dataOrderInRaw[i-1] = dataOrderIn[i-1];
    }
}

// This function should return the line N of the parity check matrix NxM
// This matrix contains 0 and 1 and is used to then create the linear combination of fragments. 
// For example, if the matrixLine functions returns 1 0 1 0 1 0 1 .... 0 1 then the combination of fragments
// will be the sum (XOR) of the first fragment, the third fragment, the fifth fragment, the seventh fragment, etc...
void matrixLine(unsigned N, unsigned M, bool matrixLineResult[]) {

    // keyValue is the key?
    unsigned keyValue = 13;
    unsigned x = keyValue+260*N;
    unsigned nbCoeff = 0;

    while (nbCoeff<M/2) {
        unsigned r = 511;
        while (r>=M) {
            x = prbs16(x);
            r = x - (x/M)*M;
        }
        if (!matrixLineResult[r]) {
            matrixLineResult[r] = true;
        }
        nbCoeff = nbCoeff+1;
    }
}

int prbs16(int x) {
    int lsb = x & 1;
    x = round(x/2);
    if (lsb==1) {
        x = x ^ 0xB400;
    }
    return x;
}


