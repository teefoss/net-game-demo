all:
	clang++ -std=c++14 *.cc unix/*.cc -o game -lSDL3
