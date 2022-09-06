CPP_FLAGS=-I/home/aparna/Documents/filesystems/boost_1_78_0/
LDFLAGS=-L/home/aparna/Documents/filesystems/boost_1_78_0/lib -lboost_system -lboost_container -lboost_thread -lboost_atomic -lpthread -lrt

SOURCES= block_mem_ml.cpp

all:
	g++ -c $(CPP_FLAGS) $(SOURCES)
	g++ -o block_mem_ml block_mem_ml.o $(LDFLAGS)

clean:
	rm *.o
	rm block_mem_ml
