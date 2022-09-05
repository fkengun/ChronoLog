#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <vector>

int main(int argc,char **argv)
{

    boost::lockfree::queue<int> *q = new boost::lockfree::queue<int> (1024);


    for(int i=0;i<5000;i++)
    {
	q->push(i);
    
    }

    int n = 0;

    int e = 0;
    while(q->pop(e))
    n++;

    std::cout <<" n = "<<n<<std::endl;

   delete q;

}

