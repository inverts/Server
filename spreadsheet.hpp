/*
* This represents a spreadsheet object and provides functions for loading and saving into a file.
*/

#include <boost/unordered_map.hpp>
#include <stack>
using namespace std;


typedef boost::unordered_map<std::string,std::string> hashmap;

class spreadsheet {

private:
  std::string fileLocation; //We must store the file location for this spreadsheet.
  hashmap cells;
  std::string name;
  std::string password;
  int version;
  std::stack< std::pair<std::string,std::string> > undoStack;
  pair<string,string> lastUpdate;
  bool push;
  int active_clients;

public:
  spreadsheet(string file, string pass);

  //spreadsheet& operator=(const spreadsheet &rhs);

  int get_active_clients();

  void add_active_client();

  void remove_active_client();

  bool check_password(string password) const;

  void load_spreadsheet();

  void save_spreadsheet();

  bool try_update_cell(string cellname, string data);

  string get_cell_data(string cellname);

  string get_name() const;

  string generate_xml();

  void reset_version();

  int get_version();

  void increment_version();

  pair<string,string> undo();

};
