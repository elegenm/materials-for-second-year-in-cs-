#include<iostream>
#include<algorithm>

using namespace std;

int main()
{
    int x;
    cin >> x;

    //K = 17;
    int result1 = (x << 4) + x;

    // K = -7;
    int result2 = x - (x << 3);

    // K = 60;
    int result3 = (x << 6) - (x << 2);

    // K = -112;
    int result4 = (x << 4) - (x << 7);

    printf("%d %d %d %d\n",result1,result2,result3,result4);
    
    return 0;
}