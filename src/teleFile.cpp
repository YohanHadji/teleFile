#include "teleFile.h"

TeleFile::TeleFile(unsigned fragmentSizeInput, double codingRateInput, void (*function)(byte [], unsigned)) 
: fragmentSize(fragmentSizeInput), codingRate(codingRateInput), functionCallBack(function), lastFrameNumber(2), currentStatus(WAITING)
{
    CODED_F_MEM = new byte*[NB_FRAGMENT_MAX];
    for (int i = 0; i < NB_FRAGMENT_MAX; i++) {
        CODED_F_MEM[i] = new byte[fragmentSizeInput];
    }

    combinationMatrix = new byte*[NB_FRAGMENT_MAX];
    for (int i = 0; i < NB_FRAGMENT_MAX; i++) {
        combinationMatrix[i] = new byte[NB_FRAGMENT_MAX];
    }
}

TeleFile::~TeleFile() {
        for (int i = 0; i < NB_FRAGMENT_MAX; i++) {
            delete[] CODED_F_MEM[i];
        }
        delete[] CODED_F_MEM;
}

unsigned TeleFile::getFragmentSize() {
    return fragmentSize;
}

unsigned TeleFile::computeCodedSize(unsigned lenIn) {

    unsigned numberOfUncodedFragments = int(lenIn / fragmentSize);
    double lenDouble = lenIn;
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

    //Serial.print("Encoding a new packet with size : ");
    //Serial.print(lenIn);
    //Serial.print(" and fragment size : ");
    //Serial.print(fragmentSize);
    //Serial.print(" and coding rate : ");
    //Serial.print(codingRate);
    //Serial.print(" the number of uncoded fragments is : ");
    //Serial.print(numberOfUncodedFragments);
    //Serial.print(" and the number of coded fragments is : ");
    //Serial.println(numberOfCodedFragments); 

    byte UNCODED_F[numberOfUncodedFragments][fragmentSize];
    for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
        for (unsigned j(1); j <= fragmentSize; j++) {
            UNCODED_F[i-1][j-1] = dataIn[(i-1)*fragmentSize+(j-1)];
        }
    }

    byte CODED_F[numberOfCodedFragments][fragmentSize];
    for (unsigned y(1); y <= numberOfCodedFragments; y++) {
        byte s[fragmentSize]; 
        bool A[numberOfUncodedFragments];
        for (unsigned i(1); i <= fragmentSize; i++) {
            s[i-1] = 0;
        }
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            A[i-1] = 0;
        }
        /* //Serial.println("Before");
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            //Serial.print(A[i-1]); 
        }  */
        matrixLine(y,numberOfUncodedFragments,A);
        // Print the content of A to make sure it's not the same eveytime
        /* //Serial.println("After");
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            //Serial.print(A[i-1]); 
        } 
        //Serial.println(); */

        // For all fragments
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
    // Just for fun, we print the coded fragments
    /* //Serial.println("Coded Fragments: ");
    for (unsigned i(1); i <= numberOfCodedFragments; i++) {
        for (unsigned j(1); j <= fragmentSize; j++) {
            //Serial.print(CODED_F[i-1][j-1]); 
            if (j != fragmentSize) {
            //Serial.print(",");
            }
        }
        //Serial.println("");
    }
    //Serial.println("End of Coded Fragments"); */

    for (unsigned i(1); i <= numberOfCodedFragments; i++) {
        for (unsigned j(1); j <= fragmentSize; j++) {
            dataOut[(i-1)*fragmentSize+(j-1)] = CODED_F[i-1][j-1];
        }
    }
}

