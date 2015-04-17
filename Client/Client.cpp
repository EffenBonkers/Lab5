// Client.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Client.h"
#include "../Server/TCPComm.h"

#define MAX_LOADSTRING 100
#define DEFAULT_PORT    "27015"
#define DEFAULT_IP      "127.0.0.1"
#define MESSAGE_SIZE    50
#define BUFSIZE         256

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND g_hDlg;
bool isConnected = false;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Client(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI        Communicate(LPVOID lParam);


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
	LoadString(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));

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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLIENT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CLIENT);
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
        DialogBox(hInst, MAKEINTRESOURCE(IDD_CLIENT), hWnd, Client);
        break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK Client(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hConnectionThread;


	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
    case WM_INITDIALOG:
        g_hDlg = hDlg;
		return (INT_PTR)TRUE;

	case WM_COMMAND:
        if (LOWORD(wParam) == IDC_STARTSTOP) {
            if (!isConnected) {
                SetText(IDC_STARTSTOP, L"STOP");
                isConnected = true;

                hConnectionThread = CreateThread(
                                        NULL,
                                        0,
                                        Communicate,
                                        NULL,
                                        0,
                                        0);

            }
            else {
                SetText(IDC_STARTSTOP, L"START");
                isConnected = false;
            }
        }
        return (INT_PTR) TRUE;
		break;
    case WM_CLOSE:
        CloseHandle(hConnectionThread);
        EndDialog(hDlg, LOWORD(wParam));
        PostQuitMessage(0);
        return (INT_PTR) TRUE;
        break;
	}
	return (INT_PTR)FALSE;
}


DWORD WINAPI Communicate(LPVOID lParam)
{
    TCPCommClient client;
    char buffer[BUFSIZE];
    unsigned int size;
    TCHAR msg[MESSAGE_SIZE];
    TCHAR temp1[MESSAGE_SIZE], temp2[MESSAGE_SIZE];
    char testmsg[10];
    BOOL signal = 0;

    mbstowcs(temp1, DEFAULT_PORT, MESSAGE_SIZE);
    mbstowcs(temp2, DEFAULT_IP, MESSAGE_SIZE);

    try {
        client.Connect(DEFAULT_PORT, DEFAULT_IP);
        swprintf(msg, MESSAGE_SIZE, L"Connected to %s: %s\r\n", temp1, temp2);
        AppendText(IDC_LOG, msg);
    
        strncpy(testmsg, "START", 10);
        client.Send(testmsg, 10);
        while (isConnected) {
            long values[2];
            client.Receive(buffer, &size, BUFSIZE);
            values[0] = *((long *)(buffer));
            values[1] = *((long *)(buffer)+1);
            signal = values[1];
            swprintf(msg, MESSAGE_SIZE, L"Value : %d %d\r\n", values[0], values[1]);
            AppendText(IDC_LOG, msg);
            if (values[0] > 40000L) {
                signal = values[1];
                swprintf(msg, MESSAGE_SIZE, L"Signal Changed: %d\r\n", values[0]);
                AppendText(IDC_LOG, msg);
                if ((values[1] & 1L) != signal) {
                    signal = values[1] & 1L;
                    swprintf(msg, MESSAGE_SIZE, L"Signal Changed: %d\r\n", signal);
                    AppendText(IDC_LOG, msg);
                }
            }
            
            Sleep(500);
            strncpy(testmsg, "ALIVE", 10);
            client.Send(testmsg, 10);
        }

        client.Receive(buffer, &size, BUFSIZE); //DISCARD
        strncpy(testmsg, "STOP", 10);
        client.Send(testmsg, 10);
        Sleep(50);
        client.Shutdown();
    }
    catch (char *e) {
        mbstowcs(msg, e, MESSAGE_SIZE);
        MessageBox(g_hDlg, msg, L"Error", MB_OK);
        bool isConnected = false;
        SetText(IDC_STARTSTOP, L"START");
        return 1;
    }

    isConnected = false;
    SetText(IDC_STARTSTOP, L"START");

    return 0;
}