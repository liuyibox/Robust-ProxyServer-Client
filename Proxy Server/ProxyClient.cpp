#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include "HttpProxy.h"

using namespace std;

int main(int argc, char* argv[]) 
{
  int sd;
  char errorbuffer[256];
  char recvbuffer[1048576];

  if(argc != 4)
  {
    fprintf(stderr, "\nUsage: client <proxy_server_ip> <proxy_server_port> <URL to retrieve>\n");
    exit(1);
  }

  if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
  {
    fprintf(stderr, "Client socket creation error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  struct sockaddr_in serverAddrInfo;
  memset((char*) & serverAddrInfo, '0', sizeof(struct sockaddr_in));
  serverAddrInfo.sin_family= AF_INET;
  serverAddrInfo.sin_port = htons(atoi(argv[2]));
  inet_pton(AF_INET, argv[1], &serverAddrInfo.sin_addr);

  if(connect(sd, (struct sockaddr*)&serverAddrInfo, sizeof(serverAddrInfo)) == -1) 
  {
    fprintf(stderr, "Connection to server error\n");
    char* errorMessage = strerror_r(errno, errorbuffer, 256);
//    printf(errorMessage);
	cout << errorMessage << endl;
    exit(1);
  }

  printf("\nConnected\n");
  string url(argv[3]);
  string message = "GET "+url+" HTTP/1.0\r\n\r\n";
  cout<<endl<<"Going to hit the proxy server with the request: "<<endl<<message;
  int bytes = send(sd, message.c_str(), message.length(), 0);

  bytes = 0;
  if((bytes = recv(sd, recvbuffer, 1048576, 0)) <= 0)
  {
    fprintf(stderr, "No data received from proxy, exiting.\n");    
    exit(1);
  }
  else
  {
    string recvd(recvbuffer, bytes);
	cout << "we have got the content from: "+url << endl;
//    cout<<recvd<<endl;
  }
   
  close(sd);
  return 0;
}
