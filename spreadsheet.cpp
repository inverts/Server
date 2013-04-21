/*
 * This represents a spreadsheet object and provides functions for loading and saving into a file.
 */
#include <string> 
#include <fstream> 

#ifndef SPREADSHEET_HPP
#define SPREADSHEET_HPP
#include "spreadsheet.hpp"
#endif

using namespace std;

/*
 * Spreadsheet constructor.
 */
spreadsheet::spreadsheet(string file) {
  //Default rows/cols.
  rows = 100;
  cols = 100;
  fileLocation = file;
}


/*
 * Checks if a password matchs this spreadsheet's password.
 */
bool spreadsheet::check_password(string pass) const {
  return (password == pass);
}
  

/*
 * Loads the spreadsheet into the file by the following spec:
 * Cells are arranged as you would see them in a spreadsheet, separated by tabs
 * for each column, and by lines for each row.
 */
void spreadsheet::load_spreadsheet() {
  fstream fs;
  fs.open(fileLocation.c_str(), std::fstream::in); //This will create the file if it doesn't exist.
  int r, c;
  for (r=0; r<rows; r++) {
    for (c=0; c<cols; c++) {
      fs << cells[c][r] << "\t";
    }
    fs << "\n";
  }
  fs.close();

}


/*
 * Saves the spreadsheet to the file specific to this object.
 */
void spreadsheet::save_spreadsheet() {
  fstream fs;
  fs.open(fileLocation.c_str(), std::fstream::out); //This will create the file if it doesn't exist.
  string currentcell;
  int r=0, c=0;
  while (fs.good()) { //This will not pass if the file couldn't be opened.
    std::getline(fs, currentcell, '\t');
    if (currentcell == "\n") {
      r++;
      continue;
    }
    cells[c++][r] = currentcell;
      
  }
  fs.close();
}

/*
 * Updates the specified cell. If the cell is out of bounds, returns false;
 */
bool spreadsheet::try_update_cell(int row, int column, string data) {
  if (row >= rows || column >=cols || row < 0 || column < 0)
    return false;
  cells[column][row] = data;
  return true;
}
