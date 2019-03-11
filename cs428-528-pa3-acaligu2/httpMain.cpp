#include <stdio.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <vector>
#include <stdlib.h>

#include "Cache.h"

using namespace std;

int cacheSize;	//Size of cache
Cache cache;	//Object containing HTTP cache

//Connect to the server and retreive the object
cacheVal contactServer(string requestMsg, string url){

	//----------Split request message by line for parsing----------//
	string line;
	vector<string> msgLines;

	istringstream strStream(requestMsg);
	while(getline(strStream, line)){

		msgLines.push_back(line);

	}

	string hostName = "";
	int portVal = -1;
	int portSize = -1;

	//----------Extract header for appending to request for server----------//
	int endOfHeader = requestMsg.find("\r\n");
	string header = requestMsg.substr(endOfHeader + 2);

	//----------Extract host name of server----------//
	int indexHost = msgLines[4].find("Host: ") + 6;
	hostName = msgLines[4].substr(indexHost, msgLines[4].size() - (indexHost + 1));

	int colonLocation = hostName.find(":");
	//No port specified, default to 80
	if(colonLocation == -1){

		portVal = 80;
		portSize = 0;

	//Port has been specified
	}else{

		//Parse out port number
		string portString = hostName.substr(colonLocation + 1);

		//Determine port size
		portSize = portString.size() + 1;

		//Cast to integer
		portVal = stoi(portString);

		//Remove ":<portNumber>" from hostName
		hostName.erase(colonLocation);

	}

	//----------Extract the resource requsted from the server----------//
	int hostI = url.find(hostName);
	string resourcePath = url.substr(hostI + hostName.size() + portSize);

	//----------Create new HTTP Request message with resource path----------//
	string newReq = "GET " + resourcePath + " HTTP/1.1\r\n";
	newReq = newReq + header;

	//----------Create a socket and connect to desired server----------//
	int httpServSock;

	httpServSock = socket(AF_INET, SOCK_STREAM, 0);

	struct hostent* h;

	//Get IP from hostName
	h = gethostbyname(hostName.c_str());

	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portVal);

	memcpy((void*)&serverAddress.sin_addr, h->h_addr_list[0], h->h_length);

	socklen_t addrLen = sizeof(serverAddress);

	if(connect(httpServSock, (struct sockaddr*) &serverAddress, addrLen) < 0){

		perror("Couldn't connect");
		exit(1);

	}

	//Cast request for server to char buffer
	const char* reqBuff = newReq.c_str();

	//Send buffer to server
	if(send(httpServSock, reqBuff, strlen(reqBuff) + 1, 0) == -1){

		perror("Couldn't send");
		exit(1);

	}

	string responseMsg;
	int responseSize = 0;

	//Loop until we receive the entire response from the server
	while(1){

		char responseBuff[1048576];

		int bytesRecv = recv(httpServSock, responseBuff, 1048576, 0);

		//All data has been sent, exit the loop
		if(bytesRecv == 0){ break; }

		//Add info to string being sent back to client
		responseMsg.append(responseBuff, bytesRecv);

		//Increment size
		responseSize += bytesRecv;

	}

	close(httpServSock);

	//Grab size of resource
	int lenIndex = responseMsg.find("Content-Length") + 16;
	string contentLength = responseMsg.substr(lenIndex);
	contentLength = contentLength.substr(0, contentLength.find("\r"));

	//New instance of struct for caching this request
	cacheVal newCache = {url, responseMsg, responseSize, stoi(contentLength)};

	//Add to the cache for fast lookup
	cache.insert(newCache);

	return newCache;

}

