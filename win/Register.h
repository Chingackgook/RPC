#include"RPC.h"
using namespace std;

class Register {
public:
    Register();

    //初始化注册中心，包括监听的IPv4地址和端口号，监听的IPv6地址和端口号
    void init(string portnum,string portnum6,string listenipnum,string listenipnum6);

    //准备开始提供服务
    void readyToServe(bool ipv4=1,bool ipv6=1);

    //监听IPv4的客户端连接
    void readytohear(int sock,bool ok);

    //监听IPv6的客户端连接
    void readytohear6(int sock,bool ok);
    
    //处理客户端请求
    void handleclient(int sock);

    //处理服务器注册信息，将服务器信息加入到serverList中
    string addServer(string serverinfo);

    //向客户端展示服务器信息，如果functionname为空，则展示所有服务器信息
    string showtoClient(string functionname="");

    //客户端准备调用服务，需要得到必要的服务器信息（包括ip地址，端口号，函数地址，参数表）
    //如果ipnum为空，则按负载均衡选择服务器
    string getServer(string functionname,string ipnum="");

    //监听服务器的心跳
    void hearingheartbeat();

    //处理心跳信息，并将回应发送给服务器
    string handleheartbeat(string heartbeatinfo);

    //计时器，用于检测服务器是否在线
    void resttimecount();

    //展示注册中心自身信息
    void showself(bool ok4,bool ok6);

    //得到服务器数量
    int getnumofserver();
private:
    shared_mutex registerLock;//读写锁
    string listenipnum;//监听的IPv4地址
    string listenipnum6;//监听的IPv6地址
    string portnum;//监听的端口号
    string portnum6;//监听的端口号
    map<int,int> dic;//服务器发来的是创建时间，为减少查询时间，使用map将创建时间映射到服务器ID
    vector<Server_info> serverList;//服务器列表
    vector<int> resttime;//服务器剩余时间
};


Register::Register(){}


void Register::init(string portnum,string portnum6,string listenipnum,string listenipnum6){
    this->listenipnum = listenipnum;
    this->listenipnum6 = listenipnum6;
    this->portnum6 = portnum6;
    this->portnum = portnum;
}

void Register::showself(bool ok4,bool ok6){
    if(ok4){
        cout<<"监听的IPv4地址为："<<listenipnum<<endl;
        if(listenipnum=="0.0.0.0"){
            cout<<"可使用："<<getLocalIPAddress()<<"访问"<<endl;
        }
        cout<<"监听的端口号为："<<portnum<<endl;
    }
    if(ok6){
        cout<<"监听的IPv6地址为："<<listenipnum6<<endl;
        if(listenipnum6=="::"||listenipnum6=="IN6ADDR_ANY"){
            cout<<"可使用"<<getLocalIPv6Address()<<"访问"<<endl;
        }
        cout<<"监听的端口号为："<<portnum6<<endl;
    }
}

void Register::readyToServe(bool ipv4,bool ipv6){
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int serverSocket6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    showself(ipv4,ipv6);
    if(ipv4){
        if (serverSocket == -1 || serverSocket6 == -1) {
            cout << "套接字创建失败" << endl;
            return;
        }
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(stoi(portnum));
        serverAddr.sin_addr.s_addr = inet_addr(listenipnum.c_str());
        if (::bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            cout << "绑定失败，请检查ipv4地址和端口号是否正确" << endl;
            closesocket(serverSocket);
            return;
        }
        if (listen(serverSocket, SOMAXCONN) == -1) {
            cout << "监听失败" << endl;
            closesocket(serverSocket);
            return;
        }
    }
    if(ipv6){
        sockaddr_in6 serverAddr6;
        serverAddr6.sin6_family = AF_INET6;
        serverAddr6.sin6_port = htons(stoi(portnum6));
        int len=sizeof(serverAddr6);
        int resut=WSAStringToAddressA((LPSTR)(listenipnum6.c_str()), AF_INET6, NULL, (sockaddr*)&serverAddr6.sin6_addr, &len);
        //serverAddr6.sin6_addr = in6addr_any;
        if (::bind(serverSocket6, (struct sockaddr*)&serverAddr6, sizeof(serverAddr6)) == SOCKET_ERROR) {
            //cout<<WSAGetLastError();
            cout << "绑定失败，请检查ipv6地址和端口号是否正确" << endl;
            closesocket(serverSocket6);
            return;
        }
        if (listen(serverSocket6, SOMAXCONN) == -1) {
            cout << "监听失败" << endl;
            closesocket(serverSocket6);
            return;
        }
    }
    thread t1(&Register::readytohear,this,serverSocket,ipv4);
    thread t2(&Register::readytohear6,this,serverSocket6,ipv6);
    thread t3(&Register::hearingheartbeat,this);
    thread t4(&Register::resttimecount,this);
    cout<<"注册中心初始化成功，欢迎使用！"<<endl;
    t1.detach();
    t2.detach();
    t3.detach();
    t4.detach();
}

void Register::readytohear(int sock,bool ok){
    // 接受连接并提供服务
    if(!ok)return;
    while (true) {
        // 接受客户端连接
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            cout << "接受连接失败4" << endl;
            closesocket(sock);
            return;
        }
        // 创建新线程处理客户端请求
        thread clientThread(&Register::handleclient,this, clientSocket);
        clientThread.detach();  // 分离线程，使其在后台运行
    }
    // 关闭服务器套接字
    closesocket(sock);
}

void Register::readytohear6(int sock,bool ok){
    if(!ok)return;
    // 接受连接并提供服务
    while (true) {
        // 接受客户端连接
        sockaddr_in6 clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == -1) {
            cout << "接受连接失败6" << endl;
            closesocket(sock);
            return;
        }
        // 创建新线程处理客户端请求
        thread clientThread(&Register::handleclient,this, clientSocket);
        clientThread.detach();  // 分离线程，使其在后台运行
    }
    // 关闭服务器套接字
    closesocket(sock);
}

void Register::handleclient(int clientSocket){
    string info;
    int bytesReceived = recvWithTimeout(clientSocket, info, 102400);
    if (bytesReceived == -1) {
        cout << "接收数据失败" << endl;
        closesocket(clientSocket);
        return;
    }
    stringstream ss(info);
    string type;
    ss >> type;
    string reply;
    if (type == "Server") {
        cout<<"收到服务器注册信息... ";
        string serverinfo;
        getline(ss, serverinfo);
        reply = addServer(serverinfo);
        stringstream ss(reply);
        string null;
        int id;
        ss>>null>>id;
        cout<<"注册成功，服务器ID为"<<id<<endl;
    } 
    else if(type =="Serverchange"){
        int createtime;
        ss>>createtime;
        int id=dic[createtime];
        string functionname;
        ss>>functionname;
        unique_lock<shared_mutex> lock(registerLock);
        for(int i=0;i<serverList[id].functionInfo.size();i++){
            if(serverList[id].functionInfo[i].functionname==functionname){
                if(serverList[id].functionInfo[i].available){
                    serverList[id].functionInfo[i].available=0;
                    cout<<"服务器"<<id<<"的函数"<<functionname<<"已下线"<<endl;
                }
                else{
                    serverList[id].functionInfo[i].available=1;
                    cout<<"服务器"<<id<<"的函数"<<functionname<<"已上线"<<endl;
                }
                break;
            }
        }
        reply="OK";
    }
    else {
        ss >> type;
        if(type=="connect"){
            return;
        }
        //cout<<"收到客户端请求"<<type<<endl;
        if (type == "all") {
            reply = showtoClient();
        } 
        else if(type == "check"){
            string functionname;
            ss>>functionname;
            reply = showtoClient(functionname);
        }
        else {
            string ipsign,functionname;
            ss>>ipsign;
            if (ipsign == "withip"){
                string ipnum;
                ss>>functionname>>ipnum;
                reply = getServer(functionname, ipnum);
            }
            else{
                ss>>functionname;
                reply = getServer(functionname);
            }
        }
    }
    //cout<<reply<<endl;
    // 发送回复给客户端
    int bytesSent = sendAll(clientSocket, reply.c_str(), reply.size());
    if (bytesSent == -1) {
        cout << "发送回复失败" << endl;
    }
    // 关闭客户端套接字
    closesocket(clientSocket);
}


string Register::addServer(string serverinfo){
    //将服务器信息加入到serverList中
    stringstream ss(serverinfo);
    Server_info server;
    ss>>server.acceptipnum;
    ss>>server.acceptipnum6;
    ss>>server.portnum;
    ss>>server.portnum6;
    ss>>server.selfipnum;
    ss>>server.selfipnum6;
    int numoffunction;
    ss>>numoffunction;
    for(int i=0;i<numoffunction;i++){
        Function_info function;
        ss>>function.functionname;
        ss>>function.functioninfo;
        int numofargs;
        ss>>numofargs;
        for(int j=0;j<numofargs;j++){
            string arg;
            ss>>arg;
            function.arglist.push_back(arg);
        }
        ss>>function.returntype;
        ss>>function.available;
        ss>>function.address;
        server.functionInfo.push_back(function);
    }
    ss>>server.numofclients;
    ss>>server.create_time;
    //因为是多线程，故需要加锁
    int id=0;
    bool findemptysign=0;
    unique_lock<shared_mutex> lock(registerLock);
    for(int i=0;i<serverList.size();i++){
        if(serverList[i].create_time==-1){
            id=i;
            findemptysign=1;
            break;
        }
    }
    if(findemptysign==0){
        serverList.push_back(server);
        id=serverList.size()-1;
        dic[server.create_time]=id;
        resttime.push_back(20);
    }
    else{
        //cout<<"找到空位"<<id<<endl;
        dic[server.create_time]=id;
        serverList[id]=server;
        resttime[id]=20;
    }
    //返回注册成功信息，并返回当前服务器ID
    string goodip=listenipnum=="0.0.0.0"?getLocalIPAddress():listenipnum;
    return "OK "+to_string(id)+" "+goodip+" "+portnum;//把注册中心的IPv4地址和端口号返回给服务器，用于心跳
}

string Register::showtoClient(string functionname){
    //将serverList中的信息返回给客户端
    bool allsign=0;
    int num=0;
    if(functionname==""){
        allsign=1;
    }
    string reply;
    shared_lock<shared_mutex> lock(registerLock);
    for(int i=0;i<serverList.size();i++){
        if(serverList[i].create_time==-1){
            continue;
        }
        num++;
        string temp="";
        int funcnum=0;
        for(int j=0;j<serverList[i].functionInfo.size();j++){
            if(!allsign&&serverList[i].functionInfo[j].functionname!=functionname){
                continue;
            }
            if(!serverList[i].functionInfo[j].available){
                continue;
            }
            funcnum++;
            if(allsign){
                temp+="第"+to_string(funcnum)+"个函数信息如下：\n";
                temp += "函数名："+serverList[i].functionInfo[j].functionname + "\n";
                temp += "函数描述："+serverList[i].functionInfo[j].functioninfo + "\n";
            }
            temp+="参数：\n";
            for(int k=0;k<serverList[i].functionInfo[j].arglist.size();k++){
                temp += serverList[i].functionInfo[j].arglist[k] + " ";
            }
            temp += "\n返回值类型："+serverList[i].functionInfo[j].returntype + "\n";
        }
        if(temp==""){
            num--;
            continue;
        }
        reply += "ID："+to_string(i)+"服务器信息如下：\n";
        reply += "该服务器监听的IPv4："+serverList[i].acceptipnum + "\n";
        if(serverList[i].acceptipnum=="0.0.0.0"){
            reply +="可使用："+serverList[i].selfipnum+"访问\n";
        }
        reply += "该服务器监听的IPv6："+serverList[i].acceptipnum6 + "\n";
        if(serverList[i].acceptipnum6=="::"||serverList[i].acceptipnum6=="IN6ADDR_ANY"){
            reply +="可使用："+serverList[i].selfipnum6+"访问\n";
        }
        reply += "IPv4端口号：" + serverList[i].portnum + "\n";
        reply += "IPv6端口号：" + serverList[i].portnum6 + "\n";
        if(allsign)reply += "可使用函数个数："+to_string(funcnum) + "\n";
        reply+=temp;
        reply += "在此服务器上运行的客户端个数："+to_string(serverList[i].numofclients) + "\n\n";
    }
    if(allsign&&num!=0)reply+="共有"+to_string(num)+"个可用服务器";
    if(reply==""){
        return "当前没有服务可用";
    }
    return reply;
}



string Register::getServer(string functionname,string ipnum){
    shared_lock<shared_mutex> lock(registerLock);
    if(ipnum==""){
        vector<Server_info> avaliableServer;
        for(int i=0;i<serverList.size();i++){
            if(serverList[i].create_time==-1){
                continue;
            }
            for(int j=0;j<serverList[i].functionInfo.size();j++){
                if(serverList[i].functionInfo[j].available==0){
                    continue;
                }
                if(serverList[i].functionInfo[j].functionname == functionname){
                    avaliableServer.push_back(serverList[i]);
                    break;
                }
            }
        }
        //负载均衡方法：选择负载最小的服务器，若有多个负载相同的服务器，则随机选择一个
        sort(avaliableServer.begin(),avaliableServer.end(),[](Server_info a,Server_info b){return a.numofclients<b.numofclients;});
        if(avaliableServer.size()==0)return "Function not found";
        int bestcount=0;
        int thebest=avaliableServer[0].numofclients;
        for(int i=0;i<avaliableServer.size();i++){
            if(avaliableServer[i].numofclients!=thebest)break;
            bestcount++;
        }
        srand(time(0));
        int id=rand()%bestcount;
        string reply=avaliableServer[id].selfipnum+" "+avaliableServer[id].selfipnum6 +" ";
        reply+=avaliableServer[id].portnum + " " +avaliableServer[id].portnum6;
        for(int j=0;j<avaliableServer[id].functionInfo.size();j++){
            if(avaliableServer[id].functionInfo[j].functionname == functionname){
                reply+=" "+to_string(avaliableServer[id].functionInfo[j].address);
                reply+=" "+to_string(avaliableServer[id].functionInfo[j].arglist.size());
                for(int k=0;k<avaliableServer[id].functionInfo[j].arglist.size();k++){
                    reply+=" "+avaliableServer[id].functionInfo[j].arglist[k];
                }
                reply+=" "+avaliableServer[id].functionInfo[j].functioninfo;
                break;
            }
        }
        return reply;
    }
    else{
        for(int i=0;i<serverList.size();i++){
            if(serverList[i].create_time==-1){
                continue;
            }
            if(serverList[i].selfipnum == ipnum||serverList[i].selfipnum6 == ipnum){
                for(int j=0;j<serverList[i].functionInfo.size();j++){
                    if(serverList[i].functionInfo[j].available==0){
                        continue;
                    }
                    if(serverList[i].functionInfo[j].functionname == functionname){
                        string reply=serverList[i].selfipnum +" "+serverList[i].selfipnum6+" "+ serverList[i].portnum + " "+serverList[i].portnum6;
                        reply+=" "+to_string(serverList[i].functionInfo[j].address);
                        reply+=" "+to_string(serverList[i].functionInfo[j].arglist.size());
                        for(int k=0;k<serverList[i].functionInfo[j].arglist.size();k++){
                            reply+=" "+serverList[i].functionInfo[j].arglist[k];
                        }
                        reply+=" "+serverList[i].functionInfo[j].functioninfo;
                        return reply;
                    }
                }
            }
        }
        return "Function not found";
    }
}

void Register::hearingheartbeat(){
    //监听心跳,使用udp套接字
    //只需要一个ipv4的套接字
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        cout << "套接字创建失败" << endl;
        return;
    }
    // 设置服务器地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(stoi(portnum));
    serverAddr.sin_addr.s_addr = inet_addr(listenipnum.c_str());
    // 绑定服务器地址和端口
    if (::bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        cout << "绑定失败" << endl;
        closesocket(sock);
        return;
    }
    // 接受连接并提供服务
    while (true) {
        // 接受客户端连接
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        char buffer[1024] = {0};
        int bytesReceived = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)&clientAddr, &clientAddrSize);
        // 下面代码跑的挺快的，不需要多线程
        if (bytesReceived == -1) {
            continue;
        }
        string heartbeatinfo = buffer;
        string reply=handleheartbeat(heartbeatinfo);
        if(reply=="okDead"){
            continue;
        }
        //将回复发送给客户端
        int bytesSent = sendto(sock, reply.c_str(), reply.size(), 0, (struct sockaddr*)&clientAddr, clientAddrSize);
    }
}

string Register::handleheartbeat(string heartbeatinfo){
    string infotype="";
    stringstream ss(heartbeatinfo);
    ss>>infotype;
    if(infotype=="Dead"){
        int createtime;
        ss>>createtime;
        int id=dic[createtime];
        resttime[id]=0;
        return "OKdead";
    }
    else if(infotype!="Heartbeat"){
        return "Fail";
    }
    string thetime;
    ss>>thetime;
    int createtime=stoi(thetime);
    if(dic.count(createtime)==0){
        return "Fail";
    }
    int idnum=dic[createtime];
    string clientnum;
    ss>>clientnum;
    serverList[idnum].numofclients=stoi(clientnum);
    resttime[idnum]=20;
    return "OKhb";
}

void Register::resttimecount(){
    while(true){
        //每隔二秒将resttime中的时间减二
        for(int i=0;i<resttime.size();i++){
            if(serverList[i].create_time==-1){
                continue;
            }
            resttime[i]-=2;
            if(resttime[i]<=0){
                //如果时间为0，则将该服务器从serverList中删除
                cout<<"服务器"<<" id:"<<i<<"已下线"<<endl;
                serverList[i]=Server_info();//将ServerList标志为死
                //删除字典中的信息
                for(auto it=dic.begin();it!=dic.end();it++){
                    if(it->second==i){
                        dic.erase(it);
                        break;
                    }
                }
            }
        }
        Sleep(2000);
    }
}

int Register::getnumofserver(){
    int num=0;
    shared_lock<shared_mutex> lock(registerLock);
    for(int i=0;i<serverList.size();i++){
        if(serverList[i].create_time!=-1){
            num++;
        }
    }
    return num;
}