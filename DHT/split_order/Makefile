CPP_FLAGS=-I/home/aparna/Documents/filesystems/boost_1_78_0/
LDFLAGS=-L/home/aparna/Documents/filesystems/boost_1_78_0/lib -lboost_system -lboost_container -lboost_thread -lboost_atomic -lpthread -lrt

SOURCES= split_mem.cpp 

all:
	g++ -c $(CPP_FLAGS) $(SOURCES)
	g++ -o split_mem split_mem.o $(LDFLAGS)

clean:
	rm *.o
	rm split_mem
