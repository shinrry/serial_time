serial: serial.o TsipParser.o
	g++ -g serial.o TsipParser.o -o a.out
serial.o: serial.cpp
	g++ -g -c serial.cpp
TsipParser.o: TsipParser.cpp TsipParser.h
	g++ -g -c TsipParser.cpp
clean:
	rm *.o


