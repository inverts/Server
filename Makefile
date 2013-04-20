make: server.cpp
	g++ server.cpp -lboost_system -lpthread -o server

example: echoserver.cpp
	g++ echoserver.cpp -lboost_system -lpthread

multicast: multicast.cpp
	g++ multicast.cpp -lboost_system -lpthread

daytimeserver: daytimeserver.cpp
	g++ daytimeserver.cpp -lboost_system -lpthread -o daytime

highscore: highscore.cpp
	g++ highscore.cpp -lboost_system -lpthread -o highscore