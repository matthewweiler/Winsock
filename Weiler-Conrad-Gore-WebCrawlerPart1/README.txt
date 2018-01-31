Names: Matthew Weiler, Jonathan Conrad, Joseph Gore

Project Details:

	Command line arguments:
		<Number-of-Threads> <Path-to-File>
	NOTE*: the first command line argument is not implemented yet so there is only 1 thread in this example. Please just use 1 for the
			<Number-of-Threads> Command line argument. Also note that the input file is not included as it can use any.
			arbitrary urls file that is line separated.

	
	Requirements Fullfilled:

		This code fullfills part 2 in that it parses a text file of urls line by line and then sends a HEAD request to the hosts for a robots
			file. If the status code starts with 4, it will then send that same host a GET request for the path that was specified in the
			url. All this will only happen if the host and IP are unique and have not been seen before. In addition to this, the DNS
			lookup for the URL only happens once in this code so that the DNS server does not get flooded with network traffic.

		This code just builds on top of the part 1 code in that it is only single threaded and will loop through the urls by itself. This code 
			also aims to start setting up the final printing format for the final project that is due in 2 weeks. Next, time stamps and 
			multithreading will be added. In addition to that, status codes will be analyzed for their full number so that further analysis 
			on the response can be added.

		Finally, the urls that we are able to crawl will have the page downloaded and then will count and display the number of links on the page
			as specified in the spec sheet.

Notes:
	We have included the .sln file so that you can run the code in
		Visual Studio if you so wish. To run the code, compile together in Visual Studio and run from the weiler-conrad-gore-main.cpp file.