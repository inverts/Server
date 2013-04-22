/*
 * This represents a spreadsheet object and provides functions for loading and saving into a file.
 */

using namespace std;

class spreadsheet {

private:
  int rows;
  int cols;
  std::string fileLocation; //We must store the file location for this spreadsheet.
  std::string cells[100][100]; //Allows for a 100x100 grid of cells.
  std::string name;
  std::string password;

public:
  spreadsheet(string file);

  bool check_password(string password) const;

  void load_spreadsheet();

  void save_spreadsheet();

  bool try_update_cell(int row, int column, string data);

  string get_cell_data(int row, int column);

  string generate_xml();

};