void TeleFile::decode(byte dataIn[], unsigned len) {
    ////Serial.println("Entering decode function");

    // Cast the first two bytes of the dataIn array as uint16_t in fragmentNumber
    uint16_t fragmentNumberRaw = dataIn[0] << 8 | dataIn[1];
    const unsigned fragmentNumber = unsigned(fragmentNumberRaw);
    // Cast the next two bytes of the dataIn array as uint16_t in numberOfCodedFragments
    uint16_t numberOfUncodedFragmentsRaw = dataIn[2] << 8 | dataIn[3];
    const unsigned numberOfUncodedFragments = unsigned(numberOfUncodedFragmentsRaw);

    ////Serial.println("A");

    ////Serial.print("Metada of the fragment is : ");
    ////Serial.print(fragmentNumber);
    ////Serial.print(" ");
    ////Serial.println(numberOfUncodedFragments);

    static unsigned indexOrderLen = 0;
    static unsigned indexLen = 0;

    ////Serial.println("B");

    bool A[numberOfUncodedFragments];
    for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
        A[i-1] = 0;
    }

    ////Serial.println("C");

    matrixLine(unsigned(fragmentNumber),unsigned(numberOfUncodedFragments),A);

    ////Serial.println("D");

    // Print the content of A
    /* //Serial.println("Line is : ");
    for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
        //Serial.print(A[i-1]); 
    }
    //Serial.println(""); */

    byte fragMemory[fragmentSize];

    ////Serial.println("E");

    for (unsigned i(1); i<=fragmentSize; i++) {
        fragMemory[i-1] = dataIn[i+3];
    }

    ////Serial.println("F");

    if (fragmentNumber<lastFrameNumber) {
        //Serial.println("New frame detected");
        currentStatus = RECEIVING_FRAMES;
        // Reset index to 0
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            index[i-1] = 0;
        }

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

        /* // We will print a bunch of stuff: the index, the indexLen, the indexOrder, the inderOrderLen, the combinationMatrix, the CODED_F_MEM
        //Serial.println("Index is : ");
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            //Serial.print(index[i-1]); 
        }
        //Serial.println("");

        //Serial.print("IndexLen is : ");
        //Serial.println(indexLen);
        
        //Serial.println("IndexOrder is : ");
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            //Serial.print(indexOrder[i-1]); 
        }
        //Serial.println("");

        //Serial.print("IndexOrderLen is : ");
        //Serial.println(indexOrderLen);

        //Serial.println("Combination Matrix is : ");
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            for (unsigned j(1); j <= numberOfUncodedFragments; j++) {
                //Serial.print(combinationMatrix[i-1][j-1]); 
                if (j != numberOfUncodedFragments) {
                //Serial.print(",");
                }
            }
            //Serial.println("");
        }

        //Serial.println("CODED_F_MEM is : ");
        for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
            for (unsigned j(1); j <= fragmentSize; j++) {
                //Serial.print(CODED_F_MEM[i-1][j-1]); 
                if (j != fragmentSize) {
                //Serial.print(",");
                }
            }
            //Serial.println("");
        }*/

        //delay(10000000);
        
    } 
    else if (currentStatus == RECEIVING_FRAMES) {
        ////Serial.println("'bout to decode the frame :)");
        static unsigned o;
        static unsigned col;
        //Serial.print("Lenght of index : "); //Serial.println(indexLen);
        for (unsigned i(1); i<=indexLen; i++) {
            ////Serial.println(" A ");
            o = indexOrder[i-1];
            ////Serial.println(" B ");
            col = index[o-1];
            //Serial.print("i-1 : "); //Serial.println(i-1);
            //Serial.print("o : "); //Serial.println(o);
            //Serial.print("col : "); //Serial.println(col);

            ////Serial.print(indexLen);
            ////Serial.print(" ");
            ////Serial.print(o);
            ////Serial.print(" ");
            ////Serial.print(col);

            // Print indx
            //Serial.print("idx :");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                //Serial.print(index[i-1]); 
            }
            //Serial.println("");

            // Print indexOrder
            //Serial.print("idxorder :");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                //Serial.print(indexOrder[i-1]); 
            } 
            //Serial.println("");

            //if(indexLen>4) {
                //delay(100000000);
            //}

            if (A[col-1] == 1) {
                //Serial.print("For fragment "); //Serial.print(fragmentNumber); //Serial.print(" eliminating col "); //Serial.println(col);
                
                /* //Serial.print("A before the xor is : ");
                for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                    //Serial.print(A[i-1]); 
                }
                //Serial.println("");  */
                
                for (unsigned j(1); j <= numberOfUncodedFragments; j++) {
                    //Serial.print(A[j-1]); //Serial.print(" ^ "); //Serial.print(combinationMatrix[col-1][j-1]); //Serial.print(" = "); //Serial.println(A[j-1] ^ combinationMatrix[col-1][j-1]);
                    A[j-1] = A[j-1] ^ combinationMatrix[col-1][j-1];
                }

                for (unsigned j(1); j <= fragmentSize; j++) {
                    fragMemory[j-1] = fragMemory[j-1] ^ CODED_F_MEM[col-1][j-1];
                }
                // We want to print a bunch of things: o, col, A, fragMemory
                /* //Serial.print("o is : "); //Serial.println(o);
                //Serial.print("col is : "); //Serial.println(col);

                //Serial.print("A after the xor is : ");
                for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                    //Serial.print(A[i-1]); 
                }
                //Serial.println("");

                //Serial.println("FragMemory is : ");
                for (unsigned i(1); i <= fragmentSize; i++) {
                    //Serial.print(fragMemory[i-1]); 
                }
                delay(1000000000); */
            }
        }
        if (!isEmpty(A, numberOfUncodedFragments)) {
            //Serial.print("Adding fragment "); //Serial.print(fragmentNumber); //Serial.println(" to the memory");
            
            // Print index
            //Serial.print(" idx just before record adding ");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                //Serial.print(index[i-1]); 
            }
            //Serial.println("");

            
            unsigned resultFind = findFirstOne(A,numberOfUncodedFragments);
            
            //Serial.print("resultFind is : "); //Serial.println(resultFind);
            
            index[indexLen++] = resultFind;

            indexOrderLen++;

            // Print A
            //Serial.print("A is : ");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                //Serial.print(A[i-1]); 
            }
            //Serial.println("");

            // Print index
            //Serial.print(" idx just after adding ");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                //Serial.print(index[i-1]); 
            }
            //Serial.println("");
            
            // Print indexOrder
            //Serial.print("idxorder just after adding fragment : ");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
               //Serial.print(indexOrder[i-1]); 
            }
            //Serial.println("");

            for (unsigned i(1); i <= fragmentSize; i++) {
                CODED_F_MEM[index[indexLen-1]-1][i-1] = fragMemory[i-1];
            }
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
                combinationMatrix[index[indexLen-1]-1][i-1] = A[i-1];
            }

            //Serial.print("The sort is done on a lenght of : ");
            //Serial.println(indexLen);
            sortIndex(index, indexLen, indexOrder);

            // Print indexOrder
            //Serial.print("idxorder just after sort: ");
            for (unsigned i(1); i <= numberOfUncodedFragments; i++) {
               //Serial.print(indexOrder[i-1]); 
            }
            //Serial.println("");

            if (indexLen == numberOfUncodedFragments) {
                //Serial.println("We received enough fragments to decode the file");
                for (unsigned k(numberOfUncodedFragments-1); k >= 1; k--) {
                    o = indexOrder[k-1];
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
                for (unsigned i(1); i<=numberOfUncodedFragments; i++) {
                    for (unsigned j(1); j<=fragmentSize; j++) {
                        dataOutput[(i-1)*fragmentSize+j-1] = CODED_F_MEM[i-1][j-1];
                    }
                }
                currentStatus = SUCCESS_IDLE;
                functionCallBack(dataOutput, numberOfUncodedFragments*fragmentSize);
            }
        }
        lastFrameNumber = fragmentNumber;
    }
}


