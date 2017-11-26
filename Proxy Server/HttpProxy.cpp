#include<string.h>
#include<iostream>
#include<sstream>
#include<map>
#include<stdio.h>
#include"HttpProxy.h"
#include<stdlib.h>
#include<vector>
#include<time.h>

using namespace std;

//global stuff
string expires("Expires");
string lastMod("Last-Modified");
string date("Date");
//string _format_ = string();
//char* format = _format_.c_str();

char* format = strdup("%a, %d %b %Y %H:%M:%S %Z");

Header::Header()
{
}

EntityHeader::EntityHeader()
{
}

void EntityHeader::setContentType(string conType)
{
  contentType = conType;
}

void EntityHeader::setContentEncoding(string conEncoding)
{
  contentEncoding = conEncoding;
}

void EntityHeader::setExpires(string ex)
{
  expires = ex;
}

void EntityHeader::setDate(string d)
{
  date = d;
}

void EntityHeader::setLastModified(string lm)
{
  lastModified = lm;
}

void EntityHeader::setLastAccessed(time_t la)
{
  lastAccessed = la;
}

string EntityHeader::getContentType()
{
  return contentType;
}

string EntityHeader::getContentEncoding()
{
  return contentEncoding;
}

string EntityHeader::getExpires()
{
  return expires;
}

string EntityHeader::getLastModified()
{
  return lastModified;
}

time_t EntityHeader::getLastAccessed()
{
  return lastAccessed;
}

string EntityHeader::getDate()
{
  return date;
}

string EntityHeader::toString()
{
  string space(" ");
  string retval = contentType+space+contentEncoding;
  return retval.c_str();
}


Entity::Entity()
{
}

void Entity::setHeader(EntityHeader* h)
{
  header = h;
}

void Entity::setBody(string b)
{
  body = b;
}

EntityHeader* Entity::getHeader()
{
  return header;
}

string Entity::getBody()
{
  return body;
}

string Entity::toString()
{
  string newline("\n");
  string retval = header->toString()+newline+body;
  return retval;
}

string extractRequestUri(char* httpRequestLine)
{
  char* token = (char*)malloc(sizeof(char*)); 
  token = strtok(httpRequestLine, " ");
  char* tokens[3];
  int i = 0;
  while(token != NULL)
  {
    tokens[i] = token;
    token = strtok(NULL, " ");
    i++;
  }

  //now get the uri
  string reqUri(tokens[1]);
  return reqUri;
}


bool isGetReq(char* t)
{
  string st(t);
  string getSt("GET");

  if(getSt == st)
    return 1;
  else
    return 0;
}


//Entity* parseResponse(string response, int responseLen)
Entity* parseResponse(string response)
{  
  Entity* entity = new Entity;
  EntityHeader* hdr = new EntityHeader;

  //cout<<"Going to parse the following "<<responseLen<<" response from the Web server:"<<endl;
  cout<<response;
  cout<<endl<<endl;

  string delim("\r\n\r\n");
  string s_response = response;
  vector<string> toplevel = strsplit(response, delim); 

  if(toplevel.size() == 1 || toplevel.size() > 2)
  {
    cout<<"Not allowed.. response should have only header and body"<<endl;
    cout<<"The no of tokens in the response \""<<s_response<<"\" is "<<toplevel.size()<<endl;
    exit(1);
  }

  //now take the header and break it up
  string httpHdr = toplevel.at(0);
  delim = "\r\n";
  vector<string> responseHdrFields = strsplit(httpHdr, delim);
  if(responseHdrFields.size() == 0)
  {
    cout<<"No tokens found in the http header \""<<httpHdr<<"\""<<endl;
    exit(1);
  }

  //now iterate thru the header fields and look for 
  //stuff like Expires, Last-modified, Date etc

  int numFields = responseHdrFields.size();
  vector<string> fieldTokens;
  //cout<<"status line: "<<responseHdrFields.at(0)<<endl;
  delim = ": ";
  for(int i=1; i<numFields; i++) //because first line is the status line
  {
    //first parse each line using ':'
    fieldTokens = strsplit(responseHdrFields.at(i), delim);

    if(fieldTokens.at(0) == expires)
      hdr->setExpires(fieldTokens.at(1));
    else if(fieldTokens.at(0) == lastMod)
      hdr->setLastModified(fieldTokens.at(1));
    else if(fieldTokens.at(0) == date)
      hdr->setDate(fieldTokens.at(1));

  }

  //set the last accessed time as the current time
  time_t curr = getCurrentTime(); 
  hdr->setLastAccessed(curr);
  
  //now set the body
  string httpBody = toplevel.at(1);
  entity->setBody(s_response);
  entity->setHeader(hdr);
  //cout<<"the html body is \""<<httpBody<<"\""<<endl;

  return entity;
} 

vector<string> strsplit(string s, string delim)
{
  vector<string> tokens;
  int start = 0;
  int len = s.length();
  size_t f;
  int found;
  string token;
  while(start < len)
  {
    f = s.find(delim, start);
    if(f == string::npos)
    {
      //this could be the last token, or the
      //patter does not exist in this string
      tokens.push_back(s.substr(start));
      break;
    }

    found = int(f);
    token = s.substr(start, found - start);
    tokens.push_back(token);
    
    start = found + delim.length();
  }

  /*cout<<"Printing"<<tokens.size()<<" tokens"<<endl;
  for(int i=0; i<tokens.size(); i++)
  {
    cout<<tokens.at(i)<<endl;
  }*/
  return tokens;
}

/*
 * Return the time diff between time2 and time1 in seconds; time2 >= time1
 */
double timeDiff(string time2, string time1)
{
  time_t t1 = toTimeT(time1);
  time_t t2 = toTimeT(time2);

  return difftime(t2, t1);
}


/*
 * converts a string to a time_t object which holds the local time
 */
time_t toTimeT(string t)
{
  if(t.empty())
  {
    cout<<"The time string is null"<<endl;
    return -1;
  }
 
  struct tm tm1;
  char* ret = strptime(t.c_str(), format, &tm1);
  if(ret == NULL)
  {
    cout<<"The string "<<t<<" was not recognized as a date format"<<endl;
    return -1;
  }
  
  time_t t1 = timegm(&tm1);
  return t1; 
}


/*
 * display a time_t object in UTC
 */
string fromTimeT(time_t t)
{
  struct tm* lt = gmtime(&t);
  char stime[30];

  strftime(stime, 30, format, lt);
  string s(stime, 30);
  return s;
}


time_t getCurrentTime()
{
  struct tm* tm1;
  time_t local = time(NULL);
  tm1 = gmtime(&local);
  time_t localutc = timegm(tm1);
  return localutc;
}


/*
 * stamp the entity with the current timestamp
 */
Entity* stampPage(Entity* en)
{
  EntityHeader* hdr = en->getHeader();
  hdr->setLastAccessed(getCurrentTime());
  en->setHeader(hdr);
  return en;
}
