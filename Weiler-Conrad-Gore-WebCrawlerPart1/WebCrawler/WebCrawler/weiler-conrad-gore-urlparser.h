#pragma once
#include "common.h"
#include <regex>
#include <string>

class URLParser {

public:

	// constructor 
	URLParser(string &link)  // & means pass the address of link to this function
	{
		url = link;
		host = "";
		port = 80;  // default port is 80 for web server
		path = "/";  // if path is empty, use "/"
		query = "";
	}


	/* url format:
	* scheme://[user:pass@]host[:port][/path][?query][#fragment]
	*/

	// e.g., url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// host: "cs.somepage.edu"
	string getHost()
	{
		// implement here, you may use url.find(...)
		string temp = url.substr(7, url.length());

		regex rgx("[^:/][^:/]*");
		smatch match;
		regex_search(temp, match, rgx);
		if (match.size() == 1){
			host = match[0];
		}

		return host;
	}

	// e.g., url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// port: 467
	short getPort()
	{
		string sPort = "";

		// implement here: find substring that represents the port number
		regex rgx("[:][0-9][0-9]*");
		smatch match;
		regex_search(url, match, rgx);

		if (match.size() == 1){
			string temp = match[0];
			return atoi(temp.substr(1, temp.length()).c_str());
		}

		if (sPort.length() > 0)
			port = atoi(sPort.c_str());  // convert substring sPort to an integer value

		return port;
	}

	// url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// path is "/index.php"
	string getPath()
	{
		// implement here
		string temp = url.substr(12, url.length());
		regex rgx("[/][^?]*");
		smatch match;
		regex_search(temp, match, rgx);
		if (match.size() == 1){
			//cout << "path: '" << match[0] << "'" << endl;
			path = match[0];
		}
		return path;
	}

	// url: "http://cs.somepage.edu:467/index.php?addrbook.php"
	// query is "?addrbook.php"
	string getQuery()
	{
		// implement here
		regex rgx("[?].*");
		smatch match;
		regex_search(url, match, rgx);
		if (match.size() == 1){
			//cout << "query: '" << match[0] << "'" << endl;
			query = match[0];
		}
		return query;
	}


private:
	string url;
	string host;
	short port;
	string path;
	string query;
};