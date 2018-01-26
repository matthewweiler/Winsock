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
enum Request {head, getr, robot};
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

bool UniqueIP(string host, short port){
	struct hostent *remote;
	if ((remote = gethostbyname(host.c_str())))
	{
		printSafe("Invalid host name string: not FQDN\n");
		return false;  // 1 means failed
	}
	else {
		std::unordered_set<std::string>::const_iterator it = IPUnique.find(host);
		if (it == IPUnique.end()) {
			return true;
		}
		else {
			return false;
		}
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

void ConnectandSend(URLParser parser) {
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

	// receive robot reply
	string response = "";
	if (ws.receive(response) == 0) {
		printSafe(response + "\n");
	}

	//if no robot, send get request
	//else, do not send request

	ws.closeSocket();
	// parse url to get host name, port, path, and so on.
	Winsock::cleanUp();
}



UINT threadFunction(LPVOID pParam) {
	Param *p = (Param *)pParam;
	HANDLE arr[] = { p->eventQ, p->mutex };
	string url = "";
	while (true){
		if (WaitForMultipleObjects(2, arr, false, INFINITE) == WAIT_OBJECT_0) {
			break;
		}
		else { //Thread obtains mutex
			url = getURL();
			cout << url << endl;
			URLParser parser(url);
			bool uh;
			bool ui;
			if (url.compare("")) {
				uh = UniqueHost(parser.getHost());
				ui = UniqueIP(parser.getHost(), parser.getPort());
				if (uh && ui) {
					//Add the unique hosts here.
					HostsUnique.insert(parser.getHost());
					//Change to add IPs not hosts
					IPUnique.insert(parser.getHost());
				}
			}
			else {
				SetEvent(p->eventQ);
			}
			ReleaseMutex(p->mutex);
			if (uh && ui) {
				ConnectandSend(parser);
				//free(&parser);
			}
			else {
				//Do nothing for now. Output if IP is not Unique.
			}
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

