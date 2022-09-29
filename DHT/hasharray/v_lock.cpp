#include "V_ref.h"
#include <thread>
#include <chrono>
#include <random>

typedef vec_ht<int32_t,int32_t,std::hash<int32_t>,std::equal_to<int32_t>,std::allocator<char>,std::mutex,LinearProbeFcn> vht;
vht* v;

struct thread_arg
{
   int tid;
   int numinserts;
   int numfinds;
   int start;
};


void ht_ops(thread_arg *t)
{

	for(int i=0;i<t->numinserts;i++)
	{
	    int32_t r = random()%10000;
	    std::pair<int32_t,int32_t> e(r,10*i);
	    v->insert(e);
	}
	
	for(int i=0;i<t->numfinds;i++)
	{
		int32_t r = random()%10000;
		auto e = v->find((int32_t)(r));
	}
}


int main(int argc,char **argv)
{

	int numthreads = atoi(argv[1]);
	int num_inserts = atoi(argv[2]);
	int num_finds = atoi(argv[3]);

        v = new vht(8192,-1,0.8);

	v->clear();

	std::vector<thread_arg> t_args;
	std::vector<std::thread> workers;

	t_args.resize(numthreads);
	workers.resize(numthreads);

	int numinserts = num_inserts/numthreads;
	int numfinds = num_finds/numthreads;
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
		std::thread t{ht_ops,&t_args[i]};
		workers[i] = std::move(t);
	}

	for(int i=0;i<numthreads;i++)
		workers[i].join();
	auto t2 = std::chrono::high_resolution_clock::now();
	double timetaken = std::chrono::duration<double>(t2-t1).count();

	std::cout <<" numinserts = "<<numinserts*numthreads<<" numfinds = "<<numfinds*numthreads<<std::endl;
	std::cout <<" timetaken = "<<timetaken<<std::endl;

	delete v;
}

