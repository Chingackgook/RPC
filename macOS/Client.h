#include"RPC.h"
using namespace std;
//test

class Client {
private:
    int reg_iptype;//注册中心的ip类型
    string reg_ipnum;//注册中心的ip地址
    string reg_portnum;//注册中心的端口号
public:
    Client();

    //初始化客户端，并尝试连接注册中心，返回是否成功
    bool init(string reg_ipnum,string reg_portnum, int iptype);

    //查询服务，若functionname为空则查询所有服务，否则查询指定服务，返回信息
    string checkServer(string functionname="");

    //调用服务前的准备工作，向注册中心查询服务，返回服务ip地址、端口号、函数地址、参数个数、参数列表
    //若ipnum不为空则查询指定ip的服务
    handle findServer(string functionname,string ipnum="");

    //调用服务，向服务端发送数据，返回服务端处理结果
    //hand包含服务端ip地址、端口号、函数地址、参数列表，data包含可序列化的数据
    Rpc_data callServer(handle hand,Rpc_data data);
};


Client::Client(){}


bool Client::init(string reg_ipnum,string reg_portnum, int reg_iptype){
    this->reg_iptype=reg_iptype;
    this->reg_ipnum=reg_ipnum;
    this->reg_portnum=reg_portnum;
    //cout<<reg_ipnum<<" "<<reg_portnum<<endl;
    //创建一个套接字尝试连接注册中心
    int sock=0;
    sockaddr_in serv_addr;
    sockaddr_in6 serv_addr6;
    if(reg_iptype==4){
        sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=inet_addr(reg_ipnum.c_str());
        serv_addr.sin_port=htons(stoi(reg_portnum));
    }
    else if(reg_iptype==6){
        sock=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
        serv_addr6.sin6_family=AF_INET6;
        inet_pton(AF_INET6, reg_ipnum.c_str(), &serv_addr6.sin6_addr);
        serv_addr6.sin6_port=htons(stoi(reg_portnum));
    }
    if(sock<0){
        cout<<"套接字创建失败"<<endl;
        return 0;
    }
    if(reg_iptype==4){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr),5)<0){
            cout<<"连接失败或超时，请检查注册中心ipv4地址与端口是否输入正确"<<endl;
            return 0;
        }
    }
    else if(reg_iptype==6){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr6,sizeof(serv_addr6),5)<0){
            cout<<"连接失败或超时，请检查注册中心ipv6地址与端口是否输入正确"<<endl;
            return 0;
        }
    }
    string data="Client connect";
    if(sendAll(sock,data.c_str(),data.size())<0){
        cout<<"初始化信息发送失败"<<endl;
        close(sock);
        return 0;
    }
    close(sock);
    return 1;
}


string Client::checkServer(string functionname){
    bool allsign=1;
    if(functionname!=""){
        allsign=0;
    }
    int sock=0;
    sockaddr_in serv_addr;
    sockaddr_in6 serv_addr6;
    if(reg_iptype==4){
        sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=inet_addr(reg_ipnum.c_str());
        serv_addr.sin_port=htons(stoi(reg_portnum));
    }
    else if(reg_iptype==6){
        sock=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
        serv_addr6.sin6_family=AF_INET6;
        inet_pton(AF_INET6, reg_ipnum.c_str(), &serv_addr6.sin6_addr);
        serv_addr6.sin6_port=htons(stoi(reg_portnum));
        //cout<<reg_ipnum<<endl;
        //cout<<reg_portnum<<endl;
    }
    if(sock<0){
        return "套接字创建失败";
    }
    if(reg_iptype==4){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr),10)<0){
            return "连接失败或超时";
        }
    }
    else if(reg_iptype==6){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr6,sizeof(serv_addr6),10)<0){
            return "连接失败或超时";
        }
    }
    string data;
    if(allsign){
        data="Client all";
    }
    else{
        data="Client check "+functionname;
    }
    sendAll(sock,data.c_str(),data.size());
    string info;
    int byterecvd=recvWithTimeout(sock, info, -1, 10);
    if(byterecvd==0){
        close(sock);
        return "接收返回信息超时";
    }
    if(byterecvd<0){
        close(sock);
        return "接收返回信息失败";
    }
    close(sock);
    return info;
}

handle Client::findServer(string functionname,string ipnum){
    //向注册中心请求指定函数名的服务器
    int sock=0;
    sockaddr_in serv_addr;
    sockaddr_in6 serv_addr6;
    if(reg_iptype==4){
        sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=inet_addr(reg_ipnum.c_str());
        serv_addr.sin_port=htons(stoi(reg_portnum));
    }
    else if(reg_iptype==6){
        sock=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
        serv_addr6.sin6_family=AF_INET6;
        inet_pton(AF_INET6, reg_ipnum.c_str(), &serv_addr6.sin6_addr);
        serv_addr6.sin6_port=htons(stoi(reg_portnum));
    }
    if(sock<0){
        cout<<"套接字创建失败"<<endl;
        return handle("fail","套接字创建失败","no","failcreatsocket",0);
    }
    if(reg_iptype==4){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr),10)<0){   
            return handle("fail","连接注册中心失败或超时","no","failconnect",0);
        }
    }
    else if(reg_iptype==6){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr6,sizeof(serv_addr6),10)<0){
            return handle("fail","连接注册中心失败或超时","no","failconnect",0);
        }
    }
    string data="";
    if(ipnum==""){
        data+="Client get withoutip "+functionname;
    }
    else{
        data+="Client get withip "+functionname+" "+ipnum;
    }
    sendAll(sock,data.c_str(),data.size());
    string response;
    int bytesReceived =recvWithTimeout(sock, response, 1024, 10);
    if (bytesReceived == 0) {
        cout << "Receive timeout" << endl;
        close(sock);
        return handle("fail","接收服务器返回超时","no","timeout",0);
    } 
    else if (bytesReceived < 0) {
        cout << "Error in recv" << endl;
        close(sock);
        return handle("fail","接收服务器返回失败","no","error",0);
    }
    close(sock);

    if(response=="Function not found"){
        return handle("fail","未找到该服务","no","function not found",0);
    }
    else{
        stringstream ss(response);
        string ip;
        string ip6;
        string port;
        string port6;
        u64 address;
        string funcinfo;
        ss>>ip>>ip6>>port>>port6>>address;
        int argnum=0;
        ss>>argnum;
        vector<string> arg;
        for(int i=0;i<argnum;i++){
            string temp;
            ss>>temp;
            arg.push_back(temp);
        }
        ss>>funcinfo;
        return handle(ip,ip6,port,port6,address,arg,funcinfo);
    }
}

Rpc_data Client::callServer(handle hand,Rpc_data data){
    Rpc_data reply;
    int iptype;
    string ser_ip;
    string ser_port;
    //cout<<hand.ser_ipnum6<<endl;
    //cout<<"hand:"<<hand.ser_ipnum<<" "<<hand.ser_ipnum6<<hand.ser_pornum<<" "<<hand.ser_pornum6<<endl;
    if(hand.ser_ipnum!="不支持"){
        iptype=4;
        ser_ip=hand.ser_ipnum;
        ser_port=hand.ser_pornum;
    }
    else if(hand.ser_ipnum6!="不支持"){
        //cout<<"yes"<<endl;
        iptype=6;
        ser_ip=hand.ser_ipnum6;
        ser_port=hand.ser_pornum6;
    }
    else{
        reply.strings.push_back("服务器信息错误");
        return reply;
    }
    u64 address=hand.address;
    int sock=0;
    sockaddr_in serv_addr;
    sockaddr_in6 serv_addr6;
    if(iptype==4){
        sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_addr.s_addr=inet_addr(ser_ip.c_str());
        serv_addr.sin_port=htons(stoi(ser_port));
    }
    else{
        sock=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
        serv_addr6.sin6_family=AF_INET6;
        inet_pton(AF_INET6, ser_ip.c_str(), &serv_addr6.sin6_addr);
        serv_addr6.sin6_port=htons(stoi(ser_port));
    }
    if(sock<0){
        reply.strings.push_back("套接字创建失败");
        return reply;
    }
    if(iptype==4){
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr),10)<0){
            reply.strings.push_back("连接失败或超时4");
            return reply;
        }
    }
    else{
        if(connectWithTimeout(sock,(struct sockaddr*)&serv_addr6,sizeof(serv_addr6),10)<0){
            reply.strings.push_back("连接失败或超时6");
            return reply;
        }
    }
    string senddata="";
    senddata+=to_string(address)+" ";
    //将数据序列化
    senddata+=data.serialize();
    sendAll(sock,senddata.c_str(),senddata.size());
    string info;
    int bytesReceived = (int)recvWithTimeout(sock, info, -1,40);
    if (bytesReceived == 0) {
        reply.strings.push_back("数据接收超时");
        return reply;
    } 
    else if (bytesReceived < 0) {
        reply.strings.push_back("数据接收失败");
        return reply;
    }
    reply.deserialize(info);
    close(sock);
    return reply;
}