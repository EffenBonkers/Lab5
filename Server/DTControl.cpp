#include "stdafx.h"
#include "DTControl.h"

HDEV DTControl::hDriver = NULL;
ECODE DTControl::status = 0;
char DTControl::name[MAX_BOARD_NAME_LENGTH] = "NULL";
char DTControl::driver[MAX_BOARD_NAME_LENGTH] = "NULL";

HDASS AnalogIn::hdassAnalog = NULL;
HBUF AnalogInContinuous::hBuffer = NULL;

HDASS DigitalIn::hdassDigitalIn = NULL;
HDASS DigitalOut::hdassDigitalOut = NULL;

#pragma region DTControl Implementation
/*
this is a callback function of olDaEnumBoards, it gets the 
strings of the Open Layers board and attempts to initialize
the board.  If successful, enumeration is halted.
*/
BOOL CALLBACK DTControl::GetDriver( LPSTR lpszBoardName, LPSTR lpszDriverName, LPARAM lParam )   
{
   /* fill in board strings */
   lstrcpynA(name,lpszBoardName,MAX_BOARD_NAME_LENGTH-1);
   lstrcpynA(driver,lpszDriverName,MAX_BOARD_NAME_LENGTH-1);

   /* try to open board */
   status = olDaInitialize((LPTSTR)lpszBoardName, &hDriver);
   if (hDriver != NULL)
      return FALSE;          /* false to stop enumerating */
   else                      
      return TRUE;           /* true to continue          */
}

void DTControl::ErrorCheck(ECODE ecode)
{
    if ((status = ecode) != OLNOERROR)
        throw ecode;
}

void DTControl::InitializeBoard()
{
    ECODE ecode;
    if (hDriver != NULL)
        return;

    /* Get first available Open Layers board */
    hDriver = NULL;
    if ((ecode = olDaEnumBoards((DABRDPROC) GetDriver, NULL)) != OLNOERROR)
        throw ecode;

    /* check for error within callback function */
    if (status != OLNOERROR)
        throw ecode;

    /* check for NULL driver handle - Means no boards */
    if (hDriver == NULL)
        throw "No Open Layer Boards Detected.";
}

void DTControl::ReleaseBoard()
{
    if (hDriver != NULL)
        olDaTerminate(hDriver);
}
#pragma endregion

#pragma region AnalogSubsystem Implementation
void AnalogInSingleValue::Initialize()
{
    try {
        InitializeBoard();

        if (hdassAnalog != NULL)
            return;

        if (channelList.size() == 0)
            throw "No Analog channels present.";

        ErrorCheck(olDaGetDASS(hDriver, OLSS_AD, 0, &hdassAnalog));
        ErrorCheck(olDaSetDataFlow(hdassAnalog, OL_DF_SINGLEVALUE));
        ErrorCheck(olDaConfig(hdassAnalog));
    }
    catch (ECODE e) { throw e; }
}

long AnalogInSingleValue::GetValue()
{
    long result;
    ErrorCheck(olDaGetSingleValue(hdassAnalog, &result, channelList.front(), gain));

    return result;
}

void AnalogInSingleValue::GetValues(long *values)
{
    int i = 0;
    for (auto channel = channelList.begin(); channel != channelList.end(); ++channel)
        ErrorCheck(olDaGetSingleValue(hdassAnalog, &values[i++], *channel, gain));
}

void AnalogSubsystem::Initialize(bool continuous)
{
    try {
        InitializeBoard();

        if (initialized)
            return;
 
        ErrorCheck(olDaGetDASS(hDriver, OLSS_AD, 0, &hdassAnalog);




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
        for (i = 0; i < params.numChannels; i++) {
            ErrorCheck(olDaSetChannelListEntry(hdassAnalog, i, params.channelList[i]));
            ErrorCheck(olDaSetGainListEntry(hdassAnalog, i, params.gainList[i]));
        }
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
