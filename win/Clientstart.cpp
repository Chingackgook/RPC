#include"Client.h"
using namespace std;
Client client;

void showhelp(){
    cout<<"启动参数："<<endl;
    cout<<"-i [ip] 指定注册中心的ip地址"<<endl;
    cout<<"-p [port] 指定注册中心的端口号"<<endl;
    cout<<"-h 显示帮助"<<endl;
    cout<<"命令："<<endl;
    cout<<"help 显示帮助"<<endl;
    cout<<"exit/quit 退出"<<endl;
    cout<<"registerinfo 查看注册中心信息"<<endl;
    cout<<"checkall 查看所有服务"<<endl;
    cout<<"check 查看指定服务"<<endl;
    cout<<"call 调用服务"<<endl;
    cout<<"restart 重启客户端"<<endl;
}

Rpc_data getfiledata(){
    cout<<"请输入文件路径："<<endl;
    string filename;
    cin.ignore();
    getline(cin,filename);
    ifstream infile;
    infile.open(filename,ios::binary|ios::in);
    if(!infile.is_open()){
        cout<<"文件打开失败"<<endl;
        Rpc_data result;
        result.strings.push_back("文件打开失败");
        return result;
    }
    vector<int> data;
    char c;
    while(infile.get(c)){
        data.push_back((int)c);
    }
    infile.close();
    Rpc_data result;
    result.strings.push_back(filename);
    result.ints=data;
    return result;
}

Rpc_data handtoRpc_Manual(handle hand){
    cout<<endl<<"函数说明："<<endl;
    cout<<hand.funcinfo<<endl;
    vector<string> strings;
    vector<int> ints;
    vector<double> doubles;
    vector<char> chars;
    int numofarg=hand.arglist.size();
    int totla=1;
    bool cinsign=1;
    for(int i=0;i<numofarg;i++){
        int siglearg=0;
        int j;
        for(j=0;j<hand.arglist[i].size();j++){
            if(hand.arglist[i][j]>='0'&&hand.arglist[i][j]<='9'){
                siglearg*=10;
                siglearg+=(int)hand.arglist[i][j]-'0';
            }
            else{
                break;
            }
        }//1*file
        string argtype=hand.arglist[i].substr(j+1);
        string argvalue;
        for(int k=0;k<siglearg;k++){
            if(argtype=="file"){
                Rpc_data filedata=getfiledata();
                return filedata;
            }
            if(argtype=="string"){
                cout<<"请输入第"<<totla<<"个"<<argtype<<"类型的参数:"<<endl;
                //可输入整行数据，可能包括空格
                if(cinsign){
                    cin.ignore();
                }
                getline(cin,argvalue);
                cinsign=0;
                strings.push_back(argvalue);
                argvalue="";
                totla++;
                continue;
            }
            cout<<"请输入第"<<totla<<"个"<<argtype<<"类型的参数:"<<endl;
            cin>>argvalue;
            if(argtype=="int"){
                ints.push_back(stoi(argvalue));
            }
            else if(argtype=="double"||argtype=="float"){
                doubles.push_back(stod(argvalue));
            }
            else if(argtype=="char"){
                chars.push_back(argvalue[0]);
            }
            cinsign=1;
            totla++;
        }
    }
    Rpc_data rpcresult;
    rpcresult.strings=strings;
    rpcresult.ints=ints;
    rpcresult.doubles=doubles;
    rpcresult.chars=chars;
    return rpcresult;
}

bool recieveFilefromServer(Rpc_data data){
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
                cout<<filename<<endl;
            filename=name+"("+to_string(count)+")"+back;
            count++;
        }
        else{
            break;
        }
    }
    ofstream outfile;
    outfile.open(filename,ios::binary|ios::out);
    if(!outfile.is_open()){
        return 0;
    }
    for(int i=0;i<datas.size();i++){
        char c=(char)datas[i];
        outfile.write(&c,1);
    }
    outfile.close();
    return 1;
}

int main(int argc,char* argv[]){
    initializeWinsock();
    bool ipok=0,portok=0;
    string regip="127.0.0.1";
    string regport="11111";
    string selfport="33333";
    int reg_iptype=4;
    if(argc==1){//默认启动，仅供测试
        goto defaultstart;
        return 0;
    }
    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'&&argv[i][1]=='h'){
            showhelp();
        }
        else if(argv[i][1]=='i'&&argv[i][0]=='-'){
            if(i+1>=argc){
                cout<<"启动参数错误"<<endl;
                return 0;
            }
            ipok=1;
            regip=argv[i+1];
            reg_iptype=getiptype(regip);
        }
        else if(argv[i][1]=='p'&&argv[i][0]=='-'){
            if(i+1>=argc){
                cout<<"启动参数错误"<<endl;
                return 0;
            }
            portok=1;
            regport=argv[i+1];
        }
    }
    defaultstart:
    if(!ipok){
        cout<<"请指定注册中心的ip地址"<<endl;
        cin>>regip;
        reg_iptype=getiptype(regip);
    }
    if(!portok){
        cout<<"请指定注册中心的端口号"<<endl;
        cin>>regport;
    }
    while(!isgoodport(regport)){
        cout<<"端口号不合法，请重新输入"<<endl;
        cin>>regport;
    }
    if(client.init(regip,regport,reg_iptype)){
        cout<<"客户端启动成功！"<<endl;
        cout<<"注册中心地址："<<regip<<" 端口号："<<regport<<endl;
    }
    else{
        cout<<"客户端未连接到注册中心，请重启！"<<endl;
    }
    string command;
    cout<<endl<<"输入help以查看可用命令"<<endl;
    while(1){
        cout<<endl;
        cin>>command;
        if(command=="help"){
            showhelp();
        }
        else if(command=="restart"){
            cout<<"请重新指定注册中心的ip地址"<<endl;
            cin>>regip;
            reg_iptype=getiptype(regip);
            cout<<"请重新指定注册中心的端口号"<<endl;
            cin>>regport;
            while(!isgoodport(regport)){
                cout<<"端口号不合法，请重新输入"<<endl;
                cin>>regport;
            }
            if(client.init(regip,regport,reg_iptype)){
                cout<<"客户端启动成功！"<<endl;
                cout<<"注册中心地址："<<regip<<" 端口号："<<regport<<endl;
            }
            else{
                cout<<"客户端未连接到注册中心，请重试！"<<endl;
            }
        }
        else if(command=="exit"||command=="quit"){
            break;
        }
        else if(command=="checkall"){
            string serveinfo = client.checkServer();
            cout<<serveinfo<<endl;
        }
        else if(command=="check"){
            string sevicename;
            cout<<"请输入服务名："<<endl;
            cin>>sevicename;
            cout<<""; //完全不知道为什么，没有这一行，ipv6套接字就会报连接失败或超时
            string serveinfo = client.checkServer(sevicename);
            cout<<serveinfo<<endl;
        }
        else if(command=="call"){
            string sevicename;
            cout<<"请输入服务名："<<endl;
            cin>>sevicename;
            cout<<"是否手动指定服务器？(y/n)"<<endl;
            string c;
            cin>>c;
            string ip="";
            cout<<"";//同上
            if(c=="y"){
                cout<<"请输入服务器ip地址："<<endl;
                cin>>ip;
                cout<<"";//依然同上面的原因
            }
            handle hand = client.findServer(sevicename,ip);//ip为空则自动选择服务器
            if(hand.ser_ipnum=="fail"){
                cout<<"调用服务失败！"<<endl;
                cout<<"错误信息："<<hand.ser_ipnum6<<endl;
                continue;
            }
            Rpc_data data=handtoRpc_Manual(hand);
            cout<<"服务器处理中"<<endl;
            Rpc_data result=client.callServer(hand,data);
            if(result.chars.size()==5&&result.chars[0]=='~'&&result.chars[1]=='f'&&result.chars[2]=='i'&&result.chars[3]=='l'&&result.chars[4]=='e'){
                cout<<"服务器返回文件"<<endl;
                if(!recieveFilefromServer(result)){
                    cout<<"文件接收失败"<<endl;
                }
                cout<<"文件接收成功"<<endl;
                continue;
            }
            cout<<"调用结果："<<endl;
            result.show();
        }
        else if(command=="registerinfo"){
            cout<<"注册中心地址："<<regip<<" 端口号："<<regport<<endl;
        }
        else{
            cout<<"未知命令，输入help以查看帮助"<<endl;
        }
    }
    cout<<"感谢使用！"<<endl;
    cleanupWinsock();
    Sleep(3000);
    return 0;
}