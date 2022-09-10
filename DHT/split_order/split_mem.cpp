#include "Split_chain.h"
#include <thread>

split_map<int32_t,int32_t> *sp;

struct thread_arg
{
   int tid;
   int num_operations;
};

void map_operations(struct thread_arg *t)
{

   for(int i=0;i<t->num_operations;i++)
    {
        int op = random()%3;
        int32_t k = (int32_t) random()%100000000;
        if(op==0)
           int s = sp->insert(k,k);
        else if(op==1)
           int pos = sp->find(k);
        else if(op==2)
	{
	    bool s = sp->erase(k);
	}

    }


}

int main(int argc,char **argv)
{

	memory_pool<int32_t,int32_t> *m = new memory_pool<int32_t,int32_t> (100);

	sp =  new split_map<int32_t,int32_t> (1024,4096,m);

	int num_threads = 12;
	std::vector<struct thread_arg> t_args(num_threads);
	std::vector<std::thread> workers(num_threads);

	int num_operations = 100000000;
	int np = num_operations/num_threads;
	int rem = num_operations%num_threads;

	auto t1 = std::chrono::high_resolution_clock::now();

	for(int i=0;i<num_threads;i++)
	{
	   t_args[i].tid = i;
	   if(i < rem)
	   t_args[i].num_operations = np+1;
	   else t_args[i].num_operations = np;
	   std::thread t{map_operations,&t_args[i]};
	   workers[i] = std::move(t);
	}

	for(int i=0;i<num_threads;i++)
		workers[i].join();

	auto t2 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double> (t2-t1).count();

	std::cout <<" Time taken = "<<t<<std::endl;
	std::cout <<" allocated = "<<sp->allocated_entries()<<" removed = "<<sp->removed_entries()<<std::endl;

	delete m;
	delete sp;


}
