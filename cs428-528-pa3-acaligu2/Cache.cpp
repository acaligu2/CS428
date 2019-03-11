#include "Cache.h"

//Initialize bytes being used and pthread lock
Cache::Cache(){

  bytes = 0;
  pthread_mutex_init(&accessLock, NULL);

}

//Set max size based on command line argument
void Cache::setMaxSize(int size){

  cacheSize = size;

  if(cacheSize == -1){

    perror("Error initializing size");
    exit(1);

  }

}

//Based on an item's url, determine if it has been cached
int Cache::find(string itemUrl){

  int locationIndex = -1;

  for(int i = 0; i < cache.size(); i++){

    //Item has been found, return index
    if(cache[i].url == itemUrl){

      locationIndex = i;
      break;

    }

  }

  return locationIndex;

}

//Insert a cache item into the cache
void Cache::insert(cacheVal newItem){

  //Lock the operation
  pthread_mutex_lock(&accessLock);

  //Need to evict from cache until item can fit
  while(newItem.responseSize > cacheSize - bytes){

    //Update bytes being used
    bytes -= cache[cache.size() - 1].responseSize;

    //Remove from cache
    cache.erase(cache.end() - 1);

  }

  //Insert new item as most recently used
  cache.insert(cache.begin(), newItem);

  //Update bytes
  bytes += newItem.responseSize;

  //Unlock operation
  pthread_mutex_unlock(&accessLock);

}

//Return an item from a url
//Used when a cache hit has been determined
cacheVal Cache::getItem(string itemUrl){

  pthread_mutex_lock(&accessLock);

  cacheVal c;

  //Determine if url has been cached
  int itemIndex = find(itemUrl);

  //Item hasn't been cached, return an empty cacheVal
  if(itemIndex == -1){

    c = {"", "", -1, -1};

  //Url is in cache
  }else{

    c = cache[itemIndex];

    //Preserve LRU property
    if(itemIndex != 0){

      //remove from current position
      cache.erase(cache.begin() + itemIndex);

      //Insert at front
      cache.insert(cache.begin(), c);

    }

  }

  pthread_mutex_unlock(&accessLock);

  return c;

}
