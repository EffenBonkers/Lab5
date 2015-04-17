// Server.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Server.h"
#include "TCPComm.h"
#include "DTControl.h"


#define MAX_LOADSTRING  100
#define DEFAULT_PORT    "27015"
#define DEFAULT_IP      "127.0.0.1"
#define MESSAGE_SIZE    150
#define BUFSIZE         256

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
bool startServer;
static HWND g_hDlg;
TCPCommServer server;
unsigned int numConnections;
long value;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	DTControlServer(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI        RunServer(LPVOID lParam);
DWORD WINAPI        Connection(LPVOID lParam);
void CALLBACK NotifyProc (UINT uiMsg, HDASS wParam, LPARAM lParam);
AnalogSubsystem analog;
DigitalSubsystem digital;

int SetText(UINT item_id, LPWSTR text)
{
    return SendMessage(GetDlgItem(g_hDlg, item_id), WM_SETTEXT, 0, (LPARAM) text);
}

int AppendText(UINT item_id, LPWSTR text)
{
    return SendMessage(GetDlgItem(g_hDlg, item_id), EM_REPLACESEL, 0, (LPARAM) text);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SERVER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SERVER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SERVER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_CREATE:
        DialogBox(hInst, MAKEINTRESOURCE(IDD_SERVER), hWnd, DTControlServer);
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK DTControlServer(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static DWORD threadServerID;
    static HANDLE hThreadServer;
    g_hDlg = hDlg;

	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:
        startServer = false;
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDSTARTSTOP)
		{
			if (!startServer) {
                SetText(IDSTARTSTOP, L"STOP");
                startServer = true;

                hThreadServer = CreateThread(   
                                    NULL,
                                    0,
                                    RunServer,
                                    NULL,
                                    0,
                                    &threadServerID);
            } else {
                SetText(IDSTARTSTOP, L"START");         
                server.Shutdown();
                AppendText(IDC_LOG, L"Server stopped.\r\n");
                startServer = false;
            }

			return (INT_PTR)TRUE;
		}
		break;
    case WM_CLOSE:
        startServer = false;
        WaitForSingleObject(hThreadServer, 100);
        EndDialog(hDlg, LOWORD(wParam));
        PostQuitMessage(0);
        return (INT_PTR) TRUE;
	}
	return (INT_PTR)FALSE;
}

DWORD WINAPI RunServer(LPVOID lParam)
{
    HANDLE hThreadConnections[MAXCONNECTIONS];
    DWORD threadStatus;
    TCHAR msg[MESSAGE_SIZE];
    TCHAR temp1[MESSAGE_SIZE], temp2[MESSAGE_SIZE];

    ZeroMemory(hThreadConnections, MAXCONNECTIONS);
    try {
        //initialize DT9816 first
        analog.params.channelList[0] = 0;
        analog.params.gainList[0] = 1.0;
        analog.params.numChannels++;

        analog.params.resolution = 16;
        analog.Initialize(true);
        mbstowcs(temp1, DTControl::params.name, MAX_BOARD_NAME_LENGTH);
        mbstowcs(temp2, DTControl::params.driver, MAX_BOARD_NAME_LENGTH);
        swprintf(msg, MESSAGE_SIZE, L"Device: %s (%s) +INITIALIZED\r\n", temp1, temp2);
        AppendText(IDC_LOG, msg);
        AppendText(IDC_LOG, L"Analog Subsystem +INITIALIZED\r\n");

        digital.params.resolution = 8;
        digital.params.gain = 1.0;
        digital.Initialize();
        AppendText(IDC_LOG, msg);
        AppendText(IDC_LOG, L"Digital Subsystem +INITIALIZED\r\n");

        mbstowcs(temp1, DEFAULT_PORT, MESSAGE_SIZE);
        mbstowcs(temp2, DEFAULT_IP, MESSAGE_SIZE);
        server.StartServer(DEFAULT_PORT, DEFAULT_IP);
        swprintf(msg, MESSAGE_SIZE, L"Server Started on %s: %s\r\n", temp1, temp2);
        AppendText(IDC_LOG, msg);

        numConnections = 0;
        while (startServer) {
            int iConn = server.Connect();
            swprintf(msg, MESSAGE_SIZE, L"Connection %d made.\r\n", iConn);
            AppendText(IDC_LOG, msg);

            if (numConnections == MAXCONNECTIONS)
                break;

            //find available connection
            int i;
            for (i = 0; i < MAXCONNECTIONS; i++) {
                if (hThreadConnections[i] == NULL)
                    break;
                GetExitCodeThread(hThreadConnections[i], &threadStatus);
                if (threadStatus != STILL_ACTIVE)
                    break;
            }

            hThreadConnections[i] = CreateThread( NULL, 0, Connection,
                                                  (LPVOID) iConn, 0, NULL);
            ++numConnections;
        }

        WaitForMultipleObjects(numConnections, hThreadConnections, TRUE, 250);
        for (int i = 0; i < MAXCONNECTIONS; ++i) {
            if (hThreadConnections[i] != NULL)
                CloseHandle(hThreadConnections[i]);
        }

        server.Shutdown();
        swprintf(msg, MESSAGE_SIZE, L"Server Shutdown\r\n");
        AppendText(IDC_LOG, msg);
    }
    catch (ECODE e) {
        char error[MESSAGE_SIZE];
        olDaGetErrorString(e, (PTSTR) error, MESSAGE_SIZE);
        mbstowcs(temp1, error, MESSAGE_SIZE);
        swprintf(msg, MESSAGE_SIZE, L"Error: %s.", temp1);
        MessageBox(g_hDlg, msg, L"Error", MB_OK);
        try {
            analog.Release();
            digital.Release();
        } catch (ECODE e) { return e; }
        SetText(IDSTARTSTOP, L"START");
        startServer = false;
        return 1;
    }catch (char *e) {
        mbstowcs(msg, e, MESSAGE_SIZE);
        MessageBox(g_hDlg, msg, L"Error", MB_OK);
        SetText(IDSTARTSTOP, L"START");
        startServer = false;
        try {
            analog.Release();
            digital.Release();
        } catch(ECODE e) { return e; }
        return 1;
    }

    SetText(IDSTARTSTOP, L"START");
    startServer = false;

    try {
            analog.Release();
            digital.Release();
        } catch (ECODE e) { return e; }

    return 0;
}

DWORD WINAPI Connection(LPVOID lParam)
{
    int iConn;
    char buffer[BUFSIZE];
    TCHAR msg[MESSAGE_SIZE], temp1[MESSAGE_SIZE], temp2[MESSAGE_SIZE];
    unsigned int size;
    bool dtaCapture = false;
    long analog_value = 23;
    long digital_value = 45;
    long value_buf[2];

    ZeroMemory(buffer, BUFSIZE);
    iConn = (int) lParam;
    try {
        while (startServer) {
            if (!server.Receive(iConn, buffer, &size, BUFSIZE))
                break;
            mbstowcs(temp1, buffer, size);
            swprintf(msg, MESSAGE_SIZE, L"RECV: %s\r\n", temp1);
            AppendText(IDC_LOG, msg);

            if (strncmp(buffer, "START", 5) == 0) {
                analog.Start((OLNOTIFYPROC) NotifyProc);
                dtaCapture = true;
            }

            if (strncmp(buffer, "STOP", 4) == 0)
                break;

            if (dtaCapture) {
                digital.GetValue(&digital_value);
                value_buf[0] = value;
                value_buf[1] = digital_value;
                memcpy(buffer, value_buf, sizeof(long)*2);
                Sleep(500);
            }

            server.Send(iConn, buffer, sizeof(long)*2);
            swprintf(msg, MESSAGE_SIZE, L"Sent %d bytes\r\n", sizeof(long)*2);
            AppendText(IDC_LOG, msg);
        }
    }
    catch (ECODE e) {
        char error[MESSAGE_SIZE];
        olDaGetErrorString(e, (PTSTR) error, MESSAGE_SIZE);
        mbstowcs(temp1, error, MESSAGE_SIZE);
        swprintf(msg, MESSAGE_SIZE, L"Error: %s.\r\n", temp1);
        AppendText(IDC_LOG, msg);
        return 1;
    } catch (char *e) {
        mbstowcs(msg, e, MESSAGE_SIZE);
        MessageBox(g_hDlg, msg, L"Error", MB_OK);
        server.CloseConnection(iConn);
        swprintf(msg, MESSAGE_SIZE, L"Connection %d Closed.\r\n", iConn);
        AppendText(IDC_LOG, msg);
        return 1;
    }

    server.CloseConnection(iConn);
    swprintf(msg, MESSAGE_SIZE, L"Connection %d Closed.\r\n", iConn);
    AppendText(IDC_LOG, msg);

    return 0;
}

void CALLBACK NotifyProc (UINT uiMsg, HDASS wParam, LPARAM lParam)
{
    if (uiMsg == OLDA_WM_BUFFER_DONE) {
        analog.GetValues(&value);
    }
}


