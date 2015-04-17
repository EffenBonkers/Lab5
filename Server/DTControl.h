#include <olmem.h>         
#include <olerrors.h>         
#include <oldaapi.h>
#include <list>

using namespace std;

#define SAMPLE_PERIOD_DIVISOR   1
#define NUM_BUFFERS             4
#define MAX_CHANNEL_SIZE        20

class DTControl {
private:
    static BOOL CALLBACK GetDriver( LPSTR lpszName, LPSTR lpszEntry, LPARAM lParam );
protected:
    static HDEV hDriver;
    void ErrorCheck(ECODE ecode);
public:
    static ECODE status;
    static char name[MAX_BOARD_NAME_LENGTH];
    static char driver[MAX_BOARD_NAME_LENGTH];
    void InitializeBoard();
    void ReleaseBoard();
};

class AnalogIn : public DTControl{
protected:
    static HDASS hdassAnalog;
public:
    UINT resolution;
    UINT encoding;
    DBL gain;
    list<UINT> channelList;
};

class AnalogInSingleValue : public AnalogIn {
public:
    void Initialize();
    long GetValue();
    void GetValues(long *values);
};

class AnalogInContinuous : public AnalogIn {
private:
    static OLNOTIFYPROC NotifyProc(HDASS hdass, UINT msg, LPARAM lParam, WPARAM wParam);
    static HBUF hBuffer;
    UINT dma;
    UINT numberADs;
    DBL freq;
    BOOL hasGain;
public:
    void Initialize();
    void Start();
    long GetValue();
    void GetValues(long *values);
    void Release();
};

class DigitalIn : public DTControl {
private:
    static HDASS hdassDigitalIn;
public:
    UINT gain;
    UINT resolution;

    void Initialize();
    void Release();
    long GetValue();
};

class DigitalOut : public DTControl {
private:
    static HDASS hdassDigitalOut;
public:
    UINT gain;
    UINT resolution;

    void Initialize();
    void PutValue(long &value);
    void Release();
};

