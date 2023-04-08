#include "Arduino.h"

#define NB_FRAGMENT_MAX 250
#define FRAGMENT_SIZE_MAX 100

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
    unsigned getFragmentSize();
  private:
    unsigned fragmentSize;
    double codingRate;
    void (*functionCallBack)(byte [], unsigned);
    byte   CODED_F_MEM[NB_FRAGMENT_MAX][FRAGMENT_SIZE_MAX];
    byte   combinationMatrix[NB_FRAGMENT_MAX][NB_FRAGMENT_MAX];
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