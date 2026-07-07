#include<iostream>
#include<algorithm>

using namespace std;

int mul3div4(int x)
{
    x = (x << 1) + x;//x * 3的位级表示
    int flag = x & INT_MIN;//取x的符号位
    flag && (x += (1 << 2) - 1);//当x符号位为1时，添加偏置；
    //如果为0，根据&运算的规则，后续语句直接跳过，不需要添加偏置；
    return x >> 2;//保证不能整除的负数除法的取整正确
}

int main()
{
    int x = -4444999;

    //进行结果验证；

    printf("%d\n",3 * x / 4);
    printf("%d\n",mul3div4(x));
}