#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include<WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
	if (argc < 2) {
		puts("enter server IP.");
		return 0;
	}
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

	SOCKADDR_IN serverAddressInfo = { 0 };
	serverAddressInfo.sin_family = AF_INET;
	serverAddressInfo.sin_port = htons(15555);
	serverAddressInfo.sin_addr.S_un.S_addr = inet_addr(argv[1]);
	if (connect(sock, (SOCKADDR*)&serverAddressInfo, sizeof(serverAddressInfo)) == SOCKET_ERROR) {
		puts("Failed to connect server.");
		return 0;
	}

	char buffer[128] = { 0 };
	while (1) {
		gets_s(buffer, sizeof(buffer));

		if (strcmp(buffer, "q") == 0) {
			break;
		}

		send(sock, buffer, sizeof(buffer),0);
		memset(buffer, 0, sizeof(buffer));

		recv(sock, buffer, sizeof(buffer), 0);
		printf("Receive from server : %s", buffer);
	}

	shutdown(sock, SD_BOTH);
	closesocket(sock);
	WSACleanup();
	return 0;
}


