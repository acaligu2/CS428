#
# Specifiy the target
make:	proxy

# Specify the object files that the target depends on
# Also specify the object files needed to create the executable
proxy:		httpMain.o Cache.o
	g++ httpMain.o Cache.o -lpthread -o proxy

httpMain.o:	httpMain.cpp
	g++ -c httpMain.cpp

Cache.o:	Cache.h Cache.cpp
	g++ -c Cache.cpp

# Specify the object files and executables that are generated
# and need to be removed to re-compile the whole thing
clean:
	rm -f *.o proxy
