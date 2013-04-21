#ifndef SS_DATA_H

#define SS_DATA_H

#include <string>
#include <stack>
#include <set>

class ss_data
{
 public:
  ss_data(std::string s, std::string p);
  int getVers();
  std::string getPass();
  int addVers();
  std::pair<std::string,std::string> undo();
  void addClient(std::string c);
  
  std::set<std::string> clients;

 private:
  int versionNum;
  std::stack< std::pair<std::string,std::string> > undoStack;
  std::string password;
  std::string name;
  
};

#endif