//Parses request from client and returns the object requsted
void* httpRequestHandler(void* arg){

	//Start clock
	auto start = chrono::high_resolution_clock::now();

	string peerIp = "";		//Client's IP --> Used to send item
	string cacheDecision = "";	//Used to determine cache hit or miss

	int socket = *(int *)arg;

	struct sockaddr_in sa1;
	socklen_t addressLength1 = sizeof(sa1);

	getpeername(socket, (struct sockaddr*)&sa1, &addressLength1);
	peerIp = inet_ntoa(sa1.sin_addr);

	char charBuff[2048];

	//Receiving HTTP Request from client
	int bytesRecv = recv(socket, charBuff, 2047, 0);

	if(bytesRecv == -1){

		perror("Failed to receive request");
		exit(1);

	}

	charBuff[2048] = '\0';

	//Cast received char buffer to a C++ string
	string requestMsg = charBuff;

	//----------Parse out "GET" and HTTP Version at end of first line----------//
	int endOfLine = requestMsg.find("\r");
	string url = requestMsg.substr(0, endOfLine);
	url = url.substr(4);
	url = url.substr(0, url.find(" HTTP"));

	cacheVal requestItem;

	//First check if the item is cached
	requestItem = cache.getItem(url);

	//Item isn't cached, ask server for resource
	if(requestItem.url == ""){

		requestItem = contactServer(requestMsg, url);
		cacheDecision = "CACHE_MISS";

	//Item is cached
	}else{

		cacheDecision = "CACHE_HIT";

	}

	//Buffer to send data to client
	char* rBuff = new char[requestItem.responseSize];

	//Copy in byte data of response from server into the buffer
	memcpy(rBuff, requestItem.responseMsg.data(), requestItem.responseSize);

	int bytesSent = 0;					//Bytes sent through socket
	int bytesLeftToSend = requestItem.responseSize;		//Bytes left to send through socket

	//Loop until all required bytes have been sent
	while(bytesSent < requestItem.responseSize){

		//Send HTTP Message to client
		int bytePortion = send(socket, rBuff + bytesSent, bytesLeftToSend, 0);

		//Increment number of bytes sent
		bytesSent += bytePortion;

		//Decrement bytes left to send
		bytesLeftToSend -= bytePortion;

	}

	close(socket);

	//End clock
	auto end = chrono::high_resolution_clock::now();

	//Using microseconds to get better precision
	chrono::microseconds microseconds = chrono::duration_cast<chrono::microseconds>(end - start);

	double precise = (double) microseconds.count() / 1000;


	cout << peerIp << "|" << url << "|" << cacheDecision << "|" <<
		requestItem.lengthOfContent << "|" << precise << endl;

	//free memory for response buffer
	delete [] rBuff;

	return NULL;

}

int main(int argc, char *argv[]) {

	if(argc != 2){

		cout << "./httpProxy <cacheSize>" << endl;
		exit(1);

	}else{

		cacheSize = stoi(argv[1]);

	}

	cache.setMaxSize(cacheSize);

	//----------Establish connection with wget client----------//
	int pSock;
	struct sockaddr_in s;

	char * proxyHost;
	ushort proxyPort;

	pSock = socket(AF_INET, SOCK_STREAM, 0);
	if(pSock < 0){

		perror("Failure creating socket");
		return -1;

	}

	s.sin_family = AF_INET;
	s.sin_port = 0;
	s.sin_addr.s_addr = INADDR_ANY;

	if(bind(pSock, (struct sockaddr *) &s, sizeof(s)) < 0){
		perror("Socket couldn't bind");
		return -1;
	}

	listen(pSock, 10);

	proxyHost = (char*) malloc(128);
	char partialHostName[128];
	partialHostName[127] = '\0';
	gethostname(partialHostName, 127);
	struct hostent *he = gethostbyname(partialHostName);

	memcpy(&s.sin_addr.s_addr, he->h_addr, he->h_length);

	strcpy(proxyHost, he->h_name);

	socklen_t addrLength = sizeof(s);

	if(getsockname(pSock, (struct sockaddr*) &s, &addrLength) == -1){

		perror("Couldn't getsockname()");
		return -1;

	}else{

		proxyPort = ntohs(s.sin_port);


	}

	cout << "Started Proxy: {" << proxyHost << ":" << proxyPort << "}" << endl;

	free(proxyHost);

	//Ready to receive requests
	while(1){

		struct sockaddr_in clientInfo;
		socklen_t clientAddrSize = sizeof(clientInfo);

		int clientSock = accept(pSock, (sockaddr *)&clientInfo, &clientAddrSize);

		if(clientSock == -1){

			perror("Couldn't accept connection");
			exit(1);

		}

		char* clienthost; /* host name of the client */
		ushort clientport; /* port number of the client */

		struct sockaddr_in address1;
		socklen_t addressLength = sizeof(address1);
		struct hostent *host;

		getpeername(clientSock, (struct sockaddr*)&address1, &addressLength);

		host = gethostbyaddr(&(address1.sin_addr), sizeof(addressLength), AF_INET);

		clientport = ntohs(address1.sin_port);
		clienthost = host->h_name;

		pthread_t handlerThread;

		int* s = &clientSock;

		//Thread is used to handle each individual request
		pthread_create(&handlerThread, NULL, httpRequestHandler, (void*) s);
		pthread_join(handlerThread, NULL);

	}

	return 0;

}
