#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

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
	puts("1\n");
	SOCKADDR_IN addressInfo;
	addressInfo.sin_family = AF_INET;
	addressInfo.sin_port = htons(15555);
	addressInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (bind(sock, (SOCKADDR*)&addressInfo, sizeof(addressInfo)) == SOCKET_ERROR ) {
		puts("Failed to bind.");
		return 0;
	}
	puts("2\n");
	if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
		puts("Cannot change state to listen.");
		return 0;
	}
	puts("3\n");
	SOCKADDR_IN clientAddress = { 0 };
	int clientAddressLength = sizeof(clientAddress);
	SOCKET clientConnectionSocket = 0;
	char buffer[128] = { 0 };
	int receivedBytes = 0;

	puts("4\n",WSAGetLastError());
	while ((clientConnectionSocket = accept(sock, (SOCKADDR*)&clientAddress, &clientAddressLength)) != INVALID_SOCKET)
	{
		puts("New Connection.");
		
		while (receivedBytes = recv(clientConnectionSocket, buffer, sizeof(buffer), 0)) {
			send(clientConnectionSocket, buffer, sizeof(buffer), 0);
			puts(buffer);
			memset(buffer, 0, sizeof(buffer));
		}

		shutdown(clientConnectionSocket, SD_BOTH);
		closesocket(clientConnectionSocket);
		puts("Disconnected.");
	}
	
	puts("close");
	closesocket(sock);

	WSACleanup();
	return 0;
}