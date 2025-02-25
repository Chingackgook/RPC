#include<iostream>
#include<cstring>
#include<vector>
#include<map>
#include<algorithm>
#include<sstream>
#include<mutex>
#include<fstream>

#include<winsock2.h>
#include<WS2tcpip.h>
#include<ws2ipdef.h>
#include<windows.h>
#include<ws2def.h>

#include"rely\mingw.thread.h"
#include"rely\mingw.shared_mutex.h"
#include"rely\mingw.mutex.h"
typedef unsigned long long int u64;
typedef std::string SerializedData;
using namespace std;

struct handle{
    string ser_ipnum;//服务端ipv4地址
    string ser_pornum;//服务端ipv4端口号
    string ser_ipnum6;//服务端ipv6地址
    string ser_pornum6;//服务端ipv6端口号
    vector<string> arglist;//参数列表
    u64 address;//指定函数的地址
    string funcinfo;//函数信息，用于向用户展示
    handle(string sip,string sip6,string spor,string spor6,u64 add,vector<string> alist=vector<string>(),string funinfo=""){
        ser_ipnum=sip;
        ser_ipnum6=sip6;
        ser_pornum=spor;
        ser_pornum6=spor6;
        address=add;
        arglist=alist;
        funcinfo=funinfo;
    }
};

struct Function_info{
    string functionname;//函数名
    string functioninfo;//函数信息,描述
    vector<string> arglist;//参数列表
    string returntype;//返回值类型
    u64 address;//函数地址（编号）
    bool available;//是否可用
};

struct Server_info{
    string selfipnum6;
    string selfipnum;
    string acceptipnum;
    string acceptipnum6;
    string portnum;
    string portnum6;
    vector<Function_info> functionInfo;
    int create_time;
    int numofclients;
    Server_info(){
        create_time=-1;
    }
};


class Rpc_data{
public:
    Rpc_data();
    vector<string> strings;//string列表
    vector<int> ints;//int列表
    vector<double> doubles;//double列表
    vector<char> chars;//char列表
    SerializedData serialize();//序列化（SerializedData就是string）
    void deserialize(SerializedData data);//反序列化
    void show();//展示数据
};

Rpc_data::Rpc_data(){
    strings.clear();
    ints.clear();
    doubles.clear();
    chars.clear();
}

SerializedData Rpc_data::serialize(){
    string result="";
    int stringnum=strings.size();
    int intnum=ints.size();
    int doublenum=doubles.size();
    int charnum=chars.size();
    result+=to_string(stringnum)+" ";
    for(int i=0;i<stringnum;i++){
        result+="\1"+strings[i]+"\1 ";
    }
    result+=to_string(intnum)+" ";
    for(int i=0;i<intnum;i++){
        result+=to_string(ints[i])+" ";
    }
    result+=to_string(doublenum)+" ";
    for(int i=0;i<doublenum;i++){
        result+=to_string(doubles[i])+" ";
    }
    result+=to_string(charnum)+" ";
    for(int i=0;i<charnum;i++){
        result+=chars[i];
    }
    return result;
}

void Rpc_data::deserialize(SerializedData data){
    int i=0;
    int stringnum=0;
    for(;data[i]!=' ';i++){
        stringnum*=10;
        stringnum+=data[i]-'0';
    }
    i++;
    for(int j=0;j<stringnum;j++){
        i++;
        string temp;
        for(;data[i]!='\1';i++){
            temp+=data[i];
        }
        i+=2;
        strings.push_back(temp);
    }
    //strings ok
    string restdata=data.substr(i);
    stringstream ss(restdata);
    int intnum;
    ss>>intnum;
    for(int j=0;j<intnum;j++){
        int temp;
        ss>>temp;
        ints.push_back(temp);
    }
    int doublenum;
    ss>>doublenum;
    for(int j=0;j<doublenum;j++){
        double temp;
        ss>>temp;
        doubles.push_back(temp);
    }
    int charnum;
    ss>>charnum;
    for(int j=0;j<charnum;j++){
        char temp;
        ss>>temp;
        chars.push_back(temp);
    }
}

void Rpc_data::show(){
    if(strings.size()!=0){
        cout<<"string个数为"<<strings.size()<<endl;
        for(int i=0;i<strings.size();i++){
            cout<<"第"<<i+1<<"个string为："<<strings[i]<<endl;
        }
    }
    if(ints.size()!=0){
        cout<<"int个数为"<<ints.size()<<endl;
        for(int i=0;i<ints.size();i++){
            cout<<"第"<<i+1<<"个int为："<<ints[i]<<endl;
        }
    }
    if(doubles.size()!=0){
        cout<<"double个数为"<<doubles.size()<<endl;
        for(int i=0;i<doubles.size();i++){
            cout<<"第"<<i+1<<"个double为："<<doubles[i]<<endl;
        }
    }
    if(chars.size()!=0){
        cout<<"char个数为"<<chars.size()<<endl;
        for(int i=0;i<chars.size();i++){
            cout<<"第"<<i+1<<"个char为："<<chars[i]<<endl;
        }
    }
}

std::string getLocalIPAddress() {
    char hostname[NI_MAXHOST];
    if (gethostname(hostname, NI_MAXHOST) != 0) {
        std::cerr << "Error getting hostname" << std::endl;
        return "NULL";
    }

    // 获取主机地址信息
    addrinfo* result = nullptr;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;  // 支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        std::cerr << "Failed to get address info" << std::endl;
        return "NULL";
    }

    std::string ipAddress;

    // 遍历地址信息列表，找到第一个 IPv4 地址
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET) {  // IPv4
            sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
            char ipStr[INET_ADDRSTRLEN];
            DWORD ipStrLen = INET_ADDRSTRLEN;
            WSAAddressToStringA(ptr->ai_addr, sizeof(sockaddr_in), nullptr, ipStr, &ipStrLen);
            ipAddress = ipStr;
            break;
        }
    }
    freeaddrinfo(result);
    if(ipAddress==""){
        return "NULL";
    }
    return ipAddress;
}

std::string getLocalIPv6Address() {
    char hostname[NI_MAXHOST];
    if (gethostname(hostname, NI_MAXHOST) != 0) {
        std::cerr << "Error getting hostname" << std::endl;
        return "NULL";
    }
    // 获取主机地址信息
    addrinfo* result = nullptr;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;  // 支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        std::cerr << "Failed to get address info" << std::endl;
        return "NULL";
    }
    std::string ipv6Address;
    // 遍历地址信息列表，找到第一个 IPv6 地址
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        if (ptr->ai_family == AF_INET6) {  // IPv6
            sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(ptr->ai_addr);
            DWORD ipStrLen = INET6_ADDRSTRLEN;
            char ipStr[INET6_ADDRSTRLEN];
            WSAAddressToStringA(ptr->ai_addr, sizeof(sockaddr_in6), nullptr, ipStr, &ipStrLen);
            ipv6Address = ipStr;
            break;
        }
    }
    freeaddrinfo(result);
    if(ipv6Address==""){
        return "NULL";
    }
    return ipv6Address;
}


int sendAll(int socket, const char* buffer, size_t length) {
    int n;
    long long total = 0;
    // 发送数据
    while (total < length) {
        n = send(socket, buffer + total, length - total, 0);
        if (n == -1) { 
            break; // 发送失败
        }
        total += (long long)n;
    }
    // 发送结束标志
    if (n != -1) {
        n = send(socket, "\0", 1, 0); // 使用'\0'作为结束标志
    }
    return n == -1 ? -1 : (int)total;
}

// 接收数据，直到遇到'\0'结束标志
int recvWithTimeout(int socket, string& buffer, size_t length, int timeoutSeconds=30) {
    int n;
    int total = 0;
    char tempbuf[1024];
    while (length==-1||total < length) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(socket, &fds);
        struct timeval timeout;
        timeout.tv_sec = timeoutSeconds;
        timeout.tv_usec = 0;
        int activity = select(socket + 1, &fds, NULL, NULL, &timeout);
        if (activity < 0) {
            return -1;
        } else if (activity == 0) {
            // 超时
            return 0; // 返回超时错误
        }
        n = recv(socket, tempbuf, 1024, 0);
        if (n == -1) {
            return -1; // 接收错误
        } else if (n == 0) {
            return -1; // 对端关闭连接
        }
        total += n;
        if(tempbuf[n-1]=='\0'){
            buffer.append(tempbuf,n-1);//去掉结束标志
            break;
        }
        buffer.append(tempbuf,n);
    }
    return (int)total;
}


int connectWithTimeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout_sec) {
    u_long mode = 1;
    if (ioctlsocket(sockfd, FIONBIO, &mode) == SOCKET_ERROR) {
        perror("ioctlsocket failed");
        return -1;
    }
    // 连接套接字
    if (connect(sockfd, addr, addrlen) == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            perror("connect failed");
            return -1;
        }
    }
    // 创建 fd_set，并将 sockfd 添加到其中
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sockfd, &writeSet);
    // 设置超时时间
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    // 使用 select 函数检查 sockfd 是否可写，设置超时时间
    int result = select(0, NULL, &writeSet, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        //perror("select failed");
        return -1;
    } else if (result == 0) {
        return -1;
    }
    // 连接成功
    return 1;
}

int getiptype(string ip){
    if(ip.find(":")!=string::npos||ip=="IN6ADDR_ANY"){
        return 6;
    }
    else{
        return 4;
    }
}


vector<string> askforanother(int type){
    cout<<"检测到监听的ip类型为ipv"<<type<<endl;
    vector<string> result;
    if(type==4){
        cout<<"是否需要监听ipv6地址？(y/n)"<<endl;
        char c;
        cin>>c;
        if(c=='y'){
            result.push_back("1");
            cout<<"ipv6地址："<<endl;
            string ip6;
            cin>>ip6;
            cout<<"端口号："<<endl;
            string port6;
            cin>>port6;
            if(ip6=="::"){
                ip6="IN6ADDR_ANY";
            }
            result.push_back(ip6);
            result.push_back(port6);
        }
        else{
            result.push_back("0");
        }
    }
    else{
        cout<<"是否需要监听ipv4地址？(y/n)"<<endl;
        char c;
        cin>>c;
        if(c=='y'){
            result.push_back("1");
            cout<<"ipv4地址："<<endl;
            string ip4;
            cin>>ip4;
            cout<<"端口号："<<endl;
            string port4;
            cin>>port4;
            result.push_back(ip4);
            result.push_back(port4);
        }
        else{
            result.push_back("0");
        }
    }
    return result;
}

bool initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        // 初始化失败
        return false;
    }
    return true;
}

void cleanupWinsock() {
    WSACleanup();
}

bool isgoodport(string port){
    if(port=="不支持"){
        return true;
    }
    for(int i=0;i<port.size();i++){
        if(port[i]<'0'||port[i]>'9'){
            return false;
        }
    }
    string temp=port;
    int portnum=stoi(temp);
    if(portnum<0||portnum>65535)
        return false;
    return true;
}