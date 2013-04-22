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
  //std::string cells[100][100]; //Allows for a 100x100 grid of cells.
  hashmap cells;
  std::string name;
  std::string password;
  std::string version;

public:
  spreadsheet(string file);

  bool check_password(string password) const;

  void load_spreadsheet();

  void save_spreadsheet();

  bool try_update_cell(string cellname, string data);

  string get_cell_data(string cellname);

  string generate_xml();

};
