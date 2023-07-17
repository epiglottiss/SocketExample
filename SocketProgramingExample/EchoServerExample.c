#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winSock2.h>
#include<stdio.h>
#pragma comment(lib, "ws2_32")

DWORD WINAPI clientThread(LPVOID clientSocketParam) {
	puts("New Connection.");

	SOCKET clientConnectionSocket = (SOCKET)clientSocketParam;
	int receivedBytes = 0;
	char buffer[128] = { 0 };
	while (receivedBytes = recv(clientConnectionSocket, buffer, sizeof(buffer), 0)) {
		send(clientConnectionSocket, buffer, sizeof(buffer), 0);
		puts(buffer);
		memset(buffer, 0, sizeof(buffer));
	}

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

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
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
	DWORD threadID = 0;

	while ((clientConnectionSocket = accept(sock, (SOCKADDR*)&clientAddress, &clientAddressLength)) != INVALID_SOCKET)
	{
		threadID = CreateThread(NULL, 0, clientThread, (LPVOID)clientConnectionSocket, 0, &threadID);

		CloseHandle(threadID);
	}
	
	puts("close");
	closesocket(sock);

	WSACleanup();
	return 0;
}