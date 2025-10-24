all : nbt_parser.exe

deflate.o : deflate.cpp deflate.h bit_parser.h
	g++ -c $< -o $@

bit_parser.o : bit_parser.cpp bit_parser.h
	g++ -c $< -o $@

nbt.o : nbt.cpp nbt.h
	g++ -c $< -o $@

nbt_parser.o : nbt_parser.cpp deflate.h nbt.h nbt_parser.h
	g++ -c $< -o $@

main.o : main.cpp nbt_parser.h
	g++ -c $< -o $@

nbt_parser.exe : deflate.o bit_parser.o nbt.o nbt_parser.o main.o
	g++ $^ -static -o $@
