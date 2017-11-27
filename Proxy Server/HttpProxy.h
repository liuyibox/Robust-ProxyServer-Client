#include<vector>
#include<string>
#include<time.h>



using namespace std;



#define MAX_CACHE_SIZE 10

#define STDIN 0



class Header {
public:
    Header();
};



class Entity_Header : public Header {

    string content_type;

    string content_encoding;

    string expires;

    string last_modified;

    string date;

    time_t last_accessed;

public:
    Entity_Header();
    void set_content_type(string);
    void set_content_encoding(string);

    void set_expires(string);
    void set_last_modified(string);
    void set_date(string);
    void set_last_accessed(time_t);

    string get_content_type();
    string get_content_encoding();

    string get_expires();
    string get_last_modified();
    string get_date();
    time_t get_last_accessed();

    string to_string();

};



class Entity {

    Entity_Header* header;
    string body;

public:
    Entity();

    void set_header(Entity_Header *);
    void set_body(string);

    Entity_Header* get_header();

    string get_body();
    string to_string();

};



bool is_get_req(char *);

string extract_uri(char *);

Entity* parse_response(string);

double timediff(string, string);

vector<string> string_split(string, string);

time_t to_Time_T(string);

string from_Time_T(time_t);

Entity* stamp_page(Entity *);

time_t get_current_time();
