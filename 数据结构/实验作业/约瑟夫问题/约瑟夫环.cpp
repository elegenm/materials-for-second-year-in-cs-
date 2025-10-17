#include<iostream>
#include<algorithm>
#include<cstring>

using namespace std;

const int N = 50;

typedef struct Node{
    int idx;
    string name;
    string gender;
    int age;
    struct Node * next;
}node,*listptr;

int n,s,m,x;

//创建循环链表
void createCircularlist(listptr &head,int n)
{
    listptr current = head;
    for(int i = 0;i < n;i ++)
    {
        int a,d;
        string b,c;
        cin >> a >> b >> c >> d;
        listptr newNode = new node;
        newNode->idx = a;
        newNode->name = b;
        newNode->gender = c;
        newNode->age = d;
        newNode->next = current->next;
        current->next = newNode;
        current = current->next;
    }
}

//打印出单个节点的信息
void printNodeinfo(listptr current)
{
    printf("idx:%d\nname:%s\ngender:%s\nage:%d\n\n", current->idx, current->name.c_str(), current->gender.c_str(), current->age);
}

//打印整个链表节点信息
void printCircularlist(listptr head)
{
    listptr current = head->next;
    while(current != head)
    {
        printNodeinfo(current);
        current = current->next;
    }
    puts("");
}

//找到编号为i的节点地址
listptr findNodeidx(listptr head,int i)
{
    if(head == NULL) return NULL;
    auto current = head->next;
    while(current != head)
    {
        if(current->idx == i)
        {
            return current;
        }
        current = current->next;
        
    }
    return NULL;
}

//找到链表中节点编号为i的节点的前驱，用于维护链表
listptr findPreNodeIdx(listptr head,int i)
{
    if(head == NULL) return NULL;
    auto pre = head;
    auto current = head->next;
    while(current != head)
    {
        if(current->idx == i)
        {
            return pre;
        }
        pre = current;
        current = current->next;
        
    }
    return NULL;
}

//对链表进行报数删除操作
void deleteNode(listptr head,int s,int m,int x)
{
    auto start = findNodeidx(head,s);//找到开始报数节点的地址
    auto preptr = findPreNodeIdx(head,s);//找到编号s节点的前驱，用于正确维护单向链表；
    auto current = start;
    int cnt = 0;//计数器，用于报数
    puts("删除节点编号:");
    while(n > x)
    {
        if(current == head){
            preptr = current;
            current = current->next;
            continue;
        }//对于循环链表的特殊判断，head头节点不进行报数，因此需要跳过

        cnt = (++ cnt) % m;//报数逻辑，从1开始，到m后取模为0进行删除，同时下一位从1开始报数

        if(cnt == 0)
        {
            n --;
            cout<<current->idx<<" ";
            preptr->next = current->next;
            delete current;
            current = preptr->next;
        }//删除节点操作，同时对删除的节点进行内存释放
        else 
        {
            preptr = current;
            current = preptr->next;
        }//不进行节点删除，下一位报数
    }
    puts("");
}

//对链表内存进行释放
void clear(listptr head)
{
    auto current = head->next;
    auto preptr = head;
    while(current != head)
    {
        preptr->next = current->next;
        delete current;
        current = preptr->next;
    }
    delete head;
}

int main()
{
    cin >> n;
    listptr head = new node;
    head->next = head;
    createCircularlist(head,n);//创建链表
    cout<<"循环链表各节点的值为:\n";
    printCircularlist(head);//打印链表

    cin >> s >> m >> x;
    listptr address = findNodeidx(head,s);
    if(address == NULL){
        cout<<"未发现编号为s的节点,请重新输入数据"<<endl;
        return 0;
    }

    cout<<"编号s的节点信息:"<<endl;
    printNodeinfo(address);

    deleteNode(head,s,m,x);

    cout<<"剩余节点:"<<endl;
    printCircularlist(head);
    
    clear(head);
    
    return 0;
}