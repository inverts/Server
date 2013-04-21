#include <string.h>
#include <set>
#include <map>
#include <string>
#include "ss_data.h"

using namespace std;

int main()
{
  return 0;
}

// Server storage
set<string> ss_name; // list of names relating to spreadsheets
set<ss_data> spreadsheets; // list of spreadsheets on the server
std::set<ss_data>::iterator it; // iterator to find a spreadsheet
map<string,ss_data> clients; // maps a client to a spreadsheet


// Creates a new spreadsheet
void createSS(string name, string password)
{
  //ss_name.insert(name);
  //spreadsheets.insert(ss_data(name,password));
}

// Saves a spreadsheet
void saveSS(ss_data ss)
{

}

// adds client to spreadsheet session
void addClient(string client, ss_data ss)
{
  //map.insert( pair<string,string>(client,ss) );
}

// checks for valid authorization
bool checkPassword(ss_data ss, string password)
{
  //return (password == ss.getPass());
}

// check if spreadsheet exists
bool checkValid(string name)
{
  //it = ss_name.find(name);
  //if( it == ss_name.end() ) return false;
  //else return true;
}



