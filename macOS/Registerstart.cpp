#include"Register.h"
using namespace std;
Register register_centre;
void showhelp(){
    cout<<"启动参数："<<endl;
    cout<<"-l [ip] 指定注册中心监听的ip地址"<<endl;
    cout<<"-p [port] 指定注册中心监听的端口号"<<endl;
    cout<<"-h 显示帮助"<<endl;
    cout<<"命令："<<endl;
    cout<<"help 显示帮助"<<endl;
    cout<<"exit/quit 退出"<<endl;
    cout<<"showself 查看注册中心自身信息"<<endl;
    cout<<"showserver 查看注册中心已注册的服务器"<<endl;
}
int main(int argc,char* argv[]){
    bool ok6=0,ok4=1,isl=0;
    string ip="0.0.0.0",ip6="不支持";
    string port="不支持",port6="不支持";
    string tempport="";
    int type=4;
    vector<string> another;
    if(argc==1){//默认启动，仅供测试
        register_centre.init("11111","11112","0.0.0.0","::");
        ok4=1;ok6=1;
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
            isl=1;
            string temp=argv[i+1];
            type = getiptype(temp);
            if(type==6){
                ip6=temp;
            }
            else{
                ip=temp;
            }
        }
        else if(argv[i][1]=='p'&&argv[i][0]=='-'){
            if(i+1>=argc){
                cout<<"启动参数错误"<<endl;
                return 0;
            }
            tempport=argv[i+1];
        }
    }
    if(tempport==""){
        cout<<"请指定端口号"<<endl;
        cin>>tempport;
    }
    if(type==4){
        ok4=1;
        port=tempport;
        another=askforanother(4);
        if(another[0]=="1"){
            ok6=1;
            ip6=another[1];
            port6=another[2];
            //cout<<ip6<<" "<<port6<<endl;
        }
    }
    else{
        ok6=1;
        port6=tempport;
        cout<<"请指定需监听的ipv4地址"<<endl;
        cin>>ip;
        cout<<"请指定ipv4端口号"<<endl;
        cin>>port;
    }
    while(!isgoodport(port)&&ok4){
        cout<<"ipv4端口号不合法，请重新输入"<<endl;
        cin>>port;
    }
    while(!isgoodport(port6)&&ok6){
        cout<<"ipv6端口号不合法，请重新输入"<<endl;
        cin>>port6;
    }
    register_centre.init(port,port6,ip,ip6);
    defaultstart:
    register_centre.readyToServe(ok4,ok6);
    string command;
    cout<<endl<<"输入help以查看可用命令"<<endl;
    while(1){
        cout<<endl;
        cin>>command;
        if(command=="help"){
            showhelp();
        }
        else if(command=="exit"||command=="quit"){
            int num=register_centre.getnumofserver();
            if(num>0){
                cout<<"仍有"<<num<<"个服务器注册于该注册中心，是否继续关闭？(y/n)"<<endl;
                char c;
                cin>>c;
                if(c=='n'){
                    continue;
                }
            }
            cout<<"注册中心已关闭，感谢使用！"<<endl;
            sleep(2);
            return 0;
        }
        else if(command=="showself"){
            register_centre.showself(ok4,ok6);
        }
        else if(command=="showserver"){
            cout<<register_centre.showtoClient()<<endl;
        }
        else{
            cout<<"未知命令，输入help以查看帮助"<<endl;
        }
    }
}