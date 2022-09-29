#include <boost/atomic.hpp>
#include <thread>
#include <chrono>
#include <iostream>

int main(int argc,char **argv)
{


	boost::atomic<uint32_t> a;
	boost::atomic<uint64_t> b;
	boost::atomic<boost::int128_type> c;

	a.store(0); b.store(0); c.store(0);

	auto t1 = std::chrono::high_resolution_clock::now();

	for(int i=0;i<100;i++)
	{
	     int t = a.load();
	     a.fetch_add(1);
	}
	auto t2 = std::chrono::high_resolution_clock::now();
	double t = std::chrono::duration<double> (t2-t1).count();

	std::cout <<" time taken a = "<<t/100<<std::endl;

	t1 = std::chrono::high_resolution_clock::now();

	for(int i=0;i<100;i++)
	{
	   int64_t t = b.load();
	   b.fetch_add(1);
	}

	t2 = std::chrono::high_resolution_clock::now();

	t = std::chrono::duration<double> (t2-t1).count();

	std::cout <<" time taken b = "<<t/100<<std::endl;

	t1 = std::chrono::high_resolution_clock::now();

	for(int i=0;i<100;i++)
	{
	   boost::int128_type t = c.load();
	   c.fetch_add(1);
	}

	t2 = std::chrono::high_resolution_clock::now();

	t = std::chrono::duration<double> (t2-t1).count();

	std::cout <<" time taken c = "<<t/100<<std::endl;


}
