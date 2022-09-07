#include "Split_chain.h"
#include <thread>

split_map<uint32_t,uint32_t> *sp;

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
        int k = random()%1000000;
        if(op==0)
           int s = sp->insert(k,k);
        else if(op==1)
           int pos = sp->find(k);
        else bool s = sp->erase(k);

    }


}

int main(int argc,char **argv)
{

	memory_pool<uint32_t,uint32_t> *m = new memory_pool<uint32_t,uint32_t> (100);

	sp =  new split_map<uint32_t,uint32_t> (16,4096,m);

	int num_threads = 12;
	std::vector<struct thread_arg> t_args(num_threads);
	std::vector<std::thread> workers(num_threads);

	int num_operations = 10000000;
	int np = num_operations/num_threads;
	int rem = num_operations%num_threads;

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


	sp->map_full();

	delete m;
	delete sp;


}
