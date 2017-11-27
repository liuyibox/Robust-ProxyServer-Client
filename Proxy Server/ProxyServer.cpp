#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <map>
#include <limits.h>
#include <errno.h>
#include "HttpProxy.h"



using namespace std;



string not_modified("304");

map<string, Entity*> ca;

string get_content_from_web_server(string, int *);
string get_content_from_web_server_if_modified(string, int *, string);

void cache_add(string, Entity *);

void cache_update(string, Entity *);

void print_cache();

Entity* page_found(string);

Entity* page_not_found(string);



int main(int argc, char*argv[]) {

    int i;
    int clientSd;
    int fdmax;

    int maxclients = 5;

    char copy_buffer[1024];

    char recv_buffer[1024];

    char error_buffer[256];

    fd_set readfds;

    fd_set master;

    if (argc < 3) {

        fprintf(stderr, "\nUsage: server <ip_address> <port number>\n");

        exit(1);

    }

    int serverSd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSd < 0) {

        fprintf(stderr,"Server creating error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//        cout << errorMessage << endl;

        exit(1);

    }

    int yes = 1;

    if (setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {

        perror("setsockopt error");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256 );
//        printf(errorMessage);
//        cout << errorMessage << endl;

        exit(1);

    }

    struct sockaddr_in server_addr_info;

    memset((char*) &server_addr_info, '0', sizeof(struct sockaddr_in));

    server_addr_info.sin_family= AF_INET;

    server_addr_info.sin_port = htons(atoi(argv[2]));

    inet_pton(AF_INET, argv[1], &server_addr_info.sin_addr);

    struct sockaddr_in client_addr_info;

    if (bind(serverSd, (struct sockaddr*) &server_addr_info, sizeof(server_addr_info)) == -1) {

        fprintf(stderr, "Server binding error\n");

//        char * errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//        cout << errorMessage << endl;

        exit(2);

    }

    if (listen(serverSd, maxclients) < 0) {

        fprintf(stderr, "Listening error");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256 );
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(3);
    }

    FD_ZERO(&readfds);

    FD_ZERO(&master);

    FD_SET(serverSd, &master);

    fdmax = serverSd;

    cout<<"Welcome to the HTTP Proxy server."<<endl;

    while(1) {

        printf("\n");
        readfds = master;

        memset(copy_buffer, 0, sizeof(copy_buffer));

        memset(recv_buffer, 0, sizeof(recv_buffer));

        if (select(fdmax + 1, &readfds, NULL, NULL, NULL) == -1) {

            fprintf(stderr, "Select error\n");

//            char* errorMessage = strerror_r(errno, errorbuffer, 256);
//            printf(errorMessage);
//	          cout << errorMessage << endl;

            exit(1);

        }

        for (i = 3; i <= fdmax; i++) {

            if (FD_ISSET(i, &readfds)) {

                if (i == serverSd) {

                    u_int clientLen = sizeof(client_addr_info);
                    clientSd = accept(serverSd, (struct sockaddr*) &client_addr_info, &clientLen);

                    if (clientSd < 0) {

                        fprintf(stderr, "Could not accept connection\n");
//                        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//	                      printf(errorMessage);
//		                  cout << errorMessage << endl;

                    } else {

                        FD_SET(clientSd, &master);

                        if (clientSd > fdmax) {

                            fdmax = clientSd;

                        }

                    }
                } else {

                    int b = recv(i, recv_buffer, sizeof(recv_buffer), 0);

                    if (b <= 0) {

                        FD_CLR(i, &master);

                        continue;

                    }

                    string str(recv_buffer, b);

                    strcpy(copy_buffer, recv_buffer);
                    string url = extract_uri(copy_buffer);

                    cout<<"The following page has been requested: "<<url<<endl;

                    if (ca.count(url) == 0) {

                        Entity* page = page_not_found(url);
                        string body = page->get_body();

                        send(i, body.c_str(), body.length(), 0);
                        print_cache();

                    } else {

                        Entity* page = page_found(url);
                        string body = page->get_body();

                        send(i, body.c_str(), body.length(), 0);
                        print_cache();

                    }

                }

            }

        }

    }

    close(serverSd);

    return 0;
}



