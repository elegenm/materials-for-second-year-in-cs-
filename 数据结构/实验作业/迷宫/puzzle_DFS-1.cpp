#include<iostream>
#include<cstdlib>
#include<fstream>

using namespace std;

const int arrmax = 100;
const int N = 20;

int g[N][N];
int n,m;
int Start,End;
int next_branch[N];
bool st[N];

typedef struct{
    int *top;
    int *base;
    int StackSize;
} Sqstack;

bool InitStack(Sqstack &S) {
    S.base = (int*)malloc(arrmax * sizeof(int));
    if (S.base == NULL) return false;
    S.top = S.base;
    S.StackSize = arrmax;
    return true;
}

bool StackEmpty(Sqstack &S) {
    return (S.base == S.top);
}

bool PushStack(Sqstack &S, int e) {
    if (S.top - S.base == S.StackSize) {
        return false;
    }
    *S.top++ = e;
    return true;
}

bool PopStack(Sqstack &S, int &e) {
    if (S.top == S.base) return false;
    e = *--S.top;
    return true;
}

bool GetTop(Sqstack &S, int &e) {
    if (S.top == S.base) return false;
    e = *(S.top - 1);
    return true;
}

void print_path(Sqstack &pathStack) {
    int *p = pathStack.base;
    while (p < pathStack.top) {
        printf("%d ", *p);
        p++;
    }
    printf("    length:%ld\n", (pathStack.top - pathStack.base) - 1);
}

int main() {
    ifstream fin;
    fin.open("puzzle.txt", ios::in);
    fin >> n >> m >> Start >> End;

    for (int i = 1; i <= m; i++) {
        int a, b;
        fin >> a >> b;
        g[a][b] = 1;
    }

    Sqstack pathStack;
    InitStack(pathStack);

    PushStack(pathStack, Start);
    st[Start] = true;

    while (!StackEmpty(pathStack)) {
        int front;
        GetTop(pathStack, front);

        if (front == End) {
            print_path(pathStack);

            st[front] = false;
            int e;
            PopStack(pathStack, e);

            if (!StackEmpty(pathStack)) {
                int prev;
                GetTop(pathStack, prev);
                next_branch[prev]++;
            }
        } else {
            bool founded = false;
            for (int i = next_branch[front] + 1; i <= n; i++) {
                if (g[front][i] && !st[i]) {
                    st[i] = true;
                    next_branch[front] = i;
                    PushStack(pathStack, i);
                    next_branch[i] = 0;
                    founded = true;
                    break;
                }
            }

            if (!founded) {
                st[front] = false;
                next_branch[front] = 0;
                int e;
                PopStack(pathStack, e);
            }
        }
    }

    return 0;
}