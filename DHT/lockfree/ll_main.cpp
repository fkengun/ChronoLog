#include "ll.h"
#include <thread>

ll *m;

struct thread_arg
{
   int tid;
};

void list_operations(struct thread_arg *t)
{

	for(int i=0;i<1000;i++)
	{
	   bool s = m->insert(i);
	   if(!s) s = m->find(i);
	}

	for(int i=0;i<500;i++)
	{
	   int a = random()%500;
	   bool s = m->erase(a);
	}
}


int main(int argc,char **argv)
{

	m = new ll();

	int num_threads = 12;
	std::vector<struct thread_arg> t_args(num_threads);
	std::vector<std::thread> workers(num_threads);

	for(int i=0;i<num_threads;i++)
	{
	     t_args[i].tid = i;
	     std::thread t{list_operations,&t_args[i]};
	     workers[i] = std::move(t);
	}

	for(int i=0;i<num_threads;i++)
		workers[i].join();


	delete m;

}
