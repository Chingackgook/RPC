#include"RPC.h"
using namespace std;

class Server{
private:
    Server_info info;//服务器信息，包含函数列表、ip地址、端口号、等
    mutex serverLock;//服务器锁，用于锁客户端数量
    string reg_ipv4;//注册中心ipv4地址
    string reg_port4;//注册中心ipv4端口号
public: 
    Server();
    int id;//服务器id，由注册中心分配，若注册失败则为-1

    //添加函数，将函数信息加入到functionInfo中
    void addfunction(string functionname,string functioninfo, vector<string> arglist, string returntype, u64 address);

    //初始化服务器，包括监听的ipv4地址、端口号、ipv6地址、端口号、自身ipv4地址、ipv6地址、创建时间
    void init(string acceptipnum,string acceptipnum6, string portnum,string portnum6, string selfipnum, string selfipnum6,int create_time);

    //注册服务器，将服务器信息发送给注册中心，注册成功则将id赋值为注册中心分配的id，失败则为-1
    void regist(string reg_ipnum, string reg_portnum ,int iptype);

    //服务器准备就绪，开始监听，ok4和ok6分别表示是否监听ipv4和ipv6
    //将开启acceptclient和acceptclient6两个线程，分别监听ipv4和ipv6
    void readyToServe(bool ok4,bool ok6);

    //接受客户端连接，ok为true表示监听ipv4，false表示不监听
    void acceptclient(int sock,bool sign);

    //接受客户端连接，ok为true表示监听ipv6，false表示不监听
    void acceptclient6(int sock,bool sign);

    //处理客户端请求，包括接收客户端数据，解析数据，调用函数，返回结果
    void handleclient(int sock);

    //心跳，每5秒向注册中心发送心跳包，若3次未收到回复则停止心跳
    void heartbeats();

    //下线，向注册中心发送下线信息
    void deadhb();

    //显示服务列表
    void showavailable();

    //上线或下线函数，sign为1表示上线，0表示下线
    //当接收到正确返回信息时，再将本地函数状态改变
    void beavailableornot(string functionname,bool sign);
};

Server::Server(){}


void Server::init(string acceptipnum, string acceptipnum6, string portnum ,string portnum6,string selfipnum, string selfipnum6,int create_time){
    info.acceptipnum = acceptipnum;
    info.acceptipnum6 = acceptipnum6;
    info.portnum = portnum;
    info.numofclients = 0;
    //如果指定了监听的ip地址，则使用指定的ip地址。如果监听所有，则返回本机的一个ip地址
    info.selfipnum6 = acceptipnum6=="IN6ADDR_ANY"?selfipnum6:acceptipnum6;
    info.selfipnum = acceptipnum=="0.0.0.0"?selfipnum:acceptipnum;
    info.create_time = create_time;
    info.portnum6 = portnum6;
    id=-1;
}


void Server::regist(string reg_ipnum, string reg_portnum,int reg_iptype){
    if(id!=-1){
        cout<<"服务器已经注册，是否强制注册到新的注册中心？（不推荐）"<<endl;
        cout<<"(y/n):";
        char sign;
        cin>>sign;
        if(sign=='n')
            return;
    }
    // 创建客户端套接字
    sockaddr_in serverAddr;
    sockaddr_in6 serverAddr6;
    int clientSocket;
    if(reg_iptype==4){
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(reg_ipnum.c_str());
        serverAddr.sin_port = htons(stoi(reg_portnum));
    }
    else{
        clientSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
        serverAddr6.sin6_family = AF_INET6;
        int len = sizeof(serverAddr6);
        WSAStringToAddressA((LPSTR)(reg_ipnum.c_str()), AF_INET6, NULL, (sockaddr*)&serverAddr6.sin6_addr, &len);
        serverAddr6.sin6_port = htons(stoi(reg_portnum));
    }
    if (clientSocket == -1) {
        cout << "套接字创建失败" << endl;
        return ;
    }
    // 连接服务器
    if(reg_iptype==4){
        if (connectWithTimeout(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr),10) == -1) {
            cout << "连接失败，请检查注册中心ip地址和端口号是否正确" << endl;
            closesocket(clientSocket);
            return ;
        }
    }
    else{
        if (connectWithTimeout(clientSocket, (struct sockaddr*)&serverAddr6, sizeof(serverAddr6),10) == -1) {
            cout << "连接失败，请检查注册中心ip地址和端口号是否正确" << endl;
            closesocket(clientSocket);
            return ;
        }
    }

    // 发送注册信息给服务器
    string info = "Server " + this->info.acceptipnum;
    info+= " " + this->info.acceptipnum6;
    info+= " " + this->info.portnum;
    info+=" "+this->info.portnum6;
    info+= " " + this->info.selfipnum;
    info+= " " + this->info.selfipnum6;
    info += " " + to_string(this->info.functionInfo.size());
    for (int i = 0; i < this->info.functionInfo.size(); i++) {
        info += " " + this->info.functionInfo[i].functionname;
        info += " " + this->info.functionInfo[i].functioninfo;
        info += " " + to_string(this->info.functionInfo[i].arglist.size());
        for (int j = 0; j < this->info.functionInfo[i].arglist.size(); j++) {
            info += " " + this->info.functionInfo[i].arglist[j];
        }
        info += " " + this->info.functionInfo[i].returntype;
        info+= " " + to_string(this->info.functionInfo[i].available);
        info += " " + to_string(this->info.functionInfo[i].address);
    }
    info+=" "+to_string(this->info.numofclients);
    info+=" "+to_string(this->info.create_time);
    //info end
    int bytesSent = sendAll(clientSocket, info.c_str(), info.size());
    if (bytesSent == -1) {
        cout << "发送注册信息失败" << endl;
        id=-1;
        closesocket(clientSocket);
        return ;
    }

    // 等待数据中心回复
    string response;
    int bytesReceived = recvWithTimeout(clientSocket, response, 100, 10);

    if (bytesReceived == -1) {
        cout << "接收注册中心返回数据失败，服务未注册" << endl;
        id=-1;
        closesocket(clientSocket);
        return ;
    }
    // 处理回复
    stringstream ss(response);
    string isoksign="";
    ss >> isoksign;
    if(isoksign=="OK"){
        ss>>id;
        ss>>reg_ipv4;
        ss>>reg_port4;
    }
    // 关闭客户端套接字
    closesocket(clientSocket);
    //开始心跳
    if(id!=-1){
        thread heartbeatsThread(&Server::heartbeats,this);
        heartbeatsThread.detach();
        cout<<"服务器注册成功，注册于："<<reg_ipnum <<" id:"<<id<<endl;
    }
    return;
}


void Server::readyToServe(bool ok4,bool ok6){
    int serverSocket4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int serverSocket6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if(ok4){
        //设置服务器地址和端口
        sockaddr_in serverAddr4;
        serverAddr4.sin_family = AF_INET;
        serverAddr4.sin_port = htons(stoi(info.portnum));
        serverAddr4.sin_addr.s_addr = inet_addr(info.acceptipnum.c_str());
        if (::bind(serverSocket4, (struct sockaddr*)&serverAddr4, sizeof(serverAddr4)) == -1) {
            cout << "绑定失败，请检查监听ip和端口号" << endl;
            closesocket(serverSocket4);
            return;
        }
        if (listen(serverSocket4, SOMAXCONN) == -1) {
            cout << "监听失败，请检查系统及网络设置" << endl;
            closesocket(serverSocket4);
            return;
        }
    }
    if(ok6){
        sockaddr_in6 serverAddr6;
        serverAddr6.sin6_family = AF_INET6;
        serverAddr6.sin6_port = htons(stoi(info.portnum6));
        int len = sizeof(serverAddr6);
        WSAStringToAddressA((LPSTR)(info.acceptipnum6.c_str()), AF_INET6, NULL, (sockaddr*)&serverAddr6.sin6_addr, &len);
        if (::bind(serverSocket6, (struct sockaddr*)&serverAddr6, sizeof(serverAddr6)) == -1) {
            cout << "绑定失败，请检查监听ip和端口号" << endl;
            closesocket(serverSocket6);
            return;
        }
        if (listen(serverSocket6, SOMAXCONN) == -1) {
            cout << "监听失败，请检查系统及网络设置" << endl;
            closesocket(serverSocket6);
            return;
        }
    }
    if(id==-1){
        cout<<"服务器未在注册中心注册成功，请输入reregister重新注册"<<endl;
        //Sleep(1);
    }
    else{
        cout<<"服务器启动！"<<endl;
        //Sleep(1);
    }
    //创建线程
    thread ipv4thread(&Server::acceptclient,this,serverSocket4,ok4);
    thread ipv6thread(&Server::acceptclient6,this,serverSocket6,ok6);
    ipv4thread.detach();
    ipv6thread.detach();
}

void Server::acceptclient6(int sock,bool sign){
    if(!sign)return;
    //cout<<"ipv6线程启动"<<endl;
    while (true) {
        // 接受客户端连接
        sockaddr_in6 clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            cout << "接受连接失败" << endl;
            closesocket(sock);
            return;
        }
        // 创建新线程处理客户端请求
        thread clientThread(&Server::handleclient,this, clientSocket);
        clientThread.detach();  // 分离线程，使其在后台运行
    }
    closesocket(sock);
}

void Server::acceptclient(int sock,bool sign){
    if(!sign)return;
    //cout<<"ipv4线程启动"<<endl;
    while (true) {
        // 接受客户端连接
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            cout << "接受连接失败" << endl;
            closesocket(sock);
            return;
        }
        // 创建新线程处理客户端请求
        // 使用线程池？？
        thread clientThread(&Server::handleclient,this, clientSocket);
        clientThread.detach();  // 分离线程，使其在后台运行
    }
    closesocket(sock);
}


void Server::handleclient(int clientSocket) {
    // 接收客户端数据
    serverLock.lock();
    info.numofclients++;
    serverLock.unlock();
    string info;
    int bytesReceived = recvWithTimeout(clientSocket, info, -1);//-1表示可接收无限长的数据
    if(bytesReceived <=0){
        cout << "接收数据失败或超时" << endl;
        closesocket(clientSocket);
        return;
    }
    // 处理数据
    stringstream ss(info);
    string functionaddstr;
    ss >> functionaddstr;
    u64 functionadd = stoull(functionaddstr);
    SerializedData serializedData(info.substr(functionaddstr.size() + 1));
    Rpc_data data;
    data.deserialize(serializedData);
    Rpc_data (*func)(Rpc_data);
    func = (Rpc_data (*)(Rpc_data))functionadd;
    cout<<"对函数"<<(u64)func<<"进行调用"<<endl;
    cout<<"请求处理中"<<endl;
    Rpc_data result = func(data);
    SerializedData resultstr = result.serialize();
    // 发送结果给客户端
    cout<<"请求处理完成"<<endl;
    cout<<"正在将结果发送给客户端"<<endl;
    int bytesSent = sendAll(clientSocket, resultstr.c_str(), resultstr.size());
    if(bytesSent == -1){
        cout << "发送返回结果失败" << endl;
    }
    // 关闭客户端套接字
    closesocket(clientSocket);
    //加锁
    serverLock.lock();
    this->info.numofclients--;
    serverLock.unlock();
    //解锁
}


void Server::addfunction(string functionname,string functioninfo, vector<string> arglist, string returntype, u64 address){
    Function_info func;
    func.functioninfo=functioninfo;
    func.functionname = functionname;
    func.arglist = arglist;
    func.returntype = returntype;
    func.address = address;
    func.available=0;
    info.functionInfo.push_back(func);
}

void Server::heartbeats(){
    if(id==-1){
        cout<<"服务器未被注册，请注册服务器"<<endl;
        return;
    }
    // 创建客户端udp套接字，只需要ipv4
    sockaddr_in serverAddr;
    int clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == -1) {
        cout << "套接字创建失败" << endl;
        return;
    }
    // 设置服务器地址和端口
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(stoi(reg_port4));
    serverAddr.sin_addr.s_addr = inet_addr(reg_ipv4.c_str());
    int sign=0;
    while(1){
        // 发送心跳包
        string hbinfo = "Heartbeat " +to_string(info.create_time)+" "+to_string(info.numofclients);
        int bytesSent = sendto(clientSocket, hbinfo.c_str(), hbinfo.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (bytesSent == -1) {
            //cout << "发送数据失败" << endl;
            closesocket(clientSocket);
            return;
        }
        // 接收心跳回复
        // 设置超时
        struct timeval timeout = {5, 0};
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        char buffer[1024] = {0};
        int bytesReceived = recvfrom(clientSocket, buffer, 1024, 0, nullptr, nullptr);
        string response = buffer;
        if((response!="OKhb"||bytesReceived==-1)){
            if(sign==3){
                id=-1;
                cout<<"已经3次未收到心跳回复，服务器停止心跳"<<endl;
                cout<<"强烈建议重新注册"<<endl;
                return;
            }
            if(sign==0){
                cout<<"未收到心跳回复，服务器注册信息可能已在注册中心丢失"<<endl;
                cout<<"重新检测中...(请先不要重新注册)"<<endl;
            }
            sign++;
        }
        if(response=="OKhb"&&sign>0){
            cout<<"注册信息重检成功，可继续正常使用"<<endl;
            sign=0;
        }
        Sleep(5000);
    }
}

void Server::deadhb(){
    if(id==-1)return;
    sockaddr_in serverAddr;
    int clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == -1) {
        //cout << "套接字创建失败" << endl;
        return;
    }
    // 设置服务器地址和端口
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(stoi(reg_port4));
    serverAddr.sin_addr.s_addr = inet_addr(reg_ipv4.c_str());
    // 发送心跳包
    string hbinfo = "Dead " +to_string(info.create_time);
    int bytesSent = sendto(clientSocket, hbinfo.c_str(), hbinfo.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    closesocket(clientSocket);
    return;
}

void Server::showavailable(){
    cout<<"服务列表:"<<endl;
    for(int i=0;i<info.functionInfo.size();i++){
        cout<<info.functionInfo[i].functionname<<": "<<info.functionInfo[i].functioninfo<<endl;
        cout<<"是否注册在注册中心:";
        if(info.functionInfo[i].available==1){
            cout<<"是"<<endl;
        }
        else{
            cout<<"否"<<endl;
        }
    }
}

void Server::beavailableornot(string functionname,bool sign){
    if(id==-1){
        cout<<"服务器未注册，请注册服务器"<<endl;
        return;
    }
    for(int i=0;i<info.functionInfo.size();i++){
        if(info.functionInfo[i].functionname==functionname){
            if(info.functionInfo[i].available==sign){
                if(sign==1){
                    cout<<"函数"<<functionname<<"已经上线，无需操作"<<endl;
                }
                else{
                    cout<<"函数"<<functionname<<"已经下线，无需操作"<<endl;
                }
                return;
            }
            //通知注册中心
            //创建tcp套接字
            sockaddr_in serverAddr;
            int clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (clientSocket == -1) {
                cout << "套接字创建失败" << endl;
                return;
            }
            // 设置服务器地址和端口
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(stoi(reg_port4));
            serverAddr.sin_addr.s_addr = inet_addr(reg_ipv4.c_str());
            // 连接服务器
            if (connectWithTimeout(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr),10) == -1) {
                cout << "连接失败" << endl;
                closesocket(clientSocket);
                return;
            }
            // 发送注册信息给服务器
            string info = "Serverchange "+to_string(this->info.create_time)+" "+functionname;
            int bytesSent = sendAll(clientSocket, info.c_str(), info.size());
            if (bytesSent == -1) {
                cout << "发送注册信息失败" << endl;
                closesocket(clientSocket);
                return;
            }
            // 等待数据中心回复
            string buffer;
            int bytesReceived = recvWithTimeout(clientSocket, buffer, 100, 10);
            if (bytesReceived == -1) {
                cout << "接收注册中心返回数据失败，函数状态未改变" << endl;
                closesocket(clientSocket);
                return;
            }
            // 处理回复
            if(buffer[0]=='O'&&buffer[1]=='K'){
                if(sign==1){
                    cout<<"函数"<<functionname<<"上线成功"<<endl;
                }
                else{
                    cout<<"函数"<<functionname<<"下线成功"<<endl;
                }
                this->info.functionInfo[i].available=sign;
            }
            else{
                cout<<"接收到错误信息，函数状态未改变"<<endl;
            }
            return;
        }
    }
    cout<<"未找到该函数"<<endl;
    return;
}