void cache_add(string url, Entity *entity) {

    // add new entries to the cache using LRU
    
    if (ca.size() < MAX_CACHE_SIZE) {

        cout<<"Current size of cache: "<<ca.size()<<", is smaller than the max, directly inserting"<<endl;

        entity = stamp_page(entity);
        ca.insert(make_pair(url, entity));

        return;

    }

    int i = 0;

    time_t current_time = get_current_time();
    map<string, Entity*>::iterator least_recent_key;

    double max_time_diff = INT_MIN;

    map<string, Entity*>::iterator it = ca.begin();
    for (; it != ca.end(); it++) {

        Entity* en = it->second;
        time_t la = en->get_header()->get_last_accessed();
        double time_diff = difftime(current_time, la);

        if (time_diff > max_time_diff) {

            max_time_diff = time_diff;
            least_recent_key = it;

        }

        i++;
    }

    cout<<"The least recently used page is "<<least_recent_key->first<<", so it will be replaced in the cache"<<endl;

    ca.erase(least_recent_key);
    ca.insert(make_pair(url, entity));
}



void cache_update(string url, Entity *entity) {

    // replace existing entries in cache

    string key;
	map<string, Entity*>::iterator ii = ca.begin();

    for (; ii != ca.end(); ii++) {

        key = ii->first;
        if (key == url) {

			cout<<"Entry found for page "<<key<<", it will be replaced with the new page now"<<endl;

			ca.erase(key);

			entity = stamp_page(entity);

			ca.insert(make_pair(url, entity));

			break;
		}

	}

}



