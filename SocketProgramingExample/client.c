#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

int __cdecl main(int argc, char *argv[])
{
	/*struct hostent* host_info;
	struct sockaddr_in server;
	memset(&host_info, 0, sizeof(host_info));*/

	if (argc < 2) {
		puts("Usage : robot hostname <uri>");
		return;
	}

	LPWSADATA wsaData;
	int startRet = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (startRet != 0) {
		printf("WSAStartup failed with error : %d\n", startRet);
		return 0;
	}
	printf("WSAStartup\n");

	struct addrinfo hints;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	INT ret = getaddrinfo(argv[1], "80", &hints, &result);
	//host_info = gethostbyname(argv[1]);
	
	if (ret != 0) {
		printf("Host Not Found! : %s\n\r", argv[1]);
		printf("Error Code : %d\n", ret);
		WSACleanup();
		return 0;
	}
	printf("Host Found\n");

	SOCKET connectSocket = INVALID_SOCKET;
	ptr = result;
	connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (connectSocket == INVALID_SOCKET) {
		printf("Socket failed : %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 0;
	}

	int connectResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (connectResult == SOCKET_ERROR) {
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
	}
	freeaddrinfo(result);
	if (connectSocket == INVALID_SOCKET) {
		puts("Failed to connect to server\n");
		WSACleanup();
		return 0;
	}
	printf("Connected %s\n", argv[1]);

	char request[80];
	if (argv[2] == NULL) {
		sprintf_s(request, sizeof(request), "GET /index.html HTTP/2.0 \n\r\n\r");
	}
	else {
		sprintf_s(request, sizeof(request), "GET /%s HTTP/2.0 \n\r\n\r", argv[2]);
	}
	int sendResult = send(connectSocket, request, (int)strlen(request), 0);
	if (sendResult == SOCKET_ERROR) {
		printf("Send faild : %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 0;
	}
	puts("Success Send Request.\n");

	int shutResult = shutdown(connectSocket, SD_SEND);
	if (shutResult == SOCKET_ERROR) {
		printf("Shutdown failed : %d \n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 0;
	}

	char recvBuf[512];
	int recvResult;
	int max = 0;
	do {
		recvResult = recv(connectSocket, &recvBuf, (int)sizeof(recvBuf), 0);
		if (recvResult > 0) {
			printf("Recved : %d\n", recvResult);
			//puts(recvBuf);
		}
		else if (recvResult == 0) {
			puts("all received and connection closed\n");
		}
		else {
			printf(" recv failed : %d\n", WSAGetLastError());
		} 
	} while (recvResult > 0);

	shutResult = shutdown(connectSocket, SD_SEND);
	if (shutResult == SOCKET_ERROR) {
		printf("Shutdown failed : %d \n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 0;
	}

	closesocket(connectSocket);
	WSACleanup();
	//memset((char*)&server, 0, sizeof(server));
	//server.sin_family = AF_INET;	//IPv4
	//memcpy((char*)&server.sin_addr, host_info->h_addr_list, host_info->h_length);
	//server.sin_port = htons(80);

	//int sock = socket(AF_INET, SOCK_STREAM, 0);


	//char buf[20];
	//inet_ntop(AF_INET, &server.sin_addr, buf, sizeof(buf));
	//printf("Trying %s...\n", buf);

	//int connectResult = connect(sock, (struct sockaddr*)&server, sizeof(server));
	//if (connectResult == -1) {
	//	printf("Connect Failed : %s\n", argv[2]);
	//	return 0;
	//}


	/*char request[80];
	if (argv[2] == NULL) {
		sprintf_s(request, sizeof(request), "GET /index.html HTTP/1.1 \n\r\n\r");
	}
	else {
		sprintf_s(request, sizeof(request), "GET /%s HTTP/1.1 \n\r\n\r", argv[2]);
	}*/

	//send(sock, request, strlen(request), 0);

	//char ch;
	//while (recv(socket, &ch, 1, 0) > 0) {
	//	putchar(ch);
	//}

	//closesocket(socket);

	return 0;
}

