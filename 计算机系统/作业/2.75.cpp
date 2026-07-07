#include<iostream>
#include<assert.h>
#include<inttypes.h>

using namespace std;

int signed_high_prod(int x, int y)
{
    int64_t mul = (int64_t) x * y;
    return mul >> 32;
}

unsigned unsigned_high_prod(unsigned x, unsigned y)
{
   int x_max = x >> 31;
   int y_max = y >> 31;
   int mul = signed_high_prod((int)x,(int )y) + x_max * y + y_max * x;
   return mul; 
}

unsigned another_high_prod(unsigned x,unsigned y)
{
    uint64_t mul = (uint64_t)x * y;
    return mul >> 32;
}

int main()
{
    int x = 0x3f3f3f3f;
    int y = 0xffffffff;
    if(unsigned_high_prod((unsigned)x,(unsigned)y) == another_high_prod((unsigned)x,(unsigned)y))
    {
        printf("right\n");
    }
    else cout<<"no"<<endl;
}
