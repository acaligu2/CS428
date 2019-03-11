#include <vector>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

typedef struct{

	string url;			//Url of requested item
	string responseMsg;		//Full HTTP Response message
	int responseSize;       	//Size of the HTTP Response
	int lengthOfContent;    	//Length of resource requested

}cacheVal;

class Cache{

	private:

	  int bytes;			//Size of items in the cache
	  int cacheSize = -1;		//Size of the entire cache (Command Line Argument)
	  vector<cacheVal> cache;	//"Cache" --> Vector of cache items
	  pthread_mutex_t accessLock;	//Lock to protect manipulating the cache


	public:

	  Cache();
	  void setMaxSize(int size);
	  int find(string itemUrl);
	  void insert(cacheVal newItem);
	  cacheVal getItem(string itemUrl);

};
