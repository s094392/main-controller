all:
	clang++ -std=c++17 main.cpp -lhiredis -o controller -O2
docker :
	g++ -std=c++14 main.cpp -lhiredis -o controller -O2
run: all
	./a.out
