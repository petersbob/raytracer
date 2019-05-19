raytracer : main.o
	g++ -o raytracer main.o

main.o : main.cpp
	g++ -c main.cpp

clean :
	rm raytracer main.o