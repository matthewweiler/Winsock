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

unordered_set<size_t> HostsUnique;
unordered_set<DWORD> IPUnique;
ifstream fin;
ofstream fout;
mutex mutexPrint, robots, pages, dns, peeLookup, hstLookup;
int URLCount = 0;
int DNSNum = 0;
int robotsNum = 0;
int pagesNum = 0;
time_t exTime = 0;
time_t dnsTime = 0;
time_t robTime = 0;

hash<string> hasher;
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

void incDNS(time_t time) {
	dns.lock();
	dnsTime += time;
	DNSNum++;
	dns.unlock();
}

void incRobots(time_t time) {
	robots.lock();
	robTime += time;
	robotsNum++;
	robots.unlock();
}

void incPages() {
	pages.lock();
	pagesNum++;
	pages.unlock();
}

size_t doHash(string tohash) {
	auto num = hasher(tohash);
	return num;
}

DWORD getIP(string host) {
	struct sockaddr_in server;
	struct hostent * remote;
	//Exception hadling for access violation writing location
	if ((remote = gethostbyname(host.c_str())) == NULL)
	{
		//printf("Invalid host name string: not FQDN\n");
		return 1; // 1 means failed
	}
	else // take the first IP address and copy into sin_addr
	{
		memcpy((char *)&(server.sin_addr), remote->h_addr, remote->h_length);
	}
	return server.sin_addr.S_un.S_addr; // return IP in binary version
}

bool UniqueHost(string host) {
	hstLookup.lock();
	int unique = doHash(host);
	if (HostsUnique.find(unique) == HostsUnique.end()) {
		HostsUnique.insert(unique);
		hstLookup.unlock();
		return true;
	}
	hstLookup.unlock();
	return false;
}

bool UniqueIP(DWORD ip) {
	peeLookup.lock();
	if (IPUnique.find(ip) == IPUnique.end()) {
		IPUnique.insert(ip);
		peeLookup.unlock();
		return true;
	}
	peeLookup.unlock();
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

void addExtractedTime(time_t time) {
	exTime += time;
}

string getURL() {
	string url;
	if (!fin.eof()) {
		fin >> url;
		return url;
	}
	return "";
}

int ConnectandSend(URLParser parser, string &mess) {

	time_t start, end;

	//Check for host uniqueness
	mess += "Checking host uniqueness... ";
	bool UH = UniqueHost(parser.getHost());
	if (UH) {
		mess += "passed\n";
	}
	else {
		mess += "failed\n";
		return -1;
	}

	//Do DNS lookup
	start = time(0);
	mess += "Doing DNS...\n";
	time_t start = time(0);
	DWORD ip = getIP(parser.getHost());
	if (ip == 1) {
		mess += "failed\n";
		return -1;
	}
	end = time(0);
	incDNS(end - start);
	time_t end = time(0);
	mess += " done in " + to_string(end - start) + " ms, found " + to_string(ip) + "\n";

	//Check for host uniqueness
	mess += "Checking IP uniqueness... ";
	bool UIP = UniqueIP(ip);
	if (UIP) {
		mess += "passed\n";
	}
	else {
		mess += "failed\n";
		return -1;
	}

	//Now you can send
	Winsock wss;
	if (wss.createTCPSocket() == 0) {
		short port = parser.getPort();
		if (wss.connectToServerIP(ip, port, mess) == 0) {
			string req = constructRequest(robot, &parser);
			mess += "Connecting on robots... ";
			start = time(0);
			int sendErr = wss.sendRequest(req);
			end = time(0);
			mess += "done in " + to_string(end-start) +" ms\n";
			start = time(0);
			mess += "Loading... ";
			if (sendErr == 0) {
				string response = "";
				int received = wss.receive(response);
				end = time(0);
				if (received == 0 && response.length() > 10) {
					incRobots(end - start);
					mess += "done in " + to_string(end - start) + "with " + to_string(sizeof(response)) + "bytes\n";
					mess += "Verifying Header... status code " + response.substr(9, 3) + "\n";
				}
				if (received == 0) {
					if (response.length() > 10 && response[9] != '2') {
						Winsock ws;
						if (ws.createTCPSocket() == 0) {
							if (ws.connectToServerIP(ip, port, mess) == 0) {
								response = "";
								string req2 = constructRequest(getr, &parser);
								start = time(0);
								mess += "Connecting on page... ";
								if (ws.sendRequest(req2) == 0) {
									end = time(0);
									mess += "done in " + to_string(end - start) + " ms\n";
									mess += "Loading... ";
									start = time(0);
									int received = ws.receive(response);
									if (received != 0) {
										mess += "failed something went wrong with the get request.";
									}
									else {
										end = time(0);
										incPages();
										mess += "done in " + to_string(start - end) + " ms with " + to_string(sizeof(response)) + "bytes\n";
										//Parse message for success and links. Do not just put response in message
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
	time_t start, end;
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
			start = time(0);
			string url = getURL();
			if (url.compare("") != 0) {
				end = time(0);
				URLCount++;
				URLParser parser(url);
				message += "URL: " + url + "\n";
				message += "Parsing URL... host " + parser.getHost() + ", port " + std::to_string(parser.getPort()) + "\n";
				addExtractedTime(end - start);

				//release mutex
				ReleaseMutex(p->mutex);

				
				ConnectandSend(parser, message);
				//printSafe(message);
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

	Sleep(5000);

	cout << "Extracted " + to_string(URLCount) + " @ " << to_string(exTime) + " ms\n";
	cout << "Looked up " + to_string(DNSNum) + " @ " << to_string(dnsTime) + " ms\n";
	cout << "Downloaded " + to_string(robotsNum) + " robots @ " << to_string(robTime) + " ms\n";
	cout << "Crawled " + to_string(pagesNum) + " pages @ _________/s\n";

	cout << "Enter any key to continue ...\n";
	getchar();

	return 0;
}
