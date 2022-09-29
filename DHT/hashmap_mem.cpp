#include "HashMap_chain.h"
#include <thread>
#include <chrono>

HashMap<int32_t,int32_t> *m;

struct thread_arg
{
   int tid;
   int num_operations;
};


void map_operations(thread_arg *t)
{
    for(int i=0;i<t->num_operations;i++)
    {
	int op = random()%3;
	int k = random()%1000000;
	if(op==0)
	   bool s = m->insert(k,k);
	else if(op==1)
	   std::pair<int,int> pos = m->find(k);
	else bool s = m->erase(k);

    }	    

}

int main(int argc,char **argv)
{

    memory_pool<int32_t,int32_t> *p = new memory_pool<int32_t,int32_t> (1024);    
    m = new HashMap<int32_t,int32_t> (2,500000,p);

    int num_operations = 1000000;

    int num_threads = 12;

    std::vector<std::thread> workers(num_threads);
    std::vector<thread_arg> t_args(num_threads);

    int ops_per_thread = num_operations/num_threads;
    int rem = num_operations%num_threads;

    auto t1 = std::chrono::high_resolution_clock::now();

    for(int i=0;i<num_threads;i++)
    {
	if(i < rem) t_args[i].num_operations = ops_per_thread+1;
	else t_args[i].num_operations = ops_per_thread;
	t_args[i].tid = i;
	std::thread t{map_operations,&t_args[i]};
	workers[i] = std::move(t);
    }

    for(int i=0;i<num_threads;i++)
	    workers[i].join();

    auto t2 = std::chrono::high_resolution_clock::now();
    double time_taken = std::chrono::duration<double> (t2-t1).count();

    std::cout <<" time taken = "<<time_taken<<std::endl;
    /*std::cout <<" allocated = "<<m->allocated_nodes()<<" removed = "<<m->removed_nodes()<<std::endl;
    std::cout <<" table entries = "<<m->count_block_entries()<<" diff = "<<m->allocated_nodes()-m->removed_nodes()<<std::endl;
    std::cout <<" max list size = "<<m->max_list_size()<<std::endl;*/
    delete m;
    delete p;

}