bool isEmpty(bool data[], unsigned len) {
    for (unsigned i(1); i <= len; i++) {
        if (!data[i-1]) {
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

    /* //Serial.println("_______________________________________");
    
    //Serial.println("dataArrayInRaw is : ");

    for (unsigned i(1); i <= len; i++) {
        //Serial.print(dataArrayInRaw[i-1]); 
    }
    //Serial.println("");

    //Serial.println("dataOrderInRaw is : ");

    for (unsigned i(1); i <= len; i++) {
        //Serial.print(dataOrderInRaw[i-1]); 
    }
    //Serial.println(""); */

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
                ////Serial.print("Swap detected : "); //Serial.print(dataArrayIn[i-1]); //Serial.print(" is smaller than "); //Serial.println(dataArrayIn[j-1]);
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

    /* //Serial.println("dataArray out is : ");
    for (unsigned i(1); i <= len; i++) {
        //Serial.print(dataArrayIn[i-1]); 
    }
    //Serial.println("");

    //Serial.println("dataOrder out is : ");
    for (unsigned i(1); i <= len; i++) {
        //Serial.print(dataOrderIn[i-1]); 
    }
    //Serial.println("");
    //Serial.println("_______________________________________"); */
}

// This function should return the line N of the parity check matrix NxM
// This matrix contains 0 and 1 and is used to then create the linear combination of fragments. 
// For example, if the matrixLine functions returns 1 0 1 0 1 0 1 .... 0 1 then the combination of fragments
// will be the sum (XOR) of the first fragment, the third fragment, the fifth fragment, the seventh fragment, etc...
void matrixLine(unsigned N, unsigned M, bool matrixLineResult[]) {

    int m(0);

    if (M==32) {
        m=32;
    }
    else {
        m=64;
    }

    // This variable will be a key to get the same coding and decoding parity matrix on both sides
    // The key is different for each total number of fragments.
    unsigned initV[64];

    initV[32]=25;
    initV[40]=7;
    initV[48]=17;
    initV[56]=13;
    initV[64]=2;

    unsigned x = initV[32]+260*N;
    unsigned nbCoeff = 0;

    while (nbCoeff<M/2) {
        unsigned r = 255;
        while (r>=M) {
            x = prbs16(x);
            r = x - (x/m)*m;
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
    //unsigned b = ((x >> 0) ^ (x >> 2) ^ (x >> 3) ^ (x >> 5)) & 1;
    //return (x >> 1) | (b << 15);
}


