CPP_FLAGS=-I/home/aparna/Documents/filesystems/boost_1_78_0/ -I/home/aparnasasidharan/Documents/include -I/home/aparna/Documents/filesystems/argobots-main/include
LDFLAGS=-L/home/aparnasasidharan/Documents/lib -lmpi -L/home/aparna/Documents/filesystems/boost_1_78_0/lib -lboost_system -lboost_container -lboost_thread -lboost_atomic -L/home/aparna/Documents/filesystems/argobots-main/lib -labt -lpthread -lrt

SOURCES= block_mem.cpp block_mem_abt.cpp block_mem_dist.cpp

all:
	mpic++ -c $(CPP_FLAGS) $(SOURCES)
	mpic++ -o block_mem block_mem.o $(LDFLAGS)
	mpic++ -o block_mem_abt block_mem_abt.o $(LDFLAGS)
	mpic++ -o block_mem_dist block_mem_dist.o $(LDFLAGS)

clean:
	rm *.o
	rm block_mem
	rm block_mem_abt
	rm block_mem_dist
