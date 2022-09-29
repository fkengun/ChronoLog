#include "HashMap_chain.h"
#include <thread>

HashMap<int32_t,int32_t> *m;

struct thread_arg
{
   int tid;
};


void map_operations(thread_arg *t)
{

	for(int i=0;i<1000;i++)
	{
		bool s = m->insert(i,i);
		if(!s) 
		{
			std::pair<int,int> p = m->find(i);
		   if(p.second==NOT_IN_TABLE) std::cout <<" t = "<<t->tid<<" i = "<<i<<std::endl;
		}
	}
	
	for(int i=0;i<5;i++)
	{
	   bool s = m->erase(i);
	}

}

int main(int argc,char **argv)
{

    m = new HashMap<int32_t,int32_t> (4,1024);

    int num_threads = 8;

    std::vector<std::thread> workers(num_threads);
    std::vector<thread_arg> t_args(num_threads);

    for(int i=0;i<num_threads;i++)
    {
	t_args[i].tid = i;
	std::thread t{map_operations,&t_args[i]};
	workers[i] = std::move(t);
    }

    for(int i=0;i<num_threads;i++)
	    workers[i].join();

    for(int i=0;i<5;i++)
    {
	std::pair<uint32_t,uint32_t> p = m->find(i);
	if(p.second==NOT_IN_TABLE) std::cout <<" not found : i = "<<i<<std::endl; 

    }
    delete m;

}
