#include "Block_chain_memory.h"
#include <thread>
#include <chrono>
#include <random>
//#include "city.h"
/*
struct HashTraits
{
  uint32_t operator()(std::string &a)
  {
    return CityHash32(a.c_str(),a.length());
  }
};


struct EqualTraits
{
   bool operator()(std::string &a, std::string &b)
   {
        return a==b;
   }

};*/

class entity
{
private:
    int a;
    int b;
    int c;
    int *d;
public:    
   entity()
   {
	a=0;b=0;c=0;d=nullptr;
   }
   entity(int n) : a(n)
   {

   }
   entity(entity &e)
   {
	a = e.a;
	b = e.b;
	c = e.c;
	d = e.d;
   }
   entity& operator=(entity& e)
   {
	a = e.a;
	b = e.b;
	c = e.c;
	d = e.d;
	return *this;
   }
};



BlockMap<int32_t,entity> *m;

//BlockMap<std::string,uint32_t,HashTraits,EqualTraits> *m;

struct thread_arg
{
   int tid;
   int num_operations;
};

void map_operations(thread_arg *t)
{
    for(int i=0;i<t->num_operations;i++)
    {
	int op = random()%4;
	int k = random()%100000000;
	if(op==0)
	{
	   entity e(k);
	   int s = m->insert(k,e);
	}
	else if(op==1)
	   int pos = m->find(k);
	else if(op==2) bool s = m->erase(k);
	else if(op==3)
	{
	    entity e;
	    bool s = m->get(k,&e);
	}

    }	    

}

int main(int argc,char **argv)
{

    memory_pool<int32_t,entity> *p = new memory_pool<int32_t,entity> (100);    
    m = new BlockMap<int32_t,entity> (65536,p,INT_MAX);

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

   // m->print_block_entries(print_value);

    std::cout <<" time taken = "<<time_taken<<std::endl;
    std::cout <<" allocated = "<<m->allocated_nodes()<<" removed = "<<m->removed_nodes()<<std::endl;
    std::cout <<" table entries = "<<m->count_block_entries()<<" diff = "<<m->allocated_nodes()-m->removed_nodes()<<std::endl;
    delete m;
    delete p;

}
