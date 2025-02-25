help:
	@echo "client: 编译客户端程序"
	@echo "server: 编译服务器端程序"
	@echo "register: 编译注册中心程序"
	@echo "all: 编译所有程序"

all: client server register

client:
	@g++ -std=c++17 Clientstart.cpp -o Clientstart
	@echo "Clientstart.cpp 编译完成"
server:
	@g++ -std=c++17 Serverstart.cpp -o Serverstart
	@echo "Serverstart.cpp 编译完成"
register:
	@g++ -std=c++17 Registerstart.cpp -o Registerstart
	@echo "Registerstart.cpp 编译完成"
