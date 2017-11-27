#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include "HttpProxy.h"



using namespace std;



int main(int argc, char* argv[]) {

    int sd;

    char error_buffer[256];

    char recv_buffer[1048576];

    if (argc != 4) {

        fprintf(stderr, "\nUsage: client <ip_address> <port number> <URL>\n");

        exit(1);

    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

        fprintf(stderr, "Client creating error\n");

//    char* errorMessage = strerror_r(errno, errorbuffer, 256);

//    printf(errorMessage);
//	  cout << errorMessage << endl;

        exit(1);

    }

    struct sockaddr_in server_addr_info;
    memset((char*) & server_addr_info, '0', sizeof(struct sockaddr_in));

    server_addr_info.sin_family= AF_INET;
    server_addr_info.sin_port = htons(atoi(argv[2]));

    inet_pton(AF_INET, argv[1], &server_addr_info.sin_addr);

    if (connect(sd, (struct sockaddr*)&server_addr_info, sizeof(server_addr_info)) == -1) {

        fprintf(stderr, "Connecting to server error\n");

//    char* errorMessage = strerror_r(errno, errorbuffer, 256);

//    printf(errorMessage);
//	  cout << errorMessage << endl;

        exit(1);

    }

    printf("\nConnected\n");

    string url(argv[3]);
    string msg = "GET " + url + " HTTP/1.0\r\n\r\n";

    cout<<endl<<"Going to hit the proxy server with the request: "<<endl<<msg;

    int byte = send(sd, msg.c_str(), msg.length(), 0);

    byte = 0;

    if ((byte = recv(sd, recv_buffer, 1048576, 0)) <= 0) {

        fprintf(stderr, "No data received from proxy, exiting...\n");

        exit(1);

    } else {

        string recvd(recv_buffer, byte);
        cout << "Got the content from: " + url << endl;

    }

    close(sd);

    return 0;

}
