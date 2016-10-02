test:AJson.o test.o
	g++ -std=c++11 -o test test.o AJson.o

AJson.o:AJson.cpp AJson.h
	g++ -std=c++11 -o AJson.o -c AJson.cpp

test.o:test.cpp
	g++ -std=c++11 -o test.o -c test.cpp


