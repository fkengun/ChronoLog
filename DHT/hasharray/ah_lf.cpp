#include "AH_array.h"
#include <vector>
#include <random>
#include <mutex>
#include <thread>
#include <chrono>

typedef AtomicHashArray<int32_t,int32_t,std::hash<int32_t>,std::equal_to<int32_t>,std::allocator<char>,LinearProbeFcn> ah;
ah::SmartPtr ah_array;

struct thread_arg
{
	int tid;
	int numinserts;
	int numfinds;
	int start;
};


void map_ops(thread_arg *t)
{

  for(int i=0;i<t->numinserts;i++)
  {
	  //int32_t r = random()%10000;
	  std::pair<int32_t,int32_t> e(i,10*i);
	  ah_array->insert(e);
  }

  for(int i=0;i<t->numfinds;i++)
  {
     //int32_t k = random()%10000;
     int32_t k = i;
     ah_array->find(k);
  }
}

int main(int argc,char **argv)
{

   int numthreads = atoi(argv[1]);
   int num_inserts = atoi(argv[2]);
   int num_finds = atoi(argv[3]);
   
   ah_array = ah::create(8192);
   
   int numinserts = num_inserts/numthreads;
   int numfinds = num_finds/numthreads;
   std::vector<thread_arg> t_args;
   std::vector<std::thread> workers;

   t_args.resize(numthreads);
   workers.resize(numthreads);

   for(int i=0;i<numthreads;i++)
   {
	   t_args[i].tid = i;
	   t_args[i].numinserts = numinserts;
	   t_args[i].numfinds = numfinds;
	   t_args[i].start = i*numinserts;
   }

   auto t1 = std::chrono::high_resolution_clock::now();

   for(int i=0;i<numthreads;i++)
   {
	   std::thread t{map_ops,&t_args[i]};
	   workers[i] = std::move(t);
   }

   for(int i=0;i<numthreads;i++)
	   workers[i].join();

   auto t2 = std::chrono::high_resolution_clock::now();

   double timetaken = std::chrono::duration<double>(t2-t1).count();

   std::cout <<" numinserts = "<<numinserts*numthreads<<" numfinds = "<<numfinds*numthreads<<std::endl; 
   std::cout <<" timetaken = "<<timetaken<<std::endl;
}
