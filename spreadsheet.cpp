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
spreadsheet::spreadsheet(string name, string pass) {
  fileLocation = "spreadsheets/" + name + ".ss";
  this->name = name;
  version = 0;
  active_clients = 0;
  password = pass;
  lastUpdate = pair<string,string>("","");
  undoStack.push(pair<string,string>("",""));
  push = true;
  load_spreadsheet(); //If the spreadsheet already exists, this will load in the data and the password.
}


int spreadsheet::get_active_clients() {
  return active_clients;
}


void spreadsheet::add_active_client() {
  active_clients++;
}


void spreadsheet::remove_active_client() {
  active_clients--;
}
//Probably not necessary.
/*spreadsheet& spreadsheet::operator=(const spreadsheet &rhs) {
  if (this == &rhs)
    return *this;
  this->name = rhs.name;
  this->fileLocation = rhs.fileLocation;
  this->version = rhs.version;
  this->cells = rhs.cells;
  this->password = rhs.password;
  return *this;
  }*/

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

      tempint = temp.find("password=\""); //Find version info
      if (tempint == std::string::npos)
	return; //Malformed.
      temp = temp.substr(tempint + 10);
      tempint = temp.find("\""); //Find ending quote
      if (tempint == std::string::npos)
	return; //Malformed.
      //version = atoi(temp.substr(0, tempint).c_str());

      this->password = temp.substr(0, tempint);

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
 * Updates the specified cell.
 * Boolean return value is in case we have some sort of cell validation in the future.
 */
bool spreadsheet::try_update_cell(string cellname, string data) {
  string tmp = cells[cellname]; // stores data
  cells[cellname] = data;

  if( lastUpdate.first != "" || lastUpdate.second != "" )
    {
      if( push ) // stores newest operation
	undoStack.push( pair<string,string>(cellname,tmp) );
      push = true;
    }

  // lastUpdate now holds the previous operation
  lastUpdate = pair<string,string>(cellname,data);

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
  ss << "<spreadsheet password=\"" << password << "\">";
  for (iter = cells.begin(); iter != cells.end(); iter++) {
    cout <<"Test Second: " << iter->second << endl;
   ss << "<cell><name>" << iter->first << "</name><contents>" << iter->second << "</contents></cell>";
  }
  ss << "</spreadsheet>";
  return ss.str();
}


/*
* Generates an XML string representation of the spreadsheet.
*/
string spreadsheet::generate_xml_to_spec() {
  hashmap::iterator iter;
  std::stringstream ss;
  ss << "<?xml version=\"1.0\" encoding=\"utf-8\"?><spreadsheet>";
  for (iter = cells.begin(); iter != cells.end(); iter++) {
    cout <<"Test Second: " << iter->second << endl;
   ss << "<cell><name>" << iter->first << "</name><contents>" << iter->second << "</contents></cell>";
  }
  ss << "</spreadsheet>";
  return ss.str();
}

/*
* Sets the version.
*/
void spreadsheet::reset_version() {
  this->version = 0;
}

/*
* Gets the version.
*/
int spreadsheet::get_version() {
  return this->version;
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
      return p;
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
  //printfiles();
cout << "Please specify filename of spreadsheet." << endl;
string filename;
cin >> filename;
 spreadsheet ss(filename,"fake");
cout << "Spreadsheet created." << endl;
ss.load_spreadsheet();
cout << "Spreadsheet loaded." << endl;
 ss.try_update_cell("A3", "First");
cout << "Cell updated." << endl;
 ss.try_update_cell("A3", "Second");
cout << "Cell updated." << endl;
 ss.try_update_cell("A3", "Third");
cout << "Cell updated." << endl;
ss.try_update_cell("A3", "Fourth");
cout << "Cell updated." << endl;
 ss.undo();
 ss.undo();
string data = ss.get_cell_data("A3");
cout << "Cell retrieved: " << data << endl;
ss.save_spreadsheet();
cout << "Spreadsheet saved." << endl;
return 0;
}
*/

