#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winSock2.h>
#include<stdio.h>
#include<list>
#include<Windows.h>
#include<iterator>
#pragma comment(lib, "ws2_32")


SOCKET sock;
WSAEVENT socketEventList[WSA_MAXIMUM_WAIT_EVENTS];
SOCKET clientSocketList[WSA_MAXIMUM_WAIT_EVENTS];
int listIndex;

void closeAllSocket() {
	for (int i = 0; i < listIndex; i++)
	{
		shutdown(clientSocketList[i], SD_BOTH);
		closesocket(clientSocketList[i]);
		WSACloseEvent(socketEventList[i]);
	}
}

void eventHandler(DWORD eventType) {

	if (eventType == CTRL_C_EVENT) {
		closeAllSocket();
		puts("All client socket were closed.");
		WSACleanup();
		exit(0);
	}
}

int main() {

	WSADATA wsaData = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		puts("WSAStartup failed with error \n");
		return 0;
	}
	puts("WSAStartup\n");

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
	addressInfo.sin_addr.S_un.S_addr = inet_addr("14.38.154.93");
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

	listIndex = 0;
	clientSocketList[listIndex] = sock;  //서버 소켓
	socketEventList[listIndex] = WSACreateEvent();	//서버 소켓 이벤트 등록

	if (WSAEventSelect(sock, socketEventList[listIndex], FD_ACCEPT) == SOCKET_ERROR) {
		puts("WSAEvnetSelect error.");
		return 0;
	}
	puts("WSAEventSelect ok.");

	DWORD signaledEventIndex;
	WSANETWORKEVENTS netEvent;
	while (true) {
		signaledEventIndex = WSAWaitForMultipleEvents(listIndex + 1, socketEventList, false, 100, false);

		if (signaledEventIndex == WSA_WAIT_FAILED || signaledEventIndex == WSA_WAIT_TIMEOUT) {
			//puts("no event.");
			continue;
		}

		if (WSAEnumNetworkEvents(clientSocketList[signaledEventIndex], socketEventList[signaledEventIndex], &netEvent) == SOCKET_ERROR) {
			puts("Enum network evnet error.");
			printf("error code : %d\n", WSAGetLastError());
			continue;
		}

		if (netEvent.lNetworkEvents & FD_ACCEPT) {
			if (netEvent.iErrorCode[FD_ACCEPT_BIT] != 0) {
				puts("FD_ACCEPT_BIT is not 0.");
				continue;
			}

			if (listIndex >= WSA_MAXIMUM_WAIT_EVENTS) {
				puts("No more connection.");
				continue;
			}

			SOCKADDR_IN clientAddress = { 0 };
			int clientAddressLength = sizeof(clientAddress);
			SOCKET clientConnectionSocket = accept(sock, (SOCKADDR*)&clientAddress, &clientAddressLength);

			if (clientConnectionSocket != INVALID_SOCKET) {
				++listIndex;
				clientSocketList[listIndex] = clientConnectionSocket;
				socketEventList[listIndex] = WSACreateEvent();
				puts("New Connection.");
			}

			WSAEventSelect(clientConnectionSocket, socketEventList[listIndex], FD_READ | FD_CLOSE);
		}

		else if (netEvent.lNetworkEvents & FD_CLOSE) {
			WSACloseEvent(socketEventList[signaledEventIndex]);
			shutdown(clientSocketList[signaledEventIndex], SD_BOTH);
			closesocket(clientSocketList[signaledEventIndex]);

			for (DWORD i = signaledEventIndex; i < listIndex; i++) {
				socketEventList[i] = socketEventList[i + 1];
				clientSocketList[i] = clientSocketList[i + 1];
			}

			listIndex--;
			printf("Disconnected. %d left.", listIndex);
		}

		else if (netEvent.lNetworkEvents & FD_READ) {
			char buffer[1024] = { 0 };
			int receivedBytes = recv(clientSocketList[signaledEventIndex],
				(char*)buffer, sizeof(buffer), 0);
			
			for (int i = 1; i < listIndex+1; i++)
			{
				send(clientSocketList[i], (char*)buffer, receivedBytes, 0);
			}
		}
		else {
			puts("event signaled but no match.");
		}
	}

	WSACleanup();
	return 0;
}