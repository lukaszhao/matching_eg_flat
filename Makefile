all:
	g++ -I. -g -c -Wall *.cpp
	g++ -g -o matching_engine.exe *.o


all2:
	g++ -I. *.cpp -o matching_engine.exe

clean:
	rm *.o *.exe

run:
	./matching_engine.exe