string get_content_from_web_server(string url) {

    // get a page from the web server

    int break_point_1, break_point_2;

    string server_name;
    string loc;

    char error_buffer[256];

    break_point_1 = url.find("/");

    if (url.at(break_point_1) == '/' && url.at(break_point_1+1) == '/') {

        break_point_2 = url.substr(break_point_1+2, url.length()-1).find("/");

        server_name = url.substr(break_point_1+2, break_point_2);
        loc = url.substr(break_point_2+break_point_1+2, url.length()-1);

    } else {

        server_name = url.substr(0, break_point_1);
        loc = url.substr(break_point_1, url.length()-1);

    }

    int http_sd;

    if((http_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

        fprintf(stderr, "HTTP socket creating error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(1);

    }

    struct addrinfo *http_info;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;

    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_name.c_str(), "http", &hints, &http_info) != 0) {

        printf("getaddrinfo error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//        cout << errorMessage << endl;

       exit(1);

    }

    if (connect(http_sd, http_info->ai_addr, http_info->ai_addrlen) == -1) {

        fprintf(stderr, "Connecting to http server error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(1);

    }

    string msg;
    msg = "GET " + url + " HTTP/1.0\r\n\r\n";

    cout<<endl<<endl;

    cout<<"Going to hit the web server with the request: "<<endl<<msg;

    int byte = 0;

    if ((byte = send(http_sd, msg.c_str(), msg.length(), 0)) <=0) {

        fprintf(stderr, "Sending error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
// 	      cout << errorMessage << endl;

        exit(1);

    }

    byte = 0;
    char recv_buffer[1048576];

    if ((byte = recv(http_sd, recv_buffer, 1048576, 0)) <= 0) {

        fprintf(stderr, "receiving error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//        cout << errorMessage << endl;

        exit(1);
    }

    string str(recv_buffer);

    return str;

}



string get_content_from_web_server_if_modified(string url, string since_time) {

    // get a page from the web server only if modified

    cout<<"The page "<<url<<" has been found in cache but it has expired"<<endl;

    int break_point_1, break_point_2;

    string server_name;

    string loc;

    char errorbuffer[256];

    break_point_1 = url.find("/");

    if (url.at(break_point_1) == '/' && url.at(break_point_1+1) == '/') {

        break_point_2 = url.substr(break_point_1+2, url.length()-1).find("/");

        server_name = url.substr(break_point_1+2, break_point_2);

        loc = url.substr(break_point_2+break_point_1+2, url.length()-1);

    } else {

        server_name = url.substr(0, break_point_1);
        loc = url.substr(break_point_1, url.length()-1);

    }

    int http_sd;

    if ((http_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

        fprintf(stderr, "HTTP socket creating error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(1);

    }

	cout << "In if modified, httpsd: " << http_sd << endl;

    struct addrinfo *http_info;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;

    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_name.c_str(), "http", &hints, &http_info) != 0) {

        printf("getaddrinfo error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(1);

    }

    if (connect(http_sd, http_info->ai_addr, http_info->ai_addrlen) == -1) {

        fprintf(stderr, "Connecting to http server error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(1);
    }

    int byte = 0;

    string msg;
    msg = "GET " + url + " HTTP/1.0\r\n" + "If-Modified-Since: " + since_time + "\r\n\r\n";

    cout<<endl<<endl;

    cout<<"Going to hit the web server with the conditional request: "<<endl<<msg;

    if ((byte = send(http_sd, msg.c_str(), msg.length(), 0)) <=0) {

        fprintf(stderr, "Sending error\n");
//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//        cout << errorMessage << endl;

        exit(1);

    }

    byte = 0;
    char recv_buffer[1048576];

    if ((byte = recv(http_sd, recv_buffer, 1048576, 0)) <= 0) {

        fprintf(stderr, "receiving error\n");

//        char* errorMessage = strerror_r(errno, errorbuffer, 256);
//        printf(errorMessage);
//	      cout << errorMessage << endl;

        exit(1);

    }

    close(http_sd);

    string str(recv_buffer);

    string temp(str);

    vector<string> v1 = string_split(temp, "\r\n");

    string status(v1.at(0));

    cout<<"The response from the web server is: "<<status<<endl<<endl;

    vector<string> v = string_split(status, " ");
    string blank;

    if (v.at(1) == not_modified) {

        return blank;

    } else {

        return str;

    }

}



Entity* page_not_found(string url) {

    // handle pages which are not found in the cache

    cout<<"The page "<<url<<" can not be found in the cache"<<endl;

    string page = get_content_from_web_server(url);

    Entity* new_page = parse_response(page);

    if (new_page != NULL) {

        cout<<"We have a new page"<<endl;

    }

    cache_add(url, new_page);

    cout<<"The page "<<url<<" has been stored in cache"<<endl;

    return new_page;

}



Entity* page_found(string url) {

    // handle pages that are found in the cache

    map<string, Entity*>::iterator ii = ca.find(url);
    Entity* en = ii->second;

    string exp = en->get_header()->get_expires();

    if (exp.empty()) {

        exp = en->get_header()->get_last_modified();

        if (exp.empty()) {

            exp = en->get_header()->get_date();

        }

    }

    int current, ex_int;
    time_t ex = to_Time_T(exp);

    if (ex <= 0) {

        cout<<"The expires field is not a timestamp: "<<exp;

        ex_int = -1;

    } else {

        ex_int = int(ex);

    }

    current = int(get_current_time());

    if (current > ex_int) {

        cout<<"A page's entry in the cache expired"<<endl;

        string pageExpired = get_content_from_web_server_if_modified(url, exp);

        if (pageExpired.empty()) {

            cout<<"No need to get the page "<<url<<" from the server which was not modified on the server"<<endl;

            en = stamp_page(en); //serve from cache itself

        } else {

            Entity* newEntity = parse_response(pageExpired);
            cache_update(url, newEntity);

            cout<<"The page "<<url<<" has been replaced in cache"<<endl;

            return newEntity;

        }
    } else { //if page hasnt expired in the cache

        cout<<"The page "<<url<<" has been found in cache and it is fresh"<<endl;

        en = stamp_page(en);

        return en;

    }

}



void print_cache() {

    Entity* en;
    map<string, Entity*>::iterator it = ca.begin();

    string key, sla;

    cout<<endl;

    cout<<"******Proxy Cache******"<<endl;

    cout<<"Size of the cache: "<<ca.size()<<endl;

    for (; it != ca.end(); it++) {

        key = it->first;
        en = it->second;

        Entity_Header* hdr = en->get_header();
        time_t la = hdr->get_last_accessed();

        sla = from_Time_T(la);

        cout<<"page: "<<key<<", last accessed: "<<sla<<endl;

    }

}
