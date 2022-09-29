//#include "HashMap_chain_lockfree.h"
#include "Block_chain_lockfree_memory.h"
#include <thread>
#include "city.h"

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

};

BlockMap_lockfree<int32_t,std::string> *m;
BlockMap_lockfree<std::string,int32_t,HashTraits,EqualTraits> *b;

struct thread_arg
{
   int tid;
};

void block_map(struct thread_arg *t)
{


  for(int i=0;i<10;i++)
  {
	bool s = b->insert("abcdef",43);
	if(s) std::cout <<" inserted"<<std::endl;
	//bool s = m->insert(i,"abcdefghijk");
  }


}

int main(int argc,char **argv)
{

  //HashMap_lockfree<int32_t,int32_t> *m = new HashMap_lockfree<int32_t,int32_t> (16,512);

    m = new BlockMap_lockfree<int32_t,std::string> (512);

    b = new BlockMap_lockfree<std::string,int32_t,HashTraits,EqualTraits> (512);

    int num_threads = 1;
    
    std::vector<struct thread_arg> t_args(num_threads);
    std::vector<std::thread> workers(num_threads);

    for(int i=0;i<num_threads;i++)
    {
	    std::thread t{block_map,&t_args[i]};
	    workers[i] = std::move(t);
    }

    for(int i=0;i<num_threads;i++)
	    workers[i].join();


    delete b;
    delete m;
}
