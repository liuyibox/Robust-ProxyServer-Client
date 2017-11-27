#include<string.h>
#include<iostream>
#include<sstream>
#include<map>
#include<stdio.h>
#include<stdlib.h>
#include<vector>
#include<time.h>
#include"HttpProxy.h"



using namespace std;



string date("Date");

string lastMod("Last-Modified");

string expires("Expires");



char* format = strdup("%a, %d %b %Y %H:%M:%S %Z");


Entity_Header::Entity_Header() {

}


Header::Header() {

}


void Entity_Header::set_content_encoding(string con_encoding) {

    content_encoding = con_encoding;

}


void Entity_Header::set_content_type(string con_type) {

    content_type = con_type;

}


void Entity_Header::set_last_modified(string lm) {

    last_modified = lm;

}


void Entity_Header::set_last_accessed(time_t la) {

    last_accessed = la;

}


void Entity_Header::set_expires(string exp) {

    expires = exp;

}

void Entity_Header::set_date(string d) {

    date = d;

}


string Entity_Header::get_content_encoding() {

    return content_encoding;

}


string Entity_Header::get_content_type() {

    return content_type;

}


string Entity_Header::get_last_modified() {

    return last_modified;

}


time_t Entity_Header::get_last_accessed() {

    return last_accessed;

}


string Entity_Header::get_expires() {

    return expires;

}


string Entity_Header::get_date() {

    return date;

}


string Entity_Header::to_string() {

    string space(" ");
    string retval = content_type + space + content_encoding;

    return retval.c_str();

}


Entity::Entity() {

}


Entity_Header* Entity::get_header() {

    return header;

}


string Entity::get_body() {

    return body;

}


void Entity::set_header(Entity_Header *h) {

    header = h;

}


void Entity::set_body(string b) {

    body = b;

}


string Entity::to_string() {

    string new_line("\n");
    string retval = header->to_string() + new_line + body;

    return retval;

}



bool is_get_req(char *t) {

    string get_St("GET");
    string st(t);

    if (get_St == st) {

        return 1;

    } else {

        return 0;

    }

}



string extract_uri(char *http_request) {

    char* token = (char*)malloc(sizeof(char*));
    token = strtok(http_request, " ");

    int i = 0;
    char* tokens[3];

    while (token != NULL) {

        tokens[i] = token;
        token = strtok(NULL, " ");

        i++;

    }

    string req_uri(tokens[1]);

    return req_uri;

}



Entity* parse_response(string res) {

    Entity_Header* hdr = new Entity_Header;
    Entity* entity = new Entity;


    cout<<res;
    cout<<endl<<endl;

    string s_response = res;
    string delim("\r\n\r\n");

    vector<string> top_level = string_split(res, delim);

    if (top_level.size() == 1 || top_level.size() > 2) {

        cout<<"Response should have only header and body"<<endl;
        cout<<"The no of tokens \""<<s_response<<"\" is "<<top_level.size()<<endl;

        exit(1);

    }

    string http_Hdr = top_level.at(0);
    delim = "\r\n";
    vector<string> response_Hdr_fields = string_split(http_Hdr, delim);
    if (response_Hdr_fields.size() == 0) {

        cout<<"No tokens found in the http header \""<<http_Hdr<<"\""<<endl;
        exit(1);

    }

    // iterate the header to find fields like Expires, Last-modified and Date
    int num_fields = response_Hdr_fields.size();

    vector<string> field_tokens;
    delim = ": ";

    for (int i = 1; i < num_fields; i++) {

        //first parse each line using ':'
        field_tokens = string_split(response_Hdr_fields.at(i), delim);

        if (field_tokens.at(0) == expires) {

            hdr->set_expires(field_tokens.at(1));

        } else if(field_tokens.at(0) == lastMod) {

            hdr->set_last_modified(field_tokens.at(1));

        } else if(field_tokens.at(0) == date) {

            hdr->set_date(field_tokens.at(1));

        }

    }

    time_t curr = get_current_time();
    hdr->set_last_accessed(curr);

    string httpBody = top_level.at(1);

    entity->set_body(s_response);
    entity->set_header(hdr);

    return entity;

}



double timediff(string time2, string time1) {

    // return the difference between time 2 and time 1

    time_t t1 = to_Time_T(time1);
    time_t t2 = to_Time_T(time2);

    return difftime(t2, t1);

}



vector<string> string_split(string s, string delim) {

    vector<string> tokens;

    int len = s.length();
    int start = 0;

    int found;
    size_t f;

    string token;

    while (start < len) {

        f = s.find(delim, start);

        if (f == string::npos) {

            tokens.push_back(s.substr(start));

            break;

        }

        found = int(f);
        token = s.substr(start, found - start);

        tokens.push_back(token);

        start = found + delim.length();

    }

    return tokens;

}



string from_Time_T(time_t t) {

    struct tm* lt = gmtime(&t);
    char s_time[30];

    strftime(s_time, 30, format, lt);
    string s(s_time, 30);

    return s;

}



time_t to_Time_T(string t) {

    // convert a string to hold local time

    if (t.empty()) {

        cout<<"The time string is null"<<endl;

        return -1;

    }

    struct tm tm1;
    char* ret = strptime(t.c_str(), format, &tm1);

    if (ret == NULL) {

        cout<<"The string "<<t<<" was not in a date format"<<endl;

        return -1;

    }

    time_t t1 = timegm(&tm1);

    return t1;

}



Entity* stamp_page(Entity *en) {

    // stamp with the current timestamp

    Entity_Header* hdr = en->get_header();

    hdr->set_last_accessed(get_current_time());
    en->set_header(hdr);

    return en;

}



time_t get_current_time() {

    struct tm* tm1;

    time_t local = time(NULL);
    tm1 = gmtime(&local);
    time_t local_utc = timegm(tm1);

    return local_utc;

}
