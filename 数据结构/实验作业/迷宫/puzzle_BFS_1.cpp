#include<iostream>
#include<cstdlib>
#include<fstream>

using namespace std ;

const int N = 20;
const int arrmax = 100;

typedef struct{
    int *base;
    int front;
    int tail;
}Queue;

bool InitQueue(Queue & Q)
{
    Q.base = (int *) malloc (arrmax * sizeof(int));
    if (Q.base == NULL) return false;
    Q.front = Q.tail = 0;
    return true;
}

bool QueueEmpty(Queue & Q)
{
    if(Q.front == Q.tail) return true;
    else return false;
}

bool QueueFull(Queue & Q)
{
    if((Q.tail + 1) % arrmax == Q.front) return true;
    else return false;
}

bool PushQueue(Queue &Q,int e)
{
    if((Q.tail + 1) % arrmax == Q.front) return false;
    *(Q.base + Q.tail )= e;
    Q.tail = (Q.tail + 1) % arrmax ;
    return true;
}

bool PopQueue(Queue & Q,int & e)
{
    if(Q.front == Q.tail) return false ;
    e = *(Q.base + Q.front);
    Q.front = (Q.front + 1) % arrmax;
    return true;
}

bool QueueTop(Queue & Q,int & e)
{
    if(Q.front == Q.tail) return false;
    e = *(Q.base + Q.front);
    return true;
}

bool DestroryQueue(Queue & Q)
{
    free(Q.base);
    if(Q.base == NULL) return true;
    else return false;
}

int g[N][N];
int n,m;
int Start,End;
bool st[N];
int pre[N];
int dist[N];
Queue q;
int path[arrmax];
int path_len;

void print_path()
{
    int now = End;
    path_len = 0;
    while(now != Start){
        path[path_len++] = now;
        now = pre[now];
    }
    path[path_len++] = Start;
    for(int i = 0;i < path_len / 2;i ++)
    {
        int a = path[i];
        path[i] = path[path_len - 1 - i];
        path[path_len - 1 - i] = a;
    }
    for(int i = 0;i < path_len;i ++)
    {
        printf("%d ",path[i]);
    }
    printf("length:%d\n",path_len - 1);
}

int main()
{
    ifstream fin;
    fin.open("puzzle.txt",ios::in);
    if(!fin){
        printf("打开文件失败\n");
        return 0;
    }
    fin >> n >> m >> Start >> End;
    for(int i = 1;i <= m ; i ++)
    {
        int a,b;
        fin >> a >> b;
        g[a][b] = g[b][a] = 1 ;
    }

    InitQueue(q);
    PushQueue(q, Start);
    st[Start] = true;

    while(!QueueEmpty(q))
    {
        int front;
        QueueTop(q, front);
        int temp;
        PopQueue(q, temp);
        if(front == End)
        {
            print_path();
            break;
        }
        for(int i = 1;i <= n;i ++)
        {
            if(!st[i] && g[front][i])
            {
                PushQueue(q, i);
                st[i] = true;
                pre[i] = front;
                dist[i] = dist[front] + 1;
            }
        }
    }
    DestroryQueue(q);
    return 0;
}