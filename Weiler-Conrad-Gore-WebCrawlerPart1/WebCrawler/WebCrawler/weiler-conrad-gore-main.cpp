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
unordered_set<string> IPUnique;
ifstream fin;
ofstream fout;
enum Request {robot, head, getr};

bool UniqueHost(string host) {
	std::unordered_set<std::string>::const_iterator it = HostsUnique.find(host);
	if (it == HostsUnique.end()) {
		HostsUnique.insert(host);
		return true;
	}
	else {
		return false;
	}
}

bool UniqueIP(string host) {
	struct hostent *remote;
	if ((remote = gethostbyname(host.c_str())) == NULL)
	{
		cout << "Invalid host name string: not FQDN\n" << endl;
		return false;  // 1 means failed
	}
	else {
		std::unordered_set<std::string>::const_iterator it = IPUnique.find(remote->h_name);
		if (it == IPUnique.end()) {
			IPUnique.insert(remote->h_name);
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

void ConnectandSend(URLParser parser, Winsock wss) {
	//Winsock::initialize();	// initialize 

	//Winsock ws;
	string host = parser.getHost();
	short port = parser.getPort();
	string path = parser.getPath();
	string query = parser.getQuery();

	// the following shows how to use winsock functions
	//wss.createTCPSocket();
	wss.connectToServer(host, port);

	// test for robot file
	string req = constructRequest(getr, &parser);
	wss.sendRequest(req);

	// receive robot reply
	string response = "";
	int received = wss.receive(response);
	if (received == 0) {
		cout << response << endl;
	}
	else if (received == 2) {
		cout << "Did not receive response within allotted time.\n";
	}

	//if no robot, send get request
	//else, do not send request

	//wss.closeSocket();
	// parse url to get host name, port, path, and so on.
	//Winsock::cleanUp();
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
	string url = getURL();
	while (url.compare("") != 0) {
		Winsock::initialize();	// initialize
		Winsock ws;
		ws.createTCPSocket();
		URLParser parser(url);
		if (UniqueHost(parser.getHost()) && UniqueIP(parser.getHost())) {
			ConnectandSend(parser, ws);
		}
		ws.closeSocket();
		// parse url to get host name, port, path, and so on.
		Winsock::cleanUp();
		url = getURL();
	}
	///////////////////////////////

	fin.close(); //fout.close();

	cout << "Enter any key to continue ...\n";
	getchar();

	return 0;
}
