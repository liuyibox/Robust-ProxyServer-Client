#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include "HttpProxy.h"

using namespace std;

string notModified("304");
map<string, Entity*> cache;

string getContentFromWebServer(string, int*);
string getContentFromWebServerIfModified(string, int*, string);
void cacheAdd(string, Entity*);
void cacheUpdate(string, Entity*);
void printCache();
Entity* pageFound(string);
Entity* pageNotFound(string);

int main(int argc, char*argv[]) 
{
  int i, clientSd, fdmax;
  fd_set readfds;
  fd_set master;
  int max_clients = 5;
  char copybuffer[1024];
  char recvbuffer[1024];
  char errorbuffer[256];
  
  
  if(argc < 3)
  {
    fprintf(stderr, "Error: Not enough command line inputs \n");
    exit(1);
  }


  int serverSd = socket(AF_INET, SOCK_STREAM, 0);
//		cout << "server sd: " << serverSd << endl;
  if(serverSd < 0)
  {
    fprintf(stderr,"Server socket creation error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  int yes = 1;
  if(setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("setsockopt error");
    char* errorMessage = strerror_r(errno, errorbuffer, 256 );
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }        

  struct sockaddr_in serverAddrInfo;
  memset((char*) &serverAddrInfo, '0', sizeof(struct sockaddr_in));
  serverAddrInfo.sin_family= AF_INET;
  serverAddrInfo.sin_port = htons(atoi(argv[2]));
  inet_pton(AF_INET, argv[1], &serverAddrInfo.sin_addr);

  struct sockaddr_in clientAddrInfo;

  if(bind(serverSd, (struct sockaddr*) &serverAddrInfo, sizeof(serverAddrInfo))==-1) 
  {
    fprintf(stderr, "Server bind error\n");
    char * errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(2);
  }

  if(listen(serverSd, max_clients) < 0)
  {
    fprintf(stderr, "Listen error");
    char* errorMessage = strerror_r(errno, errorbuffer, 256 );
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(3);
  }

  FD_ZERO(&readfds);
  FD_ZERO(&master);

  FD_SET(serverSd, &master);
  fdmax = serverSd;
  cout<<"Welcome to the HTTP Proxy server."<<endl;

//	cout << "server sd: " << serverSd << endl;
  while(1)
  {
    printf("\n");
    readfds = master;
    memset(copybuffer, 0, sizeof(copybuffer));
    memset(recvbuffer, 0, sizeof(recvbuffer));

    if(select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) 
    {
      fprintf(stderr, "Select error\n");
      char* errorMessage = strerror_r(errno, errorbuffer, 256);
//      printf(errorMessage);
	cout << errorMessage << endl;
      exit(1);
    }

    for(i=3; i<=fdmax; i++)
    {
      if(FD_ISSET(i, &readfds))
      {
        if(i == serverSd)
	{
	  u_int clientLen = sizeof(clientAddrInfo);
	  clientSd = accept(serverSd, (struct sockaddr*) &clientAddrInfo, &clientLen);
 //          	cout << "client sd: " << clientSd << endl;
	  if(clientSd < 0)
	  {
	    fprintf(stderr, "Could not accept connection\n");
	    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//	    printf(errorMessage);
		cout << errorMessage << endl;
	  }
	  else
	  {
	    FD_SET(clientSd, &master);
            if(clientSd > fdmax)
              fdmax = clientSd;
	  }
        }
	else
	{
	  int b = recv(i, recvbuffer, sizeof(recvbuffer), 0);  
	  if(b <= 0)
	  {
	    FD_CLR(i, &master);
	    continue;
	  }

	  string str(recvbuffer, b);
	  //parse the request
          strcpy(copybuffer, recvbuffer);
          string url = extractRequestUri(copybuffer);
          cout<<"The following page has been requested: "<<url<<endl;

          if(cache.count(url) == 0)
          {
            Entity* page = pageNotFound(url); 
			string body = page->getBody();
				
			send(i, body.c_str(), body.length(), 0);
			   
			printCache();
          }
          else
          {
			Entity* page = pageFound(url);
			string body = page->getBody();

            send(i, body.c_str(), body.length(), 0);
           
            printCache();
          }
	}
      }
    }
  }

  close(serverSd);
  return 0;
}


/*
 * method to add new entries to the cache using LRU
 */
void cacheAdd(string url, Entity* entity)
{
  //first check if cache has empty places
  if(cache.size() < MAX_CACHE_SIZE)
  {
    //just insert it and return
    cout<<"Current size of cache: "<<cache.size()<<", is less than the max, so directly inserting"<<endl;
    entity = stampPage(entity);
    cache.insert(make_pair(url, entity));
    return;
  }

  //now if cache is full, find an entity
  //which can be replaced; i.e. the one which has the oldest
  //last accessed time
  int i=0;
  time_t currentTime = getCurrentTime();
  map<string, Entity*>::iterator leastRecentKey;
  double maxTimeDiff=INT_MIN;
  
  map<string, Entity*>::iterator ii = cache.begin();
  for(; ii != cache.end(); ii++)
  {
    Entity* en = ii->second;
    time_t la = en->getHeader()->getLastAccessed();
    double timediff = difftime(currentTime, la);
    if(timediff > maxTimeDiff)
    {
      maxTimeDiff = timediff;
      leastRecentKey = ii;
    }
    i++;
  }

  //now we have the least recently used key in index leastRecent
  cout<<"The least recently used page is "<<leastRecentKey->first<<", hence this entry will be replaced in the cache"<<endl;
  cache.erase(leastRecentKey);
  //entity = stampPage(entity);
  cache.insert(make_pair(url, entity));
}

/*
 * method to replace existing entries in cache
 */
void cacheUpdate(string url, Entity* entity)
{
	string key;
	map<string, Entity*>::iterator ii = cache.begin();
 
	for(; ii != cache.end(); ii++)
	{
		key = ii->first;
		if(key == url)
		{
			cout<<"Entry found for page "<<key<<", this will be replaced with the new page now"<<endl;
			cache.erase(key);
			entity = stampPage(entity);
			cache.insert(make_pair(url, entity));
			break;
		}
	}
}


/*
 * method to get a page from the web server 
 */
//string getContentFromWebServer(string url, int* pageLen)
string getContentFromWebServer(string url)
{
  int breakpoint1, breakpoint2;
  string servername, location;
  char errorbuffer[256];
  
  breakpoint1 = url.find("/");
  if(url.at(breakpoint1) == '/' && url.at(breakpoint1+1) == '/')
  {
    breakpoint2 = url.substr(breakpoint1+2, url.length()-1).find("/");
    servername = url.substr(breakpoint1+2, breakpoint2);
    location = url.substr(breakpoint2+breakpoint1+2, url.length()-1);
  }
  else
  {
    servername = url.substr(0, breakpoint1);
    location = url.substr(breakpoint1, url.length()-1);
  }

  int httpsd;
  if((httpsd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    fprintf(stderr, "HTTP socket creation error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  struct addrinfo *httpinfo;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if(getaddrinfo(servername.c_str(), "http", &hints, &httpinfo) != 0)
  {
    printf("getaddrinfo error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  if(connect(httpsd, httpinfo->ai_addr, httpinfo->ai_addrlen) == -1)
  {
    fprintf(stderr, "Connection to http server error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  string message;
  message = "GET "+url+" HTTP/1.0\r\n\r\n";
  
  cout<<endl<<endl; 
  cout<<"Going to hit the web server with the request: "<<endl<<message;

  int bytes = 0;  
  if((bytes = send(httpsd, message.c_str(), message.length(), 0)) <=0)
  {
    fprintf(stderr, "Send error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }
  
  bytes = 0;
  char recvbuffer[1048576]; //1MB
  if((bytes = recv(httpsd, recvbuffer, 1048576, 0)) <= 0)
  {
    fprintf(stderr, "recv error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }
  
//  *pageLen = bytes;
  string str(recvbuffer);
  return str;
}


/*
 * method to get a page from the web server only if modified 
 */
//string getContentFromWebServerIfModified(string url, int* pageLen, string sinceTime)
string getContentFromWebServerIfModified(string url, string sinceTime)
{
  cout<<"The page "<<url<<" has been found in cache but it has expired"<<endl;

  int breakpoint1, breakpoint2;
  string servername, location;
  char errorbuffer[256];
  
  breakpoint1 = url.find("/");
  if(url.at(breakpoint1) == '/' && url.at(breakpoint1+1) == '/')
  {
    breakpoint2 = url.substr(breakpoint1+2, url.length()-1).find("/");
    servername = url.substr(breakpoint1+2, breakpoint2);
    location = url.substr(breakpoint2+breakpoint1+2, url.length()-1);
  }
  else
  {
    servername = url.substr(0, breakpoint1);
    location = url.substr(breakpoint1, url.length()-1);
  }

  int httpsd;
  if((httpsd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    fprintf(stderr, "HTTP socket creation error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

	cout << "we are in if modified, httpsd: " << httpsd << endl;

  struct addrinfo *httpinfo;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if(getaddrinfo(servername.c_str(), "http", &hints, &httpinfo) != 0)
  {
    printf("getaddrinfo error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  if(connect(httpsd, httpinfo->ai_addr, httpinfo->ai_addrlen) == -1)
  {
    fprintf(stderr, "Connection to http server error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  int bytes = 0;
  string message;
  message = "GET "+url+" HTTP/1.0\r\n"+"If-Modified-Since: "+sinceTime+"\r\n\r\n";
  cout<<endl<<endl;
  cout<<"Going to hit the web server with the conditional request: "<<endl<<message;

  if((bytes = send(httpsd, message.c_str(), message.length(), 0)) <=0)
  {
    fprintf(stderr, "Send error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }
  
  bytes = 0;
  char recvbuffer[1048576]; //1MB
  if((bytes = recv(httpsd, recvbuffer, 1048576, 0)) <= 0)
  {
    fprintf(stderr, "recv error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }
	close(httpsd);
//  cout << "received buffer: "+ string(recvbuffer) << endl;
//  cout << "received size: "<< bytes << endl;
//  *pageLen = bytes;
//	cout << "received size: "<< bytes << endl;
  string str(recvbuffer);

  //analyze the response
  string temp(str);
  vector<string> v1 = strsplit(temp, "\r\n");
  string status(v1.at(0));
  cout<<"The response from the web server is: "<<status<<endl<<endl;
  
  vector<string> v = strsplit(status, " ");
  string blank;
  if(v.at(1) == notModified)
    return blank;
  else
    return str;
}


/*
 * method to handle pages that are found in the cache
 */
Entity* pageFound(string url)
{
  map<string, Entity*>::iterator ii = cache.find(url);
  Entity* en = ii->second;
            
  //now we first check if the page is expired
  string expires = en->getHeader()->getExpires();
 
  //if Expires field is not there, use the Last-modified or the Date field, in that order
  if(expires.empty())
  {
    expires = en->getHeader()->getLastModified();
    if(expires.empty())
      expires = en->getHeader()->getDate();
  }
  
  int current, ex_int;
  time_t ex = toTimeT(expires);
  if(ex <= 0) //handle spl cases
  {
    //this means that either expires field was not there,
    //or it had some value other than a date. In any case,
    //make sure we go to webserver
    cout<<"The expires field is not a timestamp: "<<expires;
    ex_int = -1;
  }
  else
    ex_int = int(ex);
    
  current = int(getCurrentTime());
  if(current > ex_int)
  {
    //this means that the page has expired,
    //so now we get it from the webserver or serve it from cache if not modified
    cout<<"We have a page whose entry in the cache was expired"<<endl;
//    int* pageLen;
//    string pageExpired = getContentFromWebServerIfModified(url, pageLen, expires);
    string pageExpired = getContentFromWebServerIfModified(url, expires);

    //since we do a CONDITIONAL-GET, it could also be the case that there was no
    //need to get the page from the web server; in that case the above string would be 
    //empty
    if(pageExpired.empty())
    {
      cout<<"No need to get the page "<<url<<" from the server as it was not modified on the server"<<endl;
      en = stampPage(en); //serve from cache itself
      return en;
    }
    else
    {
//      Entity* newEntity = parseResponse(pageExpired, *pageLen);
      Entity* newEntity = parseResponse(pageExpired);
      cacheUpdate(url, newEntity);
      cout<<"The page "<<url<<" has been replaced in cache"<<endl;
      return newEntity; //return the page from the server
    }
  }
  else
  {
    //if page hasnt expired in the cache
    cout<<"The page "<<url<<" has been found in cache and it is fresh"<<endl;
    en = stampPage(en);
    return en;
  }
}


/*
 * method to handle pages that are not found in the cache
 */
Entity* pageNotFound(string url)
{
//  int* pageLen = new int;
  cout<<"The page "<<url<<" was not found in the cache"<<endl;
//  string page = getContentFromWebServer(url, pageLen);
  string page = getContentFromWebServer(url);

//  Entity* newPage = parseResponse(page, *pageLen);
  Entity* newPage = parseResponse(page);
  if(newPage != NULL)
    cout<<"We have a new page"<<endl;

  cacheAdd(url, newPage);
  cout<<"The page "<<url<<" has been stored in cache"<<endl;

  return newPage;
}

void printCache()
{
  Entity* en;
  map<string, Entity*>::iterator ii = cache.begin();
  string key, sla;
  cout<<endl;
  cout<<"======Proxy Cache======"<<endl;
  cout<<"Size of the cache: "<<cache.size()<<endl;
  for(; ii != cache.end(); ii++)
  {
    key = ii->first;
    en = ii->second;

    EntityHeader* hdr = en->getHeader();
    time_t la = hdr->getLastAccessed();
    sla = fromTimeT(la);

    cout<<"page: "<<key<<", last accessed: "<<sla<<endl;
  }
}
