#include<iostream>
#include<cstring>
#include<vector>
#include<map>
#include<algorithm>
#include<sstream>
#include<mutex>
#include<shared_mutex>
#include<map>
#include<fstream>

#include<fcntl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<ifaddrs.h>
#include<thread>
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
    strings.resize(0);
    ints.resize(0);
    doubles.resize(0);
    chars.resize(0);
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
    std::string ipAddress;
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET) {
                struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                char* addr = inet_ntoa(sa->sin_addr);
                // 排除回环地址
                if (strcmp(addr, "127.0.0.1") != 0) {
                    ipAddress = addr;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    return ipAddress;
}

std::string getLocalIPv6Address() {
    std::string ipAddress;
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET6) {
                struct sockaddr_in6* sa6 = reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr);
                char addr[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &(sa6->sin6_addr), addr, INET6_ADDRSTRLEN);
                if (strcmp(addr, "::1") != 0 && strncmp(addr, "fe80:", 5) != 0) {
                    ipAddress = addr;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    if(ipAddress==""){
        ipAddress="NULL";
    }
    return ipAddress;
}


ssize_t sendAll(int socket, const char* buffer, size_t length) {
    ssize_t n;
    long long total = 0;
    while (total < length) {
        n = send(socket, buffer + total, length - total, 0);
        if (n == -1) break; 
        total += (long long)n;
    }
    // 发送结束标志
    if (n != -1) {
        n = send(socket, "\0", 1, 0); // 使用'\0'作为结束标志
    }
    return n==-1?-1:(ssize_t)total;
}

// 接收数据，直到遇到'\0'结束标志
int recvWithTimeout(int socket, string& buffer, size_t length, int timeoutSeconds=30) {
    ssize_t n;
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
        if (activity < 0)return -1;
        else if (activity == 0) return 0; // 超时，返回错误
        n = recv(socket, tempbuf, 1024, 0);
        if (n == -1) return -1; // 接收错误
        else if (n == 0) return -1; // 对端关闭连接
        total += n;
        if (tempbuf[n - 1] == '\0') {
            buffer.append(tempbuf, n - 1);
            break;
        }
        buffer.append(tempbuf, n);
    }
    return (int)total;
}

int connectWithTimeout(int sock, const struct sockaddr *addr, socklen_t addrlen, int timeout_sec) {
    // 设置 socket 为非阻塞模式
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) return -1;
    if (fcntl(sock,F_SETFL,flags|O_NONBLOCK)<0) return -1;
    int result = connect(sock, addr, addrlen);
    if (result < 0 && errno != EINPROGRESS) return -1;
    if (result < 0 && errno == EINPROGRESS) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(sock, &set);
        struct timeval tv;
        tv.tv_sec = timeout_sec;tv.tv_usec = 0;
        result = select(sock + 1, NULL, &set, NULL, &tv);
        if (result < 0) return -1;
        else if (result == 0) return -1; // 连接超时
        else {
            int error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0)return -1;
            if (error != 0)return -1;
        }
    }
    if (fcntl(sock, F_SETFL, flags) < 0)return -1;
    return 1;
}

int getiptype(string ip){
    if(ip.find(":")!=string::npos){
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

bool isgoodport(string portnum){
    if(portnum=="不支持"){
        return 1;
    }
    for(int i=0;i<portnum.size();i++){
        if(portnum[i]<'0'||portnum[i]>'9'){
            return false;
        }
    }
    int port=stoi(portnum);
    if(port>65535||port<0){
        return false;
    }
    return true;
}