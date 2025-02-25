#include"Server.h"
#include<math.h>
using namespace std;

Server server;

void showhelp(){
    cout<<"启动参数："<<endl;
    cout<<"-l [ip] 指定服务器监听的ip地址"<<endl;
    cout<<"-p [port] 指定服务器监听的端口号"<<endl;
    cout<<"-h 显示帮助"<<endl;
    cout<<"命令："<<endl;
    cout<<"help 显示帮助"<<endl;
    cout<<"exit/quit 退出"<<endl;
    cout<<"register 注册函数"<<endl;
    cout<<"unregister 注销函数"<<endl;
    cout<<"reregister 重新注册服务器"<<endl;
    cout<<"show 查看可用服务"<<endl;
    cout<<"showself 查看服务器自身信息"<<endl;
}

vector<string> stringtovec(string str){
    vector<string> result;
    stringstream ss(str);
    string temp;
    while(ss >> temp){
        result.push_back(temp);
    }
    return result;
}

Rpc_data senfile(Rpc_data data){
    if(data.strings.size()==0||data.strings[0]=="文件打开失败"){
        Rpc_data result;
        result.strings.push_back("文件打开失败，请检查文件路径是否正确，及管理员权限");
        return result;
    }
    string filename=data.strings[0];
    vector<int> datas=data.ints;
    int sub=-1;
     for (int i = 0; i < filename.length(); i++) {
        if (filename[i] == '.') {
            sub = i;
            break;
        }
    }
    if(sub==-1){
        sub=filename.length();
    }
    string name = filename.substr(0, sub);
    string back = filename.substr(name.length());
    filename=name+back;
    fstream file;
    int count=1;
    while(1){
        file.open(filename,ios::in);
        if(file.is_open()){
            file.close();
            filename=name+"("+to_string(count)+")"+back;
            count++;
        }
        else{
            break;
        }
    }
    ofstream outfile;
    outfile.open(filename,ios::binary|ios::out);
    for(int i=0;i<datas.size();i++){
        char c=(char)datas[i];
        outfile.write(&c,1);
    }
    outfile.close();
    Rpc_data result;
    result.strings.push_back("文件接收成功");
    return result;
}

Rpc_data designatedtime(Rpc_data data){
    int thetime=data.ints[0];
    Sleep(thetime*1000);
    Rpc_data result;
    result.strings.push_back("服务器处理"+to_string(thetime)+"秒");
    return result;
}

Rpc_data add(Rpc_data data){
    double a,b;
    a=data.doubles[0];
    b=data.doubles[1];
    Rpc_data result;
    result.doubles.push_back(a+b);
    return result;
}

Rpc_data bigintplus(Rpc_data data){
    string a,b;
    a=data.strings[0];
    b=data.strings[1];
    int lena=a.length();
    int lenb=b.length();
    int len=max(lena,lenb);
    string result="";
    int carry=0;
    for(int i=0;i<len;i++){
        int numa=0;
        int numb=0;
        if(i<lena){
            numa=a[lena-i-1]-'0';
        }
        if(i<lenb){
            numb=b[lenb-i-1]-'0';
        }
        int temp=numa+numb+carry;
        carry=temp/10;
        temp%=10;
        result+=to_string(temp);
    }
    if(carry!=0){
        result+=to_string(carry);
    }
    reverse(result.begin(),result.end());
    Rpc_data res;
    res.strings.push_back(result);
    return res;
}

Rpc_data String_reverse(Rpc_data data){
    string temp=data.strings[0];
    reverse(temp.begin(),temp.end());
    Rpc_data result;
    result.strings.push_back(temp);
    return result;
}

Rpc_data testString(Rpc_data data){
    //int int string string
    int a,b;
    string c,d;
    a=data.ints[0];
    b=data.ints[1];
    c=data.strings[0];
    d=data.strings[1];
    Rpc_data result;
    result.strings.push_back("两数相加为"+to_string(a+b)+" 字符串1:"+c+" 字符串2:"+d);
    return result;
}

Rpc_data sub(Rpc_data data){
    double a,b;
    a=data.doubles[0];
    b=data.doubles[1];
    Rpc_data result;
    result.doubles.push_back(a-b);
    return result;
}

Rpc_data matrix5_5(Rpc_data data){
    vector<vector<int>> a(5,vector<int>(5,0));
    for(int i=0;i<5;i++){
        for(int j=0;j<5;j++){
            a[i][j]=data.ints[i*5+j];
        }
    }
    int res=0;
    for(int i=0;i<5;i++){
        int temp=1;
        for(int j=0;j<5;j++){
            temp*=a[j][(i+j)%5];
        }
        res+=temp;
    }
    Rpc_data result;
    result.ints.push_back(res);
    return result;
}

Rpc_data quadratic(Rpc_data data){
    double a,b,c;
    a=data.doubles[0];
    b=data.doubles[1];
    c=data.doubles[2];
    double delta=b*b-4*a*c;
    Rpc_data result;
    if(delta<0){
        result.strings.push_back("无实数解");
        return result;
    }
    else if(delta==0){
        result.strings.push_back("有一个实数解");
        result.doubles.push_back(-b/(2*a));
        return result;
    }
    else{
        result.strings.push_back("有两个实数解");
        result.doubles.push_back((-b+sqrt(delta))/(2*a));
        result.doubles.push_back((-b-sqrt(delta))/(2*a));
        return result;
    }
}

int main(int argc,char* argv[]){
    initializeWinsock();
    bool ipv6sign=0;
    bool ip4ok=1;
    bool ip6ok=1;
    string ipv4,ipv6;
    ipv4=getLocalIPAddress();//获取本机的一个ip地址，如果监听端口号是0.0.0.0
    ipv6=getLocalIPv6Address();//则可以使用这个ip注册到注册中心
    string listenip="0.0.0.0";
    string listenip6="IN6ADDR_ANY";
    string tempinip="";
    string porttemp="-1";
    string port4="22222";
    string port6="22223";
    if(argc==1){//默认启动，仅供测试
        goto defaultstart;
    }
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'&&argv[i][1]=='h'){
            showhelp();
        }
        else if(argv[i][1]=='l'&&argv[i][0]=='-'){
            if(i+1>=argc){
                cout<<"启动参数错误"<<endl;
                return 0;
            }
            tempinip=argv[i+1];
            if(getiptype(tempinip)==6){
                if(tempinip=="::"){
                    tempinip="IN6ADDR_ANY";
                }
                ipv6sign=1;
            }
        }
        else if(argv[i][1]=='p'&&argv[i][0]=='-'){
            if(i+1>=argc){
                cout<<"启动参数错误"<<endl;
                return 0;
            }
            porttemp=argv[i+1];
        }
    }
    if(porttemp=="-1"){
        cout<<"请指定端口号"<<endl;
        cin>>porttemp;
    }
    if(ipv6sign){
        if(tempinip=="")tempinip="IN6ADDR_ANY";
        listenip6=tempinip;
        port6=porttemp;
        vector<string> temp=askforanother(6);
        if(temp[0]=="1"){
            listenip=temp[1];
            port4=temp[2];
        }
        else{
            ip4ok=0;
            listenip="不支持";
            port4="不支持";
            ipv4="不支持";
        }
    }
    else{
        if(tempinip=="")tempinip="0.0.0.0";
        listenip=tempinip;
        port4=porttemp;
        vector<string> temp=askforanother(4);
        if(temp[0]=="1"){
            listenip6=temp[1];
            port6=temp[2];
        }
        else{
            ip6ok=0;
            listenip6="不支持";
            port6="不支持";
            ipv6="不支持";
        }
    }
    defaultstart://默认启动,仅供测试
    time_t seconds;
    seconds = time(NULL);
    while(isgoodport(port4)==0){
        cout<<"ipv4端口号不合法，请重新输入"<<endl;
        cin>>port4;
    }
    while(isgoodport(port6)==0){
        cout<<"ipv6端口号不合法，请重新输入"<<endl;
        cin>>port6;
    }
    //cout<<listenip<<" "<<listenip6<<" "<<port4<<port6<<" "<<ipv4<<" "<<ipv6<<" "<<seconds<<endl;
    server.init(listenip,listenip6,port4,port6,ipv4,ipv6,(u64)seconds);
    cout<<"服务器初始化成功！"<<endl;
    server.addfunction("add","两数相加",stringtovec("1*double 1*double"),"double",(u64)add);
    server.addfunction("sub","两数相减",stringtovec("2*float"),"double",(u64)sub);
    server.addfunction("matrix5_5","计算5*5的行列式值",stringtovec("25*int"),"int",(u64)matrix5_5);
    server.addfunction("String_reverse","反转字符串",stringtovec("1*string"),"string",(u64)String_reverse);
    server.addfunction("bigintplus","大整数相加",stringtovec("2*string"),"string",(u64)bigintplus);
    server.addfunction("quadratic","解一元二次方程，依次输入a,b,c",stringtovec("3*float"),"double&string",(u64)quadratic);
    server.addfunction("designatedtime","服务器处理指定时间",stringtovec("1*int"),"string",(u64)designatedtime);
    server.addfunction("testString","测试处理带空格的字符串",stringtovec("1*int 1*int 2*string"),"string",(u64)testString);
    server.addfunction("sendfile","接收文件",stringtovec("1*file"),"string",(u64)senfile);
    string reg_ip="127.0.0.1",reg_port="11111";
    cout<<"请指定注册中心的ip地址："<<endl;
    cin>>reg_ip;
    cout<<"请指定注册中心的端口号："<<endl;
    cin>>reg_port;
    while(!isgoodport(reg_port)){
        cout<<"端口号不合法，请重新输入"<<endl;
        cin>>reg_port;
    }
    //cout<<"注册中心的ip地址为："<<reg_ip<<" 端口号为："<<reg_port<<reg_type<<endl;
    server.regist(reg_ip,reg_port,getiptype(reg_ip));
    server.readyToServe(ip4ok,ip6ok);
    if(server.id!=-1)cout<<"请手动输入命令register，以注册服务函数"<<endl;
    string command;
    cout<<endl<<"输入help以查看可用命令"<<endl;
    while(1){
        cout<<endl;
        cin>>command;
        if(command=="help"){
            showhelp();
        }
        else if(command=="exit"||command=="quit"){
            server.deadhb();
            break;
        }
        else if(command=="reregister"){
            reg_ip="";reg_port="";
            cout<<"请重新指定注册中心的ip地址："<<endl;
            cin>>reg_ip;
            cout<<"请重新指定注册中心的端口号："<<endl;
            cin>>reg_port;
            while(!isgoodport(reg_port)){
                cout<<"端口号不合法，请重新输入"<<endl;
                cin>>reg_port;
            }
            server.regist(reg_ip,reg_port,getiptype(reg_ip));
        }
        else if(command=="show"){
            server.showavailable();
        }
        else if(command=="register"){
            string name;
            cout<<"请输入函数名："<<endl;
            cin>>name;
            server.beavailableornot(name,1);
        }
        else if(command=="unregister"){
            string name;
            cout<<"请输入函数名："<<endl;
            cin>>name;
            server.beavailableornot(name,0);
        }
        else if(command=="showself"){
            cout<<"服务器监听的ipv4地址："<<listenip<<" 端口号："<<port4<<endl;
            if(listenip=="0.0.0.0"){
                cout<<"可使用："<<ipv4<<"访问"<<endl;
            }
            cout<<"服务器监听的ipv6地址："<<listenip6<<" 端口号："<<port6<<endl;
            cout<<"服务器注册在"<<reg_ip<<" 端口号："<<reg_port<<endl;
            if(listenip6=="IN6ADDR_ANY"){
                cout<<"可使用："<<ipv6<<"访问"<<endl;
            }
        }
        else{
            cout<<"未知命令，输入help以查看帮助"<<endl;
        }
    }
    cout<<"服务器已关闭，感谢使用"<<endl;
    cleanupWinsock();
    Sleep(1000);
    return 0;
}