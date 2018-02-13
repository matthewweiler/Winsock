#pragma once
#pragma comment (lib, "Ws2_32.lib")  // link to winsock lib
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // for inet_addr(), gethostbyname()

#include <winsock2.h>   // the .h file defines all windows socket functions 

#include "common.h"



class Winsock
{
public:
	static int initialize()   // call Winsock::intialize() in main, to intialize winsock only once
	{
		WSADATA wsaData;
		int iResult;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);   // defined in winsock2.h
		if (iResult != 0) {
			printf("WSAStartup failed: %d\n", iResult);
			WSACleanup();
			return 1;  // 1 means failed
		}
		return 0;   // 0 means no error (i.e., successful)
	}

	static void cleanUp() // call Winsock::cleanUp() in main only once
	{
		WSACleanup();
	}

	int createTCPSocket(void)
	{
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET) {
			printf("socket() error %d\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}
		return 0;
	}

	int connectToServerIP(DWORD IP, short port, string &mess)
	{
		struct sockaddr_in server; // structure for connecting to server
		server.sin_addr.S_un.S_addr = IP; // directly drop its tinary sersion into sin_addr
										  // setup the port # and protocol type
		server.sin_family = AF_INET; // IPv4
		server.sin_port = htons(port);// host-to-network flips the byte order
		if (connect(sock, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			mess += "Connection error: " + to_string(error) + "\n"; 
			return 1;
		}
		string inet = inet_ntoa(server.sin_addr);
		mess += "Successfully connected to " + inet + " on port " + to_string(htons(server.sin_port)) + "\n";
		return 0;
	}

	/*
	// dot-separated hostIP (e.g., "132.11.22.2"), 2-byte port(e.g., 80)
	int connectToServerIP(string hostIP, short port)
	{
		struct sockaddr_in server; // structure for connecting to server

		DWORD IP = inet_addr(hostIP.c_str());
		if (IP == INADDR_NONE)
		{
			printf("Invalid IP string: nor IP address\n");
			return 1;  // 1 means failed						
		}
		else
		{
			server.sin_addr.S_un.S_addr = IP; // if a valid IP, directly drop its binary version into sin_addr
		}

		// setup the port # and protocol type
		server.sin_family = AF_INET;  // IPv4
		server.sin_port = htons(port);// host-to-network flips the byte order

		if (connect(sock, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
		{
			printf("Connection error: %d\n", WSAGetLastError());
			return 1;
		}
		printf("Successfully connected to %s (%s) on port %d\n", hostIP.c_str(), inet_ntoa(server.sin_addr),
			htons(server.sin_port));

		return 0;
	}
	*/


	// define your sendRequest(...) function, to send a HEAD or GET request
	int sendRequest(string req){
		if (send(sock, req.c_str(), req.length(), 0) == SOCKET_ERROR){
			printf("send() error - %d\n", WSAGetLastError());
			return 1;
		}
		return 0;
	}

	// define your receive(...) function, to receive the reply from the server
	int receive(string &resp){
		FD_SET Reader;
		FD_ZERO(&Reader);
		FD_SET(sock, &Reader);

		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;

		resp = "";
		int bytes = 0;
		char buf[1024];
		do{
			if (select(0, &Reader, NULL, NULL, &timeout) > 0){
				if ((bytes = recv(sock, buf, 1024 - 1, 0)) == SOCKET_ERROR){
					printf("failed with %d on recv\n", WSAGetLastError());
					return 1;
				}
				else if (bytes > 0){
					buf[bytes] = 0;
					resp += buf;
				}
			}
			else {
				return 2;
			}
		} while (bytes > 0);

		return 0;
	}

	void closeSocket(void)
	{
		closesocket(sock);
	}



private:
	SOCKET sock;

	// define other private variables if needed


};