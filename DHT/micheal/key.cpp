#include <iostream>
#include <vector>
#include <string.h>
#include <cstring>
#include <chrono>
#include <algorithm>

class req
{
public :
   int a;
   int b;

   req(int i,int j) : a(i), b(j)
   {

   }	
};

bool compare_func(uint64_t &a,uint64_t &b)
{
	if(b >= a) return true;
	else return false;
}

int main(int argc,char **argv)
{
/*
	req *r = new req(0,0);

	std::cout<<" r = "<<r->a<<" r = "<<r->b<<std::endl;	
	new (r) req(1,1);

	std::cout <<" r = "<<r->a<<" r = "<<r->b<<std::endl;

	delete r;*/

	std::vector<uint64_t> list;

	for(int i=0;i<100;i++)
		list.push_back(i);

	uint32_t length = list.size();
	uint32_t q = 23;
	uint32_t n = length/q;
	uint32_t r = length%q;
	uint32_t offset = r*(n+1);
	
	uint32_t s = 20 ;
	uint32_t p = UINT32_MAX;

	if(s >= 0 && s < length)
	{
	  if(s < offset) 
		p = s/(n+1);
	  else
		p = r+(s-offset)/n;
	}

	if(p != -1)
	{
	  uint32_t min_range, max_range;
	  
	  if(p < r) 
	  {
		  min_range = p*(n+1); max_range = min_range+(n+1);
	  }
	  else 
	  {
		  min_range = r*(n+1)+(p-r)*n;
		  max_range = min_range+n;
	  }
	  std::cout <<" min_range = "<<min_range<<" max_range = "<<max_range<<std::endl;

	}
	std::cout <<" s = "<<s<<" n = "<<n<<" r = "<<r<<" offset = "<<offset<<" p = "<<p<<std::endl;

}

