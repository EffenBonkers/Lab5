#include <stdlib.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <stdio.h>

#define MAXCONNECTIONS	10

class TCPCommServer {
private:    
    SOCKET listenSocket;
    SOCKET connectionSockets[MAXCONNECTIONS];
    int numConn;
    WSADATA wsaData;
public:
    void StartServer(char *port, char *ip_address);
    int Connect();
    void CloseConnection(int whichConn);
    bool Receive(int whichConn, char *buffer, unsigned int *actual_size, unsigned int max_size);
    void Send(int whichConn, char *buffer, unsigned int size);
    void Shutdown();
};

class TCPCommClient {
private:
    WSAData wsaData;
    SOCKET connectionSocket;
public:
    void Connect(char *port, char *ip_address);
    bool Receive(char *buffer, unsigned int *actual_size, unsigned int max_size);
    void Send(char *buffer, unsigned int size);
    void Shutdown();
};


