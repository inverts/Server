sockettool: sockettool.cpp
	g++ sockettool.cpp -lboost_system -lpthread -o sockettool

simpleserver: simpleserver.cpp
	g++ simpleserver.cpp -lboost_system -lpthread -o simpleserver

server: spreadsheetserver.cpp spreadsheet.cpp spreadsheet.hpp
	g++ spreadsheetserver.cpp spreadsheet.cpp -lboost_system -lpthread -o server