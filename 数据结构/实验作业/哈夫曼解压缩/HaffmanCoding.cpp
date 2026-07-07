#include<iostream>
#include<algorithm>
#include<queue>
#include<fstream>
#include<cstring>
#include<map>
#include<sstream>
#include<vector> 
#include<iomanip>

using namespace std;

const int N = 256; 

int char_freq[N];

typedef struct node{
    char ch;
    int fre;
    node * leftNode = NULL;
    node * rightNode = NULL;
}Haffman_Tree_Node,*Haff_List;

struct compare{
    bool operator()(Haff_List a,Haff_List b){
        return a->fre > b->fre;
    }
};

priority_queue<Haff_List,vector<Haff_List>,compare> q;

// --- 原有哈夫曼功能函数 (保持不变) ---

void create_Haffman_Tree()
{
    while(q.size() > 1){
        Haff_List leftnode = q.top();
        q.pop();
        Haff_List rightnode = q.top();
        q.pop();
        Haff_List newnode = new node;
        newnode->leftNode = leftnode;
        newnode->rightNode = rightnode;
        newnode->fre = leftnode->fre + rightnode->fre;
        newnode->ch = '\0';
        q.push(newnode);
    }
}

void generate_codetable(Haff_List root,string code,map<char,string> &codeTable)
{
    if(!root) return;
    if(root->leftNode == NULL&&root->rightNode== NULL){
        codeTable[root->ch] = code;
        return;
    }
    generate_codetable(root->leftNode,code +'0',codeTable);
    generate_codetable(root->rightNode,code + '1',codeTable);
}

void write_codetable(map <char,string>& codetable,string filename)
{
    ofstream outFile(filename,ios::out);
    if(!outFile) {
        printf("genereting codetable file failed\n");
        return;
    }
    outFile<<"codetable:"<<endl;
    for(auto pair:codetable){
        char ch = pair.first;
        string code = pair.second;
        // 稍微优化显示，避免控制字符乱码
        if(ch == '\n') outFile << "\\n \t " << code << endl;
        else if(ch == '\r') outFile << "\\r \t " << code << endl;
        else if(ch == ' ') outFile << "SPACE \t " << code << endl;
        else outFile << "'" << ch << "' \t " << code << endl;
    }
}

void compress_file(map <char,string>&codetable,string input_filename,string output_filename)
{
    ifstream inFile(input_filename,ios::in);
    ofstream outFile(output_filename,ios::binary|ios::out);
    if(!inFile || !outFile){
        printf("opening file failed\n");
        return;
    }

    outFile<<codetable.size()<<endl;
    for(auto pair: codetable){
        outFile<<(int)(unsigned char)pair.first << " " << pair.second.length() << " " << pair.second << endl;
    }

    char bit_code = 0; // 初始化为0
    int bit_count = 0;

    char c;
    inFile >> noskipws; // 确保读取空格
    while(inFile >> c)
    {
        auto it = codetable.find(c);
        if(it == codetable.end()) continue;

        string code = it->second;

        for(auto c: code)
        {
            bit_code <<= 1;
            if(c == '1'){
                bit_code |= 1;
            }
            bit_count ++;

            if(bit_count == 8)
            {
                outFile.put(bit_code);
                bit_code = 0; // 重置
                bit_count = 0;
            }
        }
    }
    if(bit_count > 0)
    {
        bit_code <<= (8 - bit_count);
        outFile.put(bit_code);
    }

    inFile.close();
    outFile.close();
}

void decompress_file(string input_filename,string output_filename)
{
    ifstream inFile(input_filename,ios::binary|ios::in);
    ofstream outFile(output_filename,ios::out);
    if(!inFile || !outFile){
        printf("open the decompress file failed");
        if(inFile) inFile.close();
        if(outFile) outFile.close();
        return;
    }
    int n;
    string line;
    getline(inFile,line); // 读取数量行
    stringstream ss(line);
    ss >> n;
    
    map<string,char> discodetable;
    for(int i = 0; i < n; i++) // 修改循环，从0开始比较符合直觉
    {
        int ascii_c, len;
        string str_code;
        inFile >> ascii_c >> len >> str_code;
        discodetable[str_code] = (char)ascii_c;
    }
    // 吃掉码表后的换行符，移动到数据区
    string dummy; getline(inFile, dummy); 

    int byte;
    char byte_code;
    string temp_str = "";
    while((byte = inFile.get()) != EOF){
        byte_code = (char) byte;
        for(int i = 7; i >= 0; i--){ // 修正解码逻辑：逐位处理
            int current_bit = (byte_code >> i) & 1;
            temp_str += (current_bit == 1)?"1":"0";
            auto it = discodetable.find(temp_str);
            if(it != discodetable.end()){
                outFile.put(it->second);
                temp_str.clear();
            }
        }
    } 
    inFile.close();
    outFile.close();
    return;
}

long get_file_size(string filename) {
    ifstream in(filename, ios::ate | ios::binary);
    if(!in) return 0;
    return in.tellg(); 
}


void getBadChar(string pattern, int m, int badchar[]) {
    for (int i = 0; i < 256; i++)
        badchar[i] = -1;
    for (int i = 0; i < m; i++)
        badchar[(unsigned char)pattern[i]] = i;
}


vector<int> BoyerMooreSearch(string txt, string pat) {
    vector<int> positions;
    int m = pat.size();
    int n = txt.size();
    int badchar[256];

    getBadChar(pat, m, badchar);

    int s = 0; 
    while(s <= (n - m)) {
        int j = m - 1;
        while(j >= 0 && pat[j] == txt[s + j])
            j--;

        if (j < 0) {
            positions.push_back(s); // 记录位置
            s += (s + m < n) ? m - badchar[(unsigned char)txt[s + m]] : 1;
        } else {
            s += max(1, j - badchar[(unsigned char)txt[s + j]]);
        }
    }
    return positions;
}


void search_keyword(string compressed_file, string keyword) {
    ifstream inFile(compressed_file, ios::binary | ios::in);
    if(!inFile) { cout << "Search error: file not found." << endl; return; }

    // 重建映射表
    int n;
    inFile >> n;
    string dummy; getline(inFile, dummy); 

    map<string, char> discodetable;
    for(int i = 0; i < n; i++) {
        int ascii_c, len;
        string s_code;
        inFile >> ascii_c >> len >> s_code;
        discodetable[s_code] = (char)ascii_c;
    }
    getline(inFile, dummy); 


    string memory_text = ""; 
    int byte;
    string temp_str = "";
    while((byte = inFile.get()) != EOF){
        for(int i = 7; i >= 0; i--){
            int bit = (byte >> i) & 1;
            temp_str += (bit ? '1' : '0');
            if(discodetable.count(temp_str)) {
                memory_text += discodetable[temp_str];
                temp_str = "";
            }
        }
    }
    inFile.close();

 
    cout << "\n[Search Result]" << endl;
    cout << "Keyword: " << keyword << endl;
    vector<int> results = BoyerMooreSearch(memory_text, keyword);
    
    if(results.empty()) {
        cout << "Status: Not Found" << endl;
    } else {
        cout << "Status: Found " << results.size() << " times." << endl;
        cout << "Positions: ";
        for(int pos : results) cout << pos << " ";
        cout << endl;
    }
}



int main()
{
    memset(char_freq, 0, sizeof(char_freq)); // 初始化频率数组

    ifstream inFile;
    inFile.open("input.txt",ios::in);
    if(!inFile) {
        ofstream test("input.txt"); test << "This is a test data for Huffman BM search."; test.close();
        inFile.open("input.txt", ios::in);
    }
    
    char a;
    inFile >> noskipws; 
    while(inFile >> a){
        char_freq[(unsigned char)a]++;
    }
    inFile.close();
    
    for(int i = 0;i < 256;i ++) 
    {   
        if(char_freq[i] == 0) continue;
        Haff_List newnode = new Haffman_Tree_Node;
        newnode->ch = i;
        newnode->fre = char_freq[i];
        q.push(newnode);
    }

    if(q.empty()) return 0;

    create_Haffman_Tree();

    Haff_List root = q.top();

    map<char,string> codeTable;
    string code;
    generate_codetable(root,code,codeTable);
    
    if(codeTable.size() == 1){
        auto it = codeTable.begin();
        it->second = "0";
    }

    write_codetable(codeTable,"codetable.txt");

    compress_file(codeTable,"input.txt","output.txt");

    decompress_file("output.txt","final.txt");

    long originalSize = get_file_size("input.txt");
    long compressedSize = get_file_size("output.txt");

    cout << "\n---------------- Statistics ----------------" << endl;
    cout << "Original File Size:   " << originalSize << " bytes" << endl;
    cout << "Compressed File Size: " << compressedSize << " bytes" << endl;
    
    if (originalSize > 0) {
        double ratio = ((double)compressedSize / originalSize) * 100.0;
        cout << fixed << setprecision(2);
        cout << "Compression Ratio:    " << ratio << "%" << endl;
    }
    cout << "\n---------------- Keyword Search ----------------" << endl;
    cout << "Enter a keyword to search in compressed file: ";
    string keyword;
    cin >> keyword;
    search_keyword("output.txt", keyword);

    return 0;
}