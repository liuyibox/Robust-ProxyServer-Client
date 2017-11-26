#include<string>
#include<vector>
#include<time.h>

using namespace std;

#define MAX_CACHE_SIZE 10
#define STDIN 0

class Header
{
  public:
  Header();
};

class EntityHeader : public Header
{
  string contentType;
  string contentEncoding;
  string expires;
  string lastModified;
  string date;
  time_t lastAccessed;

  public:
  EntityHeader();
  void setContentType(string);
  void setContentEncoding(string);
  void setExpires(string);
  void setLastModified(string);
  void setDate(string);
  void setLastAccessed(time_t);
  string getContentType();
  string getContentEncoding();
  string getExpires();
  string getLastModified();
  string getDate();
  time_t getLastAccessed();
  string toString();
};

class Entity
{
  EntityHeader* header;
  string body;

  public:
  Entity();
  void setHeader(EntityHeader*);
  void setBody(string);
  EntityHeader* getHeader();
  string getBody();
  string toString();
};

//methods
string extractRequestUri(char*);
bool isGetReq(char*);
//Entity* parseResponse(string, int);
Entity* parseResponse(string);
vector<string> strsplit(string, string);
double timeDiff(string, string);
time_t toTimeT(string);
string fromTimeT(time_t);
time_t getCurrentTime();
Entity* stampPage(Entity*);
