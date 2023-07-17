#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winSock2.h>
#include<stdio.h>
#include<list>
#include<Windows.h>
#include<iterator>
#pragma comment(lib, "ws2_32")


CRITICAL_SECTION criticalSection;
SOCKET sock;
std::list<SOCKET> clientList;

void addUser(SOCKET clientSocket) {
	EnterCriticalSection(&criticalSection);
	clientList.push_back(clientSocket);
	LeaveCriticalSection(&criticalSection);
}

void sendMessageAllClient(char* message) {

	std::list<SOCKET>::iterator it;
	
	EnterCriticalSection(&criticalSection);
	for (it = clientList.begin(); it != clientList.end(); it++) {
		send(*it, message, sizeof(message), 0);
	}
	LeaveCriticalSection(&criticalSection);
}

void eventHandler(DWORD eventType) {

	if (eventType == CTRL_C_EVENT) {
		std::list<SOCKET>::iterator it;

		EnterCriticalSection(&criticalSection);
		for (it = clientList.begin(); it != clientList.end(); it++) {
			closesocket(*it);
		}
		LeaveCriticalSection(&criticalSection);
		puts("All client disconnected");

		Sleep(500);
		DeleteCriticalSection(&criticalSection);
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
}

DWORD WINAPI clientThread(LPVOID clientSocketParam) {
	puts("New Connection.");

	SOCKET clientConnectionSocket = (SOCKET)clientSocketParam;
	int receivedBytes = 0;
	char buffer[128] = { 0 };
	while (receivedBytes = recv(clientConnectionSocket, buffer, sizeof(buffer), 0) >0) {
		sendMessageAllClient(buffer);
		puts(buffer);
		memset(buffer, 0, sizeof(buffer));
	}

	EnterCriticalSection(&criticalSection);
	clientList.remove(clientConnectionSocket);
	LeaveCriticalSection(&criticalSection);

	shutdown(clientConnectionSocket, SD_BOTH);
	closesocket(clientConnectionSocket);
	puts("Disconnected.");
	return 0;
}

int main() {

	WSADATA wsaData = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		puts("WSAStartup failed with error \n");
		return 0;
	}
	puts("WSAStartup\n");

	InitializeCriticalSection(&criticalSection);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)eventHandler, true);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		puts("Invalid Socket. Failed to create server socket.");
		return 0;
	}
	puts("Socket Created.\n");

	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, 1) == SOCKET_ERROR) {
		puts("Failed to set socket option.");
		return 0;
	}

	SOCKADDR_IN addressInfo;
	addressInfo.sin_family = AF_INET;
	addressInfo.sin_port = htons(15555);
	//addressInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addressInfo.sin_addr.S_un.S_addr = inet_addr("220.123.222.147");
	if (bind(sock, (SOCKADDR*)&addressInfo, sizeof(addressInfo)) == SOCKET_ERROR ) {
		puts("Failed to bind.");
		printf("%ld\n", WSAGetLastError());
		return 0;
	}
	puts("bind\n");
	if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
		puts("Cannot change state to listen.");
		return 0;
	}
	puts("listen\n");

	SOCKADDR_IN clientAddress = { 0 };
	int clientAddressLength = sizeof(clientAddress);
	SOCKET clientConnectionSocket = 0;
	HANDLE threadHandle;
	DWORD threadID = 0;

	while ((clientConnectionSocket = accept(sock, (SOCKADDR*)&clientAddress, &clientAddressLength)) != INVALID_SOCKET)
	{
		addUser(clientConnectionSocket);

		threadHandle = CreateThread(NULL, 0, clientThread, (LPVOID)clientConnectionSocket, 0, &threadID);

		CloseHandle(threadHandle);
	}
	
	puts("close");
	closesocket(sock);

	WSACleanup();
	return 0;
}