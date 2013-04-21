make: server.cpp
	g++ server.cpp -lboost_system -lpthread -o server

daytimeserver: daytimeserver.cpp
	g++ daytimeserver.cpp -lboost_system -lpthread -o daytime

highscore: highscore.cpp
	g++ highscore.cpp -lboost_system -lpthread -o highscore

sockettool: sockettool.cpp
	g++ sockettool.cpp -lboost_system -lpthread -o sockettool

simpleserver: simpleserver.cpp
	g++ simpleserver.cpp -lboost_system -lpthread -o simpleserver