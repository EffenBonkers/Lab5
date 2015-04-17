#include "stdafx.h"
#include "DTControl.h"

HDEV DTControl::hDriver = NULL;
bool DTControl::boardInitialized = false;
DTControl::Parameters DTControl::params = { 0, "NULL", "NULL" };

HDASS AnalogSubsystem::hdassAnalog = NULL;
HBUF AnalogSubsystem::hBuffer = NULL;
bool AnalogSubsystem::initialized = false;
bool AnalogSubsystem::isContinuous = false;

HDASS DigitalSubsystem::hdassDigitalIn = NULL;
HDASS DigitalSubsystem::hdassDigitalOut = NULL;
bool DigitalSubsystem::initialized = false;

#pragma region DTControl Implementation
/*
this is a callback function of olDaEnumBoards, it gets the 
strings of the Open Layers board and attempts to initialize
the board.  If successful, enumeration is halted.
*/
BOOL CALLBACK DTControl::GetDriver( LPSTR lpszBoardName, LPSTR lpszDriverName, LPARAM lParam )   
{
   /* fill in board strings */
   lstrcpynA(params.name,lpszBoardName,MAX_BOARD_NAME_LENGTH-1);
   lstrcpynA(params.driver,lpszDriverName,MAX_BOARD_NAME_LENGTH-1);

   /* try to open board */
   params.status = olDaInitialize((LPTSTR)lpszBoardName, &hDriver);
   if (hDriver != NULL)
      return FALSE;          /* false to stop enumerating */
   else                      
      return TRUE;           /* true to continue          */
}

void DTControl::ErrorCheck(ECODE ecode)
{
    if ((params.status = ecode) != OLNOERROR)
        throw ecode;
}

void DTControl::InitializeBoard()
{
    ECODE ecode;
    if (boardInitialized)
        return;

    /* Get first available Open Layers board */
    hDriver = NULL;
    if ((ecode = olDaEnumBoards((DABRDPROC) GetDriver, NULL)) != OLNOERROR)
        throw ecode;

    /* check for error within callback function */
    if (params.status != OLNOERROR)
        throw ecode;

    /* check for NULL driver handle - Means no boards */
    if (hDriver == NULL)
        throw "No Open Layer Boards Detected.";

    boardInitialized = true;
}

void DTControl::ReleaseBoard()
{
    if (hDriver != NULL)
        olDaTerminate(hDriver);
}
#pragma endregion

#pragma region AnalogSubsystem Implementation
AnalogSubsystem::AnalogSubsystem()
{
    params.freq = 1000.0;
}

void AnalogSubsystem::Initialize(bool continuous)
{
    try {
        InitializeBoard();

        if (initialized)
            return;

#pragma region Get Handle to ADC subsystem 
        /* get handle to first available ADC sub system */
        ErrorCheck(olDaGetDevCaps(hDriver, OLDC_ADELEMENTS, &params.numberADs));

        UINT currentAD = 0;
        UINT ec;
        // Enumerate through all the A/D subsystems and try and select the first available one...
        while (1)
        {
            ec = olDaGetDASS(hDriver, OLSS_AD, currentAD, &hdassAnalog);
            if (ec != OLNOERROR)
            {
                // busy subsys etc...
                currentAD++;
                if (currentAD > params.numberADs)
                {
                    throw "No Available A/D subsystems.";
                }

            }
            else
                break;
        }
#pragma endregion

        isContinuous = continuous;
        /* Configure for acquisition type */
        if (isContinuous)
            InitializeContinuous();
        else
            InitializeSingle();

        ErrorCheck(olDaConfig(hdassAnalog));    //configure subsystem

#pragma region Setup buffers and put buffer in ready queue
        if (isContinuous) {
            //set sample period
            double buffer_size = params.freq / SAMPLE_PERIOD_DIVISOR;
            int sample_size;

            //Get device resolution and set appropriate sample size
            ErrorCheck(olDaGetResolution(hdassAnalog, &params.resolution));
            if (params.resolution > 16)
                sample_size = 4;        //4 bytes (4*8=32bits)
            else
                sample_size = 2;        //2 bytes (2*8=16bits) DT9816 should use this value

            //Allocate each buffer then place in the ready queue
            for (int i = 0; i < NUM_BUFFERS; i++)
            {
                ErrorCheck(olDmCallocBuffer(0, 0, (ULNG)buffer_size, sample_size, &hBuffer));
                ErrorCheck(olDaPutBuffer(hdassAnalog, hBuffer));
            }


        }
#pragma endregion
    }
    catch (ECODE e) { throw; }
    catch (char *s) { throw; }

}

void AnalogSubsystem::InitializeSingle()
{
    try {
        if (params.numChannels == 0)
            throw "No Analog channels present.";

        ErrorCheck (olDaSetDataFlow(hdassAnalog,OL_DF_SINGLEVALUE));
    }
    catch (...) { throw; }
}

void AnalogSubsystem::InitializeContinuous()
{
    try {
        if (params.numChannels == 0)
            throw "No Analog channels present.";



#pragma region Get Capabilities and Configure Parameters
        /* Get device capabilities */
        double maxfreq;
        ErrorCheck(olDaGetSSCapsEx(hdassAnalog, OLSSCE_MAXTHROUGHPUT, &maxfreq));
        ErrorCheck(olDaGetSSCaps(hdassAnalog, OLSSC_NUMDMACHANS, &params.dma));
        ErrorCheck(olDaGetSSCaps(hdassAnalog, OLSSC_SUP_PROGRAMGAIN, &params.hasGain));

        params.dma = min(1, params.dma);
        params.freq = min(params.freq, maxfreq);

        /* Read data continuously */
        ErrorCheck(olDaSetDataFlow(hdassAnalog, OL_DF_CONTINUOUS));
        /* Fill multiple buffers at once. Start over with first buffer, once full.
         * Other modes: OL_WRP_NONE (stops after full), OL_WRP_SINGLE (one buffer at a time)
         */
        ErrorCheck(olDaSetWrapMode(hdassAnalog, OL_WRP_MULTIPLE));
        ErrorCheck(olDaSetClockFrequency(hdassAnalog, params.freq));
        ErrorCheck(olDaSetDmaUsage(hdassAnalog, params.dma));
        ErrorCheck(olDaSetEncoding(hdassAnalog, OL_ENC_BINARY));
#pragma endregion

#pragma region Set and configure channels
        ErrorCheck(olDaSetChannelListSize(hdassAnalog, params.numChannels));
        UINT i = 0;
        for (i = 0; i < params.numChannels; i++)
            ErrorCheck(olDaSetChannelListEntry(hdassAnalog, i, params.channelList[i]));
            ErrorCheck(olDaSetGainListEntry(hdassAnalog, i, params.gainList[i]));
#pragma endregion
    }
    catch (ECODE e) { throw e; }
}

void AnalogSubsystem::Start(OLNOTIFYPROC lpfnNotifyProc)
{
    static DWORD lParam;
    try {
        if (isContinuous) {
            //setup callback
            ErrorCheck(olDaSetNotificationProcedure(hdassAnalog, lpfnNotifyProc, (LPARAM) &lParam));
            //reconfigure to ensure DTA sets up callback
            ErrorCheck(olDaConfig(hdassAnalog));
            //start receiving data
            ErrorCheck(olDaStart(hdassAnalog));
        } else
            ErrorCheck(olDaConfig(hdassAnalog));
    }
    catch (ECODE e) { throw; }
}

void AnalogSubsystem::GetValue(long *value)
{
    olDaGetSingleValue(hdassAnalog, value, params.channelList[0], 1.0);
}

void AnalogSubsystem::GetValues(long *values)
{
    ULNG valid_samples;
    PWORD pBuffer;
    PDWORD pBuffer32;

    try {
        if (isContinuous) {
            //Get Buffer off done list
            ErrorCheck(olDaGetBuffer(hdassAnalog, &hBuffer));

            //if there is a buffer
            if (hBuffer)
            {
                /* get max samples in input buffer */
                ErrorCheck(olDmGetValidSamples(hBuffer, &valid_samples));

                /* get pointer to the buffer */
                if (params.resolution > 16)
                {
                    ErrorCheck(olDmGetBufferPtr(hBuffer, (LPVOID*)&pBuffer32));
                    /* get samples from buffer */
                    for (UINT i = 0; i < params.numChannels; i++)
                        values[i] = pBuffer32[valid_samples - i - 1];
                }
                else
                {
                    ErrorCheck(olDmGetBufferPtr(hBuffer, (LPVOID*)&pBuffer));
                    /* get samples from buffer */
                    for (UINT i = 0; i < params.numChannels; i++)
                        values[i] = pBuffer[valid_samples - i - 1];
                }

                /* put buffer back to ready list */
                ErrorCheck(olDaPutBuffer(hdassAnalog, hBuffer));

            }
        }
        else {
            for (UINT i = 0; i < params.numChannels; ++i)
                ErrorCheck(olDaGetSingleValue(hdassAnalog, &values[i], params.channelList[i], params.gainList[i]));
        }
    }
    catch (ECODE e) { throw; }
}

void AnalogSubsystem::Release()
{
    try {
        if (hdassAnalog != NULL) {
            if (isContinuous) {
                ErrorCheck(olDaFlushBuffers(hdassAnalog));

                for (int i = 0; i < NUM_BUFFERS; i++) {
                    ErrorCheck(olDaGetBuffer(hdassAnalog, &hBuffer));
                    ErrorCheck(olDmFreeBuffer(hBuffer));
                }
            }

            ErrorCheck(olDaReleaseDASS(hdassAnalog));
        }
    } catch (ECODE e) { throw; }
}
#pragma endregion

#pragma region DigitalSubsystem Implementations

void DigitalSubsystem::Initialize()
{
    try {
        // get handle to DIN subsystem
        ErrorCheck(olDaGetDASS(hDriver, OLSS_DIN, 0, &hdassDigitalIn));
        //single value data transfer
        ErrorCheck(olDaSetDataFlow(hdassDigitalIn, OL_DF_SINGLEVALUE));
        //resolution of 8 bits
        ErrorCheck(olDaSetResolution(hdassDigitalIn, params.resolution));
        ErrorCheck(olDaConfig(hdassDigitalIn));

        // get handle to DOUT subsystem
        ErrorCheck(olDaGetDASS(hDriver, OLSS_DOUT, 0, &hdassDigitalOut));
        //single value data transfer
        ErrorCheck(olDaSetDataFlow(hdassDigitalOut, OL_DF_SINGLEVALUE));
        //resolution of 8 bits
        ErrorCheck(olDaSetResolution(hdassDigitalOut, params.resolution));
        ErrorCheck(olDaConfig(hdassDigitalOut));
    }
    catch (ECODE e) { throw; }
}

void DigitalSubsystem::Start()
{
    // nothing needed for single value operation
}

void DigitalSubsystem::GetValue(long *value)
{
    try {
        ErrorCheck(olDaGetSingleValue(hdassDigitalIn, value, 0, 1.0));
    }
    catch (ECODE e) { throw; }
}

void DigitalSubsystem::PutValue(long &value)
{
    try {
        ErrorCheck(olDaPutSingleValue(hdassDigitalOut, value, 0, 1.0));
    }
    catch (ECODE e) { throw; }
}

void DigitalSubsystem::Release()
{
    try {
        if (hdassDigitalIn != NULL)
            ErrorCheck(olDaReleaseDASS(hdassDigitalIn));
        if (hdassDigitalOut != NULL)
            ErrorCheck(olDaReleaseDASS(hdassDigitalOut));
    }
    catch (ECODE e) { throw; }
}
#pragma endregion
