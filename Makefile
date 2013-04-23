sockettool: sockettool.cpp
	g++ sockettool.cpp -lboost_system -lpthread -o sockettool

simpleserver: simpleserver.cpp spreadsheet.cpp
	g++ simpleserver.cpp spreadsheet.cpp -lboost_system -lpthread -o simpleserver

spreadsheet: spreadsheet.cpp
	g++ spreadsheet.cpp -lboost_system -lpthread -o spreadsheet

server: spreadsheetserver.cpp spreadsheet.cpp spreadsheet.hpp
	g++ spreadsheetserver.cpp spreadsheet.cpp -lboost_system -lpthread -o server

clean:
	rm -f *~
	rm -f *#