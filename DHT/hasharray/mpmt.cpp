#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/containers_fwd.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include "libs/interprocess/test/allocator_v1.hpp"
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include "libs/interprocess/test/dummy_test_allocator.hpp"
#include "libs/interprocess/test/movable_int.hpp"
#include "libs/interprocess/test/boost_interprocess_check.hpp"
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/lockfree/queue.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/deque.hpp>
#include <string.h>
#include <random>

#include <boost/interprocess/detail/config_begin.hpp>

namespace boost {
namespace interprocess {

//using boost::container::deque;
using boost::lockfree::queue;
using boost::lockfree::detail::freelist_stack;

}  //namespace interprocess {
}  //namespace boost {

//static boost::interprocess::managed_shared_memory *shmem_m;

/*
void* operator new[](std::size_t count)
{
	return shmem_m->allocate(count);
}

void *allocate(std::size_t count)
{
	return shmem_m->allocate(count);
}*/
#include <boost/interprocess/detail/config_end.hpp>

/*void *operator new[](std::size_t count)
{
    boost::interprocess::managed_shared_memory shmem_m(boost::interprocess::open_copy_on_write,shm_name.c_str());
    return shmem_m.allocate(count);
}*/

struct vtype
{
   int value;
};

using namespace boost::interprocess;

typedef allocator<int,managed_shared_memory::segment_manager> allocator_t;
typedef std::vector<int,allocator<int,managed_shared_memory::segment_manager>> shmem_vec;

struct thread_arg
{
        int tid;
        int pid;
	int id;
	int num_elements;
	shmem_vec *v;
	std::mutex *m;
};

std::vector<std::mutex> mx;
std::mutex mxt;

void func(thread_arg *t)
{

	for(int i=0;i<10;i++)
	{
	   mx[i].lock();
	   (*(t->v))[i] = t->id*i;
	   mx[i].unlock();
	}
}



int main(int argc,char **argv)
{
   int numprocs = 2;
   int numthreads = 1;
   int my_pid = getpid();
   int pid=0;

   typedef basic_managed_shared_memory
      <char,
      rbtree_best_fit<mutex_family>,
      iset_index
      > my_managed_shared_memory;

   std::string shm_name = "shm_file";  // dev/shm/
   shared_memory_object::remove(shm_name.c_str());
   typedef allocator<int,managed_shared_memory::segment_manager> allocator_t;
   managed_shared_memory shmem(open_or_create,shm_name.c_str(),65536);
   boost::container::vector<int> vec;

   /*allocator_t shmem_allocator(shmem.get_segment_manager());

   auto svec = shmem.construct<int*> ("shmem_vec") (shmem_allocator);*/

   shared_memory_object mapping(open_only,shm_name.c_str(),read_write);

   mapped_region rg(mapping,read_write,0,65536,0);

   int*add1 = (int*)rg.get_address();

   for(int i=0;i<100;i++)
	   add1[i] = 100;

   rg.flush(0,65536,0);

   shared_memory_object mapping2(open_only,shm_name.c_str(),read_only);
   mapped_region rg2(mapping2,read_only,0,65536,0);

   int *add2 = (int*)rg2.get_address();

   std::cout <<" add2 = "<<add2<<std::endl;


   /*std::string mname = "shm_mutex";
   shared_memory_object::remove(mname.c_str());
   named_mutex mt(open_or_create,mname.c_str());*/
   /*interprocess_sharable_mutex mt;*/
   //typedef scoped_lock<interprocess_sharable_mutex> lock_type;
   //typedef scoped_lock<named_mutex> lock_type;
   

   //typedef boost::interprocess::vector<int,allocator<int,managed_shared_memory::segment_manager>> shmem_vec;
   //typedef boost::lockfree::queue<int,allocator_t> shmem_queue;

  // typedef allocator<int, my_managed_shared_memory::segment_manager> q_allocator_t;
  // typedef boost::lockfree::queue<int,q_allocator_t> shmem_queue;

   /*int num_elements = 100;
   int *array = NULL;

   array = (int *)shmem.allocate(num_elements*sizeof(int));

   for(int i=0;i<num_elements;i++)
	   array[i] = 100;*/
   
   /*allocator_t shmem_allocator(shmem.get_segment_manager());

   shmem_vec *v = shmem.construct<shmem_vec> ("shmem_vec") (shmem_allocator);

   auto m1 = shmem.find<shmem_vec>("shmem_vec").first;

   m1->reserve(1000);

   for(int i=0;i<10;i++)
	   m1->push_back(10);

   int id = 0;
   for(int i=0;i<numprocs;i++)
   {
	if(getpid()==my_pid)
	{
           id = i;
	   pid = fork();
	}
	
   }

   if(pid==0)
   {
	 std::string name = "shm_file";
	 managed_shared_memory shm(open_only,shm_name.c_str());
	 auto m2 = shm.find<shmem_vec>("shmem_vec").first;
         std::cout <<" id = "<<id<<std::endl;	
	 std::string mname_l = "shm_mutex";	
	 named_mutex nt(open_only,mname_l.c_str()); 
	  
	 for(int i=0;i<10;i++)
	 {
             scoped_lock<named_mutex> lock(nt,defer_lock);
     	     {	     
	        m2->push_back(id);
	     }
	 }
	 std::cout <<" m2 size = "<<m2->size()<<std::endl;*/

   	/*std::vector<thread_arg> t_args;
   	std::vector<std::thread> workers;

   	t_args.resize(numthreads); workers.resize(numthreads);

	std::vector<std::mutex> tmp(10);

	std::swap(mx,tmp);

   	for(int i=0;i<numthreads;i++)
   	{
	   t_args[i].tid = i;
	   t_args[i].pid = getpid();
	   t_args[i].id = id;
	   t_args[i].num_elements = 10;
	   t_args[i].v = m2;
   	}

   	for(int i=0;i<numthreads;i++)
   	{
	   std::thread t{func,&t_args[i]};
	   workers[i] = std::move(t);
   	}

   	for(int i=0;i<numthreads;i++)
	{
	   workers[i].join();
   	}*/
  /* }

   if(my_pid == getpid())
   {
	int wstatus;
        pid_t w;

         do
         {
            w = waitpid(pid,&wstatus,__WALL);
         }while(!WIFEXITED(wstatus));*/

	// std::cout <<" v = "<<(*m1)[0]<<std::endl;
	//shmem.flush();
   //}

   //shmem.deallocate(array);
   //shmem.destroy<shmem_vec> ("shmem_vec");
   /*
   t_args.clear();
   workers.clear();*/
}

