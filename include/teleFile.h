#include "Arduino.h"

#define NB_FRAGMENT_MAX 500
#define FRAGMENT_SIZE_MAX 100

#define BITS_PER_ROW (NB_FRAGMENT_MAX + 7)/8

// The max lenght of the transmitted file will be (FRAGMENT_SIZE_MAX * NB_FRAGMENT_MAX) ~ 100 000 bytes

enum status {
    WAITING = 0,
    RECEIVING_FRAMES,
    SUCCESS_IDLE,
};

class TeleFile {
  public:
    TeleFile(unsigned, double, void (*function)(byte [], unsigned));
    ~TeleFile();
    void decode(byte [], unsigned);
    void encode(byte [], unsigned, byte[]);
    unsigned computeCodedSize(unsigned lenIn);
    unsigned computeUncodedSize(unsigned lenIn);
    unsigned getFragmentSize();
    unsigned getNumberOfCodedFragments();
    unsigned getNumberOfUncodedFragments();
    unsigned getLastEndTime();
    unsigned getLastPacketSent();
    void     setLastPacketSent(unsigned);
    bool     isTransmissionOver();
    void     setTransmissionOver(bool);
    void     setEndTime(unsigned long);
    void     endTransmission();
  private:
    unsigned fragmentSize;
    unsigned numberOfCodedFragments;
    unsigned numberOfUncodedFragments;
    double codingRate;
    unsigned long lastEndTime;
    bool transmissionOver;
    unsigned lastPacketSent;
    void (*functionCallBack)(byte [], unsigned);
    byte   CODED_F_MEM[NB_FRAGMENT_MAX][FRAGMENT_SIZE_MAX];
    
    byte combinationBitMatrix[NB_FRAGMENT_MAX][BITS_PER_ROW];

    void setCombinationBitMatrix(unsigned, unsigned, bool);
    bool getCombinationBitMatrix(unsigned, unsigned);
    void resetCombinationBitMatrix();

    unsigned lastFrameNumber;
    unsigned index[NB_FRAGMENT_MAX];
    unsigned indexOrder[NB_FRAGMENT_MAX];
    status currentStatus;
    byte STOP_BYTE = 0xFE;
};

int prbs16(int);
void matrixLine(unsigned, unsigned, bool []);
bool isEmpty(bool [], unsigned);
unsigned findFirstOne(bool [], unsigned);
void sortIndex(unsigned [], unsigned, unsigned []);