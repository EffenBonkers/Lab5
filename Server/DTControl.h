#include <olmem.h>         
#include <olerrors.h>         
#include <oldaapi.h>

#define SAMPLE_PERIOD_DIVISOR   1
#define NUM_BUFFERS             4
#define MAX_CHANNEL_SIZE        20

class DTControl {
private:
    static BOOL CALLBACK GetDriver( LPSTR lpszName, LPSTR lpszEntry, LPARAM lParam );
protected:
    static HDEV hDriver;
    static bool boardInitialized;
    void ErrorCheck(ECODE ecode);
public:
    static struct Parameters {
        ECODE status;
        char name[MAX_BOARD_NAME_LENGTH];
        char driver[MAX_BOARD_NAME_LENGTH];
    } params;

    void InitializeBoard();
    void ReleaseBoard();
};

class AnalogSubsystem : public DTControl {
private:
    static HDASS hdassAnalog;
    static bool initialized;
    static bool isContinuous;
    static HBUF hBuffer;
    void InitializeContinuous();
    void InitializeSingle();
public:
    struct Parameters {
        double gainList[MAX_CHANNEL_SIZE];
        unsigned int channelList[MAX_CHANNEL_SIZE];
        double min, max;
        double freq;
        unsigned int dma;
        unsigned int resolution;
        unsigned int encoding;
        unsigned int hasGain;
        unsigned int numChannels;
        unsigned int numberADs;
    } params;

    AnalogSubsystem();
    void Initialize(bool continuous);
    void Start(OLNOTIFYPROC lpfnNotifyProc);
    void GetValues(long *values);
    void GetValue(long *value);
    void Release();
};

class DigitalSubsystem : public DTControl {
private:
    static HDASS hdassDigitalIn;
    static HDASS hdassDigitalOut;
    static bool initialized;
public:
    struct Parameters {
        double gain;
        unsigned int resolution;
    } params;

    void Initialize();
    void Start();
    void GetValue(long *value);
    void PutValue(long &value);
    void Release();
};

