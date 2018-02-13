#pragma once
#include "stdafx.h"
#include "common.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include "weiler-conrad-gore-winsock.h"
#include "weiler-conrad-gore-urlparser.h"
#include <mutex>

unordered_set<string> HostsUnique;
unordered_set<DWORD> IPUnique;
ifstream fin;
ofstream fout;
mutex mutexPrint;
mutex robots;
mutex pages;
int URLCount = 0;
int DNSNum = 0;
int robotsNum = 0;
int pagesNum = 0;
enum Request { robot, head, getr };

class Parameters {
public:
	HANDLE mutex;
	HANDLE eventQuit;
};

void printSafe(string s) {
	mutexPrint.lock();
	cout << s << flush;
	mutexPrint.unlock();
}

void incRobots() {
	robots.lock();
	robotsNum++;
	robots.unlock();
}

void incPages() {
	pages.lock();
	pagesNum++;
	pages.unlock();
}

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
	if (HostsUnique.find(host) == HostsUnique.end()) {
		HostsUnique.insert(host);
		return true;
	}
	return false;
}

bool UniqueIP(DWORD ip) {
	if (IPUnique.find(ip) == IPUnique.end()) {
		IPUnique.insert(ip);
		return true;
	}
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

int ConnectandSend(URLParser parser, DWORD IP, string &mess) {
	Winsock wss;
	if (wss.createTCPSocket() == 0) {
		short port = parser.getPort();
		if (wss.connectToServerIP(IP, port, mess) == 0) {
			string req = constructRequest(robot, &parser);
			mess += "Connecting on robots... ";
			int sendErr = wss.sendRequest(req);
			mess += "done in ___\n";
			mess += "Loading... ";
			if (sendErr == 0) {
				incRobots();
				string response = "";
				int received = wss.receive(response);
				mess += "done in ___ with ___ bytes\n";
				mess += "Verifying Header... \n";
				if (received == 0 && response[9] == '4') {
					Winsock ws;
					if (ws.createTCPSocket() == 0) {
						if (ws.connectToServerIP(IP, port, mess) == 0) {
							response = "";
							string req2 = constructRequest(getr, &parser);
							mess += "Connecting on page... ";
							if (ws.sendRequest(req2) == 0) {
								mess += "done in __\n";
								mess += "Loading... ";
								int received = ws.receive(response);
								if (received != 0) {
									mess += "failed something went wrong with the get request.";
								}
								else {
									incPages();
									mess += "done in ___ with ___ bytes\n";
									mess += response + "\n";
								}
							}
							else {
								mess += "failed connection\n";
							}
						}
					}
					else {
						mess += "failed to create socked to the server for Get Request.\n";
						return 1;
					}
					ws.closeSocket();
				}
				else if (received == 2) {
					mess += "failed with slow download.\n";
				}

			}
			else {
				mess += "failed with robots send error.\n";
			}
		}
		else {
			mess += "Failed to connect to the server.\n";
			return 1;
		}
	}
	else {
		mess += "Failed to create socket to the server.\n";
		return 1;
	}
	wss.closeSocket();
	return 0;
}

UINT thread_fun(LPVOID pParam) {
	Parameters *p = (Parameters *)pParam;
	HANDLE arr[] = { p->eventQuit, p->mutex };
	string message;
	while (true)
	{
		if (WaitForMultipleObjects(2, arr, false, INFINITE)
			== WAIT_OBJECT_0)
			break;
		else // this thread obtains p->mutex
		{
			message = "";
			string url = getURL();
			if (url.compare("") != 0) {
				URLCount++;
				URLParser parser(url);
				message += "URL: " + url + "\n";
				message += "Parsing URL... host " + parser.getHost() + ", port " + std::to_string(parser.getPort()) + "\n";

				//Use Mutex here to get keep away from 
				message += "Checking host uniqueness... ";
				bool UH = UniqueHost(parser.getHost());
				if (UH) {
					message += "passed\n";
				}
				else {
					message += "failed\n";
					printSafe(message);
					continue;
				}

				message += "Doing DNS...\n";
				DWORD ip = getIP(parser.getHost());
				if (ip == 1) {
					message += "failed\n";
					printSafe(message);
					continue;
				}

				DNSNum++;
				message += " done in __, found " + std::to_string(ip) + "\n";

				//Use Mutex here to get keep away from 
				message += "Checking IP uniqueness... ";
				bool UIP = UniqueIP(ip);
				if (UIP) {
					message += "passed\n";
				}
				else {
					message += "failed\n";
					printSafe(message);
					continue;
				}

				//release mutex
				ReleaseMutex(p->mutex);

				if (UH && ip != 1 && UIP) {
					ConnectandSend(parser, getIP(parser.getHost()), message);
					printSafe(message);
				}
			}
			else {
				SetEvent(p->eventQuit);
			}

		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int numThreads = 0; 
	string filename = ""; // include<string>
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

	Parameters p;
	p.mutex = CreateMutex(NULL, 0, NULL);
	p.eventQuit = CreateEvent(NULL, true, false, NULL);

	//If doesn't work, initialize within each thread
	Winsock::initialize();

	for (int i = 0; i < numThreads; ++i) {
		CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)thread_fun, &p, 0, NULL);
	}
	
	WaitForSingleObject(p.eventQuit, INFINITE);

	//Implement Multithreading here
	Winsock::cleanUp();
	///////////////////////////////

	fin.close(); //fout.close();

	cout << "Extracted " + to_string(URLCount) + " @ _______/s\n";
	cout << "Looked up " + to_string(DNSNum) + "@ _________/s\n";
	cout << "Downloaded " + to_string(robotsNum) + " robots @ _______/s\n";
	cout << "Crawled " + to_string(pagesNum) + " pages @ _________/s\n";

	cout << "Enter any key to continue ...\n";
	getchar();

	return 0;
}
