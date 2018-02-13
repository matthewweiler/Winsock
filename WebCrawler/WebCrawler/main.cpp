#pragma once
#include "stdafx.h"
#include "common.h"
#include "winsock.h"
#include "urlparser.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <mutex>

unordered_set<string> HostsUnique;
unordered_set<string> IPUnique;
ifstream fin;
ofstream fout;
enum Request { head, getr, robot };
mutex mutexPrint;
class Param {
public:
	HANDLE mutex;
	HANDLE eventQ;
};

void printSafe(string s) {
	mutexPrint.lock();
	cout << s;
	mutexPrint.unlock();
}

bool UniqueHost(string host) {
	std::unordered_set<std::string>::const_iterator it = HostsUnique.find(host);
	if (it == HostsUnique.end()) {
		return true;
	}
	else {
		return false;
	}
}

bool UniqueIP(string host) {
	std::unordered_set<std::string>::const_iterator it = IPUnique.find(host);
	if (it == IPUnique.end()) {
		return true;
	}
	else {
		return false;
	}
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

void ConnectandSend(URLParser parser, string& message) {
	Winsock::initialize();	// initialize 

	Winsock ws;
	string host = parser.getHost();
	short port = parser.getPort();
	string path = parser.getPath();
	string query = parser.getQuery();

	// the following shows how to use winsock functions
	ws.createTCPSocket();
	ws.connectToServer(host, port);

	// test for robot file
	string req = constructRequest(robot, &parser);
	ws.sendRequest(req);
	message += "sent request\n";

	// receive robot reply
	string response = "";
	if (ws.receive(response) == 0) {
		printSafe(response + "\n");
	}
	message += "response received\n";

	//if no robot, send get request
	if (response[9] != '2') {
		string get_req = constructRequest(getr, &parser);
		ws.closeSocket();
		ws.cleanUp();
		Winsock ws;
		ws.createTCPSocket();
		ws.connectToServer(host, port);
		ws.sendRequest(get_req);
		response = "";
		if (ws.receive(response) == 0)
			message += "response received\n";
		//printSafe(response + "\n");
	}
	//else, do not send request

	ws.closeSocket();
	// parse url to get host name, port, path, and so on.
	Winsock::cleanUp();
}



UINT threadFunction(LPVOID pParam) {
	Param *p = (Param *)pParam;
	HANDLE arr[] = { p->eventQ, p->mutex };
	string url = "";
	string message = "";
	while (true) {
		if (WaitForMultipleObjects(2, arr, false, INFINITE) == WAIT_OBJECT_0) {
			break;
		}
		else { //one Thread obtains mutex
			printSafe(message);
			message = "";
			url = getURL();
			string message = url + "\n";
			//cout << url << endl;
			URLParser parser(url);
			bool uh;
			bool ui;
			if (!url.compare("")) {
				uh = UniqueHost(parser.getHost());
				if (uh) {
					message += "unique host\n";
					HostsUnique.insert(parser.getHost());
				}
				else {
					message += "NOT a unique host\n";
					continue;
				}

				// get IP
				struct sockaddr_in server; // structure for connecting to server
				struct hostent *remote;    // structure used in DNS lookups: convert host name into IP address

				if ((remote = gethostbyname(parser.getHost().c_str())) == NULL) {
					message += "failed to get IP\n";
					//printf("Invalid host name string: not FQDN\n");
					continue;  // 1 means failed
				}
				else // take the first IP address and copy into sin_addr 
				{
					memcpy((char *)&(server.sin_addr), remote->h_addr, remote->h_length);
				}

				ui = UniqueIP((char*)&server.sin_addr);
				//ui = UniqueIP(parser.getHost(), parser.getPort());
				if (ui) {
					message += "unique IP\n";
					IPUnique.insert((char*)&server.sin_addr);
				}
				else {
					message += "NOT unique IP\n";
					continue;
				}
				/*
				if (uh && ui) {
					//Add the unique hosts here.
					HostsUnique.insert(parser.getHost());
					//Change to add IPs not hosts
					IPUnique.insert(parser.getHost());
				}
				*/

			}
			else {
				SetEvent(p->eventQ);
			}
			ReleaseMutex(p->mutex);
			ConnectandSend(parser, message);
			//printSafe(message);
		}
	}
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
		printSafe("Not Enough arguments");
		getchar();
		return 0;
	}
	fin.open(filename);
	// fout.open("crawldata.txt");
	if (fin.fail()) // cannot open this file
	{
		printSafe("NO such a file. Failed to open it\n");
		getchar();
		return 0;
	}

	//Create Threads and wait for end
	Param p;
	p.mutex = CreateMutex(NULL, 0, NULL);
	p.eventQ = CreateEvent(NULL, true, false, NULL);
	for (int i = 0; i < numThreads; i++) {
		CreateThread(NULL, 4096, (LPTHREAD_START_ROUTINE)threadFunction, &p, 0, NULL);
	}
	WaitForSingleObject(p.eventQ, INFINITE);


	fin.close(); //fout.close();

	printSafe("Enter any key to continue ...\n");
	getchar();

	return 0;   // 0 means successful
}

