/*
 * This represents a spreadsheet object and provides functions for loading and saving into a file.
 */

#include <boost/unordered_map.hpp>
using namespace std;


typedef boost::unordered_map<std::string,std::string> hashmap;

class spreadsheet {

private:
  int rows;
  int cols;
  std::string fileLocation; //We must store the file location for this spreadsheet.
  hashmap cells;
  std::string name;
  std::string password;
  int version;

public:
  spreadsheet(string file);

  bool check_password(string password) const;

  void load_spreadsheet();

  void save_spreadsheet();

  bool try_update_cell(string cellname, string data);

  string get_cell_data(string cellname);

  string generate_xml();

  void set_version(int v);

  void increment_version();

};
