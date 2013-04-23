/*
 * This represents a spreadsheet object and provides functions for loading and saving into a file.
 */
#include <string> 
#include <fstream> 
#include <iostream> 
#include <boost/unordered_set.hpp>
#include <sstream> 

#ifndef SPREADSHEET_HPP
#define SPREADSHEET_HPP
#include "spreadsheet.hpp"
#endif

using namespace std;


typedef boost::unordered_map<std::string,std::string> hashmap;

/*
 * Spreadsheet constructor.
 */
spreadsheet::spreadsheet(string name) {
  fileLocation = "spreadsheets/" + name;
  this->name = name;
  version = 0;
  std::string pass = password;
  lastUpdate = pair<string,string>("","");
  undoStack.push(pair<string,string>("",""));
  push = true;
}

spreadsheet& spreadsheet::operator=(const spreadsheet &rhs) {
  if (this == &rhs)
    return *this;
  this->name = rhs.name;
  this->fileLocation = rhs.fileLocation;
  this->version = rhs.version;
  this->cells = rhs.cells;
  this->password = rhs.password;
  return *this;
}

/*
 * Checks if a password matchs this spreadsheet's password.
 */
bool spreadsheet::check_password(string pass) const {
  return (password == pass);
}

/*
 * Saves the spreadsheet to the file specific to this object.
 */
void spreadsheet::save_spreadsheet() {
  fstream fs;
  fs.open(fileLocation.c_str(), std::fstream::out); //This will create the file if it doesn't exist.
  fs << generate_xml();

  fs.close();

}

/*
 * Loads the spreadsheet into the file by the following spec:
 * Cells are arranged as you would see them in a spreadsheet, separated by tabs
 * for each column, and by lines for each row.
 */
void spreadsheet::load_spreadsheet() {
  fstream fs;
  fs.open(fileLocation.c_str(), std::fstream::in); //This will create the file if it doesn't exist.
  string temp;
  int tempint;
  string callnametemp;
  while (fs.good()) { //This will not pass if the file couldn't be opened.
    try {
      std::getline(fs, temp, '<');
      std::getline(fs, temp, '>');

      tempint = temp.find("version=\""); //Find version info
      if (tempint == std::string::npos)
	return; //Malformed.
      temp = temp.substr(tempint + 9);
      tempint = temp.find("\""); //Find ending quote
      if (tempint == std::string::npos)
	return; //Malformed.
      version = atoi(temp.substr(0, tempint).c_str());

      //This should be the first cell.
      std::getline(fs, temp, '<');
      std::getline(fs, temp, '>');

      while(temp == "cell") {
	std::getline(fs, temp, '<');
	std::getline(fs, temp, '>');
	if (temp != "name")
	  return; //Malformed.
	std::getline(fs, temp, '<');

	callnametemp = temp; //Cell name.

	std::getline(fs, temp, '>');
	if (temp != "/name")
	  return; //Malformed.
	std::getline(fs, temp, '<');
	std::getline(fs, temp, '>');
	if (temp != "contents")
	  return; //Malformed.
	std::getline(fs, temp, '<');

	cells[callnametemp] = temp; //Fill in cell contents.

	std::getline(fs, temp, '>');
	if (temp != "/contents")
	  return; //Malformed.
	std::getline(fs, temp, '<');
	std::getline(fs, temp, '>');
	if (temp != "/cell")
	  return; //Malformed.

	//Read in the next cell.
	std::getline(fs, temp, '<');
	std::getline(fs, temp, '>');

      }
    } catch (int e) {
      return; //Malformed.
    }
  }
  fs.close();
}  

/*
 * Updates the specified cell. If the cell is out of bounds, returns false;
 */
bool spreadsheet::try_update_cell(string cellname, string data) {
  cells[cellname] = data;
  return true;
}

/*
 * Returns the contents of a cell as a string.
 */
string spreadsheet::get_cell_data(string cellname) {
  if (cells.find(cellname) == cells.end())
    return NULL;
  return cells[cellname];

}

string spreadsheet::get_name() const{
  return this->name;
}

/*
 * Generates an XML string representation of the spreadsheet.
 */
string spreadsheet::generate_xml() {
  hashmap::iterator iter;
  std::stringstream ss;
  ss << "<spreadsheet version=\"" << version << "\">\n";
  for (iter = cells.begin(); iter != cells.end(); iter++) {
   ss << "<cell><name>" << iter->first << "</name><contents>" << iter->second << "</contents></cell>\n";
  }
  return ss.str();
}

/*
 * Sets the version.
 */
void spreadsheet::set_version(int v) {
  this->version = v;
}

/*
 * Increments version number by 1.
 */
void spreadsheet::increment_version() {
  this->version += 1;
}

/*
 * undos the last action to the spreadsheet
 */
pair<string,string> spreadsheet::undo()
{
  pair<string,string> p = undoStack.top();
  // if nothing has changed
  if( undoStack.top().first == "" && undoStack.top().second == "" )
    {
      // do nothing?
    }
  else
    {
      undoStack.pop(); // takes off last changed cell/data 
      push = false; // tells the stack not to push lastUpdate
      try_update_cell(p.first,p.second); // updates last cell to previous state
      
    }

  return p;
}

/*
 * This is just a tester method. It should be removed prior to deployment.
 */

/*
int main() {
  cout << "Please specify filename of spreadsheet." << endl;
  string filename;
  cin >> filename;
  spreadsheet ss(filename);
  cout << "Spreadsheet created." << endl;
  ss.load_spreadsheet();
  cout << "Spreadsheet loaded." << endl;
  ss.try_update_cell("A3", "NEW STRING!");
  cout << "Cell updated." << endl;
  ss.try_update_cell("C45", "Weeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee!");
  cout << "Cell updated." << endl;
  string data = ss.get_cell_data("A3");
  cout << "Cell retrieved: " << data << endl;
  ss.save_spreadsheet();
  cout << "Spreadsheet saved." << endl;
  return 0;
}

*/
