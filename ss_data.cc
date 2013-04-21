#include <stack>
#include <string.h>
#include <string>
#include "ss_data.h"

using namespace std;

// Constructor
ss_data::ss_data(string s, string p)
{
  name = s;
  password = p;
}

// gets the current version number
int ss_data::getVers()
{
  return versionNum;
}

// increments the version number
int ss_data::addVers()
{
  versionNum++;
  return versionNum;
}

// returns password of this spreadsheet
string ss_data::getPass()
{
  return password;
}

// undo last change to the spreadsheet
pair<string,string> ss_data::undo()
{
  undoStack.top();
  undoStack.pop();

  // returns the previous state of the given (1) cell and (2) string/function
  return undoStack.top();
}

// adds a client to this spreadsheet session
void addClient(string c)
{
  //clients.insert(c);
}

int main()
{
  return 0;
}
