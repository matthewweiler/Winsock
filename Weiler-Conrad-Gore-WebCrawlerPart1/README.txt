Names: Matthew Weiler, Jonathan Conrad, Joseph Gore

Project Details:

	URL (hardcoded in weiler-conrad-gore-main.cpp):
		http://www.weatherline.net/

	
	Requirements Fullfilled:
		This code fullfills part 1 in that it downloads the url http://www.weatherline.net/ and displays the contents to the console.
			In order to do this, we needed to parse the url with creating the parser getHost(), getPort(), getPath(), and getQuery()
			functions. In addition to that, we used those functions to create the request and in turn implemented the sendRequest function
			to send the GET request to the specified server.
		In addition to that, we also implemented the receive function in order to recieve the bytes back from the server. We appended those bytes
			to a response and sent that back to be displayed on the screen.

Notes:
	We have included a smaple screenshot of the and of the html captured and then also have included the .sln file so that you can run the code in
		Visual Studio if you so wish. To run the code, compile together in Visual Studio and run from the weiler-conrad-gore-main.cpp file.