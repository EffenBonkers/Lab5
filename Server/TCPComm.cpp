#include "stdafx.h"
#include "TCPComm.h"

#pragma comment(lib, "ws2_32.lib")


void TCPCommServer::StartServer(char *port, char *ip_address)
{
    char error[50];
    struct addrinfo *result = NULL, *ptr = NULL, hints;

    // Initialize Winsock (Use version 2.2)
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        sprintf(error, "WSAStartup Failed: %d", iResult);
        throw error;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;          
    hints.ai_socktype = SOCK_STREAM;    
    hints.ai_protocol = IPPROTO_TCP;    
    hints.ai_flags = AI_PASSIVE;        
                                        
    iResult = getaddrinfo(ip_address, port, &hints, &result);
    if (iResult != 0) {
        sprintf(error, "getaddrinfo Failed: %d", iResult);
        WSACleanup();
        throw error;
	}

    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET) {
        sprintf(error, "Error at socket(): %ld", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        throw error;
    }

    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        sprintf(error, "bind failed with error: %d", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        throw error;
    }

    freeaddrinfo(result);

    for (int iConn = 0; iConn < MAXCONNECTIONS; iConn++)
        connectionSockets[iConn] = NULL;

    numConn = 0;
}

int TCPCommServer::Connect()
{
    char error[50];

    if (numConn == MAXCONNECTIONS) {
        sprintf(error, "Max Connections reached: %d", MAXCONNECTIONS);
        throw error;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        sprintf(error, "Listen failed with error: %ld", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        throw;
    }

    //find open connection
    int iConn;
    for (iConn = 0; iConn < MAXCONNECTIONS; iConn++) {
        if (connectionSockets[iConn] == NULL)
            break;
    }

    connectionSockets[iConn] = INVALID_SOCKET;
    connectionSockets[iConn] = accept(listenSocket, NULL, NULL);
    if (connectionSockets[iConn] == INVALID_SOCKET) {
        sprintf(error, "accept failed: %d", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        throw error;
    }

    numConn++;

    return iConn;
}

bool TCPCommServer::Receive(int whichConn, char *buffer, unsigned int *actual_size, unsigned int max_size)
{
    int iResult;
    char error[50];
    if ((iResult = recv(connectionSockets[whichConn], buffer, max_size, 0)) < 0) {
        sprintf(error, "recv failed: %d", WSAGetLastError());
        closesocket(connectionSockets[whichConn]);
        WSACleanup();
        throw error;
    }else if (iResult == 0)
        return false;

    *actual_size = iResult;
    return true;
}

void TCPCommServer::Send(int whichConn, char *buffer, unsigned int size)
{
    char error[50];
    if (send(connectionSockets[whichConn], buffer, size, 0) == SOCKET_ERROR) {
        sprintf(error, "send failed: %d", WSAGetLastError());
        WSACleanup();
        throw error;
    }
}

void TCPCommServer::CloseConnection(int whichConn)
{
    closesocket(connectionSockets[whichConn]);
    connectionSockets[whichConn] = NULL;
    numConn--;
}

void TCPCommServer::Shutdown()
{
    for (int i= 0; i < MAXCONNECTIONS; i++)
        if (connectionSockets[i] != NULL)
            closesocket(connectionSockets[i]);

    closesocket(listenSocket);
    WSACleanup();
}
#pragma endregion

#pragma region TCPCommClient implementation
void TCPCommClient::Connect(char *port, char *ip_address)
{
    WSADATA wsaData;
    int iResult;
    struct addrinfo *ptr = NULL, *result = NULL, hints;
    connectionSocket = INVALID_SOCKET;
    char error[50];

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        sprintf(error, "WSAStartup failed.");
        throw error;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;

    iResult = getaddrinfo(ip_address, port, &hints, &result);
    if (iResult != 0) {
        sprintf(error, "getaddrinfo failed: %d", iResult);
        WSACleanup();
        throw error;
    }
    
    ptr = result;
    connectionSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (connectionSocket == INVALID_SOCKET) {
        sprintf(error, "Error at socket(): %ld", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        throw error;
    }

    iResult = connect(connectionSocket, ptr->ai_addr, ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(connectionSocket);
        connectionSocket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (connectionSocket == INVALID_SOCKET) {
        sprintf(error, "Unable to connect to server.");
        WSACleanup();
        throw error;
    }
}

bool TCPCommClient::Receive(char *buffer, unsigned int *actual_size, unsigned int max_size)
{
    int iResult;
    char error[50];
    if ((iResult = recv(connectionSocket, buffer, max_size, 0)) < 0) {
        sprintf(error, "recv failed: %d", WSAGetLastError());
        closesocket(connectionSocket);
        WSACleanup();
        throw error;
    } else if (iResult == 0)
        return false;

    *actual_size = iResult;
    return true;
}

void TCPCommClient::Send(char *buffer, unsigned int size)
{
    char error[50];
    if (send(connectionSocket, buffer, size, 0) == SOCKET_ERROR) {
        sprintf(error, "send failed: %d", WSAGetLastError());
        closesocket(connectionSocket);
        WSACleanup();
        throw error;
    }
}

void TCPCommClient::Shutdown()
{
    closesocket(connectionSocket);
    WSACleanup();
}
#pragma endregion
