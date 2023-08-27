
#include <WinSock2.h>
#pragma comment(lib, "ws2_32")
#include <Windows.h>
#include <stdio.h>
#include <list>

#define MAX_THREAD_COUNT 4

typedef struct _USERSESSION {
    SOCKET socket;
    char buffer[8192];
} USERSESSION;

SOCKET serverSocket;
HANDLE iocpHandle;
CRITICAL_SECTION criticalSection;
std::list<SOCKET> clientSocketList;

void CloseAll() {
    std::list<SOCKET>::iterator it;

    EnterCriticalSection(&criticalSection);
    for ( it = clientSocketList.begin(); it != clientSocketList.end(); it++)
    {
        shutdown(*it, SD_BOTH);
        closesocket(*it);
    }
    LeaveCriticalSection(&criticalSection);
}

void ReleaseServer() {
    CloseAll();
    Sleep(500);

    shutdown(serverSocket, SD_BOTH);
    closesocket(serverSocket);
    serverSocket = NULL;

    CloseHandle(iocpHandle);
    iocpHandle = NULL;

    Sleep(500);
    DeleteCriticalSection(&criticalSection);
}

bool CtrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT) {
        ReleaseServer();

        puts("End of Server.");
        WSACleanup();
        exit(0);
        return true;
    }
    return false;
}

void CloseClient(SOCKET clientSocket) {
    shutdown(clientSocket, SD_BOTH);
    closesocket(clientSocket);

    EnterCriticalSection(&criticalSection);
    clientSocketList.remove(clientSocket);
    LeaveCriticalSection(&criticalSection);
}

void SendMessageAll(char* message, int bytes) {
    std::list<SOCKET>::iterator it;

    EnterCriticalSection(&criticalSection);
    for (it = clientSocketList.begin(); it != clientSocketList.end(); it++)
    {
        send(*it, message, bytes, 0);
    }
    LeaveCriticalSection(&criticalSection);
}

DWORD WINAPI ThreadComplete(LPVOID parameter) {
    puts("IOCP Worker Thread Start.");

    DWORD transferredBytes;
    USERSESSION* userSession = NULL; 
    LPWSAOVERLAPPED io = NULL;
    bool result;
    while (true) {
        result = GetQueuedCompletionStatus(iocpHandle, &transferredBytes, (PULONG_PTR)&userSession, &io, INFINITE);

        if (result == true) {
            if (transferredBytes == 0) {
                CloseClient(userSession->socket);
                delete userSession;
                delete io;
                puts("[GQCS] client disconnected. ");
            }
            else {
                SendMessageAll(userSession->buffer, transferredBytes);
                memset(userSession->buffer, 0, sizeof(userSession->buffer));

                DWORD receivedBytes = 0;
                DWORD flag = 0;
                WSABUF wsaBuf = { 0 };
                wsaBuf.buf = userSession->buffer;
                wsaBuf.len = sizeof(userSession->buffer);
                WSARecv(userSession->socket, &wsaBuf, 1, &receivedBytes, &flag, io, NULL);

                if (WSAGetLastError() != WSA_IO_PENDING) {
                    puts("[GQCS] WSARecv didn't return IO pending. ");
                }
            }
        }
        else {
            if (io == NULL) {
                puts("[GQCS] Overlapped io is null.");
                break;
            }
            else {
                if (userSession == NULL) {
                    CloseClient(userSession->socket);
                    delete io;
                    delete userSession;
                }
                puts("[GQCS] Disconnected");
            }
        }
    }
    puts("IOCP Worker Thread End.");
    return 0;
}

DWORD WINAPI ThreadAcceptLoop(LPVOID parameter) {

    SOCKADDR clientAddress;
    SOCKET clientSocket;
    int addressSize = sizeof(SOCKADDR);
    USERSESSION *newUser;
    WSAOVERLAPPED *overlapped;
    WSABUF wsaBuf;
    DWORD receivedBytes;
    DWORD flag;
    int wsaRecvResult = 0;
    while ((clientSocket = accept(serverSocket, &clientAddress, &addressSize)) != INVALID_SOCKET) {
        puts("New Connection.");
        EnterCriticalSection(&criticalSection);
        clientSocketList.push_back(clientSocket);
        LeaveCriticalSection(&criticalSection);

        newUser = new USERSESSION;
        ZeroMemory(newUser, sizeof(USERSESSION));
        newUser->socket = clientSocket;

        overlapped = new WSAOVERLAPPED;
        ZeroMemory(overlapped, sizeof(WSAOVERLAPPED));

        CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, (ULONG_PTR)newUser, 0);

        wsaBuf.buf = newUser->buffer;
        wsaBuf.len = sizeof(newUser->buffer);
        receivedBytes = 0;
        flag = 0;
        wsaRecvResult = WSARecv(clientSocket, &wsaBuf, 1, &receivedBytes, &flag, overlapped, NULL);
        if (WSAGetLastError() != WSA_IO_PENDING) {
            puts("WSARecv is not IO pending.");
        }
    }
    return 0;
}

int main()
{
    WSADATA wsaData = { 0 };
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        puts("WSAStartup failed.");
        return 0;
    }

    InitializeCriticalSection(&criticalSection);

    if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, true) == false) {
        puts("SetConsoleCtrlHandler failed.");
        return 0;
    }

    iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
        NULL, 0, 0); //if thread count is 0, it depends on OS.
    if (iocpHandle == NULL) {
        puts("iocpCreate failed.");
        return 0;
    }

    HANDLE threadHandle;
    DWORD threadId;
    for (int i = 0; i < MAX_THREAD_COUNT; i++) {
        threadId = 0;

        threadHandle = CreateThread(NULL, 0, ThreadComplete, NULL, 0, &threadId);

        CloseHandle(threadHandle);
    }

    serverSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

    SOCKADDR_IN address;
    address.sin_family = AF_INET;
    address.sin_port = htons(15555);
    address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (SOCKADDR*)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
        puts("bind failed.");
        ReleaseServer();
        return 0;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        puts("listen failed.");
        ReleaseServer();
        return 0;
    }

    threadHandle = CreateThread(NULL, 0, ThreadAcceptLoop, NULL, 0, &threadId);
    CloseHandle(threadHandle);

    puts("Server Started. ");
    while (true) {
        getchar();
    }
    return 0;
}


