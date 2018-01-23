#pragma once
#include "stdafx.h"
#include "common.h"
#include "winsock.h"
#include "urlparser.h"

int main(int argc, char **argv)
{
	Winsock::initialize();	// initialize 

	short port = 80;

	Winsock ws;

	// parse url to get host name, port, path, and so on.
	string url = "http://www.weatherline.net/";
	URLParser parser(url);
	string host = parser.getHost();
	port = parser.getPort();
	string path = parser.getPath();
	string query = parser.getQuery();

	// the following shows how to use winsock functions

	//string host = "www.yahoo.com";
	ws.createTCPSocket();
	ws.connectToServer(host, port);


	// construct a GET or HEAD request (in a string), send request
	string GET = "GET " + path + query + " HTTP/1.0\nHost: " + host + "\n\n";
	cout << GET;
	ws.sendRequest(GET);
	// receive reply
	string response = "";
	if (ws.receive(response) == 0){
		cout << response << endl;
	}

	ws.closeSocket();

	// parse url to get host name, port, path, and so on.

	string hostIP = "131.238.72.77";  // udayton.edu's IP
	ws.createTCPSocket();
	ws.connectToServerIP(hostIP, port);
	// construct a GET or HEAD request (in a string), send request
	// receive reply
	ws.closeSocket();

	Winsock::cleanUp();

	printf("Enter any key to continue ...\n");
	getchar();

	return 0;   // 0 means successful
}