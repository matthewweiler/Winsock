#pragma once
#include "stdafx.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include "weiler-conrad-gore-winsock.h"
#include "weiler-conrad-gore-urlparser.h"

unordered_set<string> HostsUnique;
unordered_set<DWORD> IPUnique;
ifstream fin;
ofstream fout;
enum Request { robot, head, getr };



DWORD getIP(string host) {
	struct sockaddr_in server;
	struct hostent * remote;
	if ((remote = gethostbyname(host.c_str())) == NULL)
	{
		printf("Invalid host name string: not FQDN\n");
		return 1; // 1 means failed
	}
	else // take the first IP address and copy into sin_addr
	{
		memcpy((char *)&(server.sin_addr), remote->h_addr, remote->h_length);
	}
	return server.sin_addr.S_un.S_addr; // return IP in binary version
}

bool UniqueHost(string host) {
	cout << "Checking host uniqueness... ";
	if (HostsUnique.find(host) == HostsUnique.end()) {
		HostsUnique.insert(host);
		cout << "passed" << endl;
		return true;
	}
	cout << "failed" << endl;
	return false;
}

bool UniqueIP(DWORD ip) {
	cout << "Checking IP uniqueness... ";
	if (IPUnique.find(ip) == IPUnique.end()) {
		IPUnique.insert(ip);
		cout << "passed" << endl;
		return true;
	}
	cout << "failed" << endl;
	return false;
}

string constructRequest(Request r, URLParser* p) {
	switch (r) {
	case robot: return "HEAD /robots.txt HTTP/1.0\nHost: " + p->getHost() + "\n\n";
	case head: return "HEAD " + p->getPath() + p->getQuery() + " HTTP/1.0\nHost: " + p->getHost() + "\n\n";
	case getr: return "GET " + p->getPath() + p->getQuery() + " HTTP/1.0\nHost: " + p->getHost() + "\n\n";
	default: return "HEAD /robots.txt HTTP/1.0\nHost: " + p->getHost() + "\n\n";
	}
}

string getURL() {
	string url;
	if (!fin.eof()) {
		fin >> url;
		return url;
	}
	return "";
}

int ConnectandSend(URLParser parser, DWORD IP) {
	Winsock wss;
	if (wss.createTCPSocket() == 0) {
		short port = parser.getPort();
		if (wss.connectToServerIP(IP, port) == 0) {
			string req = constructRequest(robot, &parser);
			cout << "Connecting on robots... ";
			int sendErr = wss.sendRequest(req);
			cout << "done in ___" << endl;
			cout << "Loading... ";
			if (sendErr == 0) {
				string response = "";
				int received = wss.receive(response);
				cout << "done in ___ with ___ bytes" << endl;
				cout << "Verifying Header... " << endl;
				if (received == 0 && response[9] == '4') {
					Winsock ws;
					if (ws.createTCPSocket() == 0) {
						if (ws.connectToServerIP(IP, port) == 0) {
							response = "";
							req = constructRequest(getr, &parser);
							cout << "Connecting on page... ";
							if (ws.sendRequest(req) == 0) {
								cout << "done in __" << endl;
								cout << "Loading... ";
								int received = ws.receive(response);
								if (received != 0) {
									cout << "failed something went wrong with the get request.";
								}
								else {
									cout << "done in ___ with ___ bytes" << endl;
									cout << response << endl;
								}
							}
							else {
								cout << "failed connection" << endl;
							}
						}
					}
					else {
						cout << "failed to create socked to the server for Get Request." << endl;
						return 1;
					}
					ws.closeSocket();
				}
				else if (received == 2) {
					cout << "failed with slow download." << endl;
				}

			}
			else {
				cout << "failed with robots send error." << endl;
			}
		}
		else {
			cout << "Failed to connect to the server." << endl;
			return 1;
		}
	}
	else {
		cout << "Failed to create socket to the server." << endl;
		return 1;
	}
	wss.closeSocket();
	return 0;
}

int main(int argc, char **argv)
{
	int numThreads = 0; string filename = ""; // include<string>
	if (argc > 1)
	{
		numThreads = atoi(argv[1]);
		filename = argv[2];
		printf("number of threads is %d, filename is %s\n\n",
			numThreads, filename.c_str());
	}
	else {
		cout << "Not Enough arguments\n";
		getchar();
		return 0;
	}

	fin.open(filename);

	//Check for file open failure
	if (fin.fail()) // cannot open this file
	{
		cout << "NO such a file. Failed to open it\n";
		getchar();
		return 0;
	}

	//Do crawling logic here
	Winsock::initialize();
	string url = getURL();
	while (url.compare("") != 0) {
		URLParser parser(url);
		cout << "URL: " + url << endl;
		cout << "Parsing URL... host " + parser.getHost() + ", port " + std::to_string(parser.getPort()) << endl;
		bool UH = UniqueHost(parser.getHost());
		cout << "Doing DNS...";
		DWORD ip = getIP(parser.getHost());
		cout << " done in __, found " + std::to_string(ip) << endl;
		bool UIP = UniqueIP(ip);
		if (UH && ip != 1 && UIP) {
			ConnectandSend(parser, getIP(parser.getHost()));
		}
		url = getURL();
		cout << endl << endl;
	}
	Winsock::cleanUp();
	///////////////////////////////

	fin.close(); //fout.close();

	cout << "Enter any key to continue ...\n";
	getchar();

	return 0;
}
