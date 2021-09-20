
#define WIN32_LEAN_AND_MEAN

#include<map>  //无序哈希表
#include <WinSock2.h>
#include <iostream>
#include <Windows.h>
#pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
#include <iostream>
#include"cJSON.h"
#include"CJsonObject.hpp"


using namespace std;
using namespace neb;

void InitWSAD();
CRITICAL_SECTION g_cs; //线程锁
DWORD WINAPI HANDLEClientThread(LPVOID pParam);

enum DataType
{
	DT_LOGIN,	//登录包
	DT_LOGIN_OK,//登录成功
	DT_LOGOUT,  //下线
	DT_PUBLIC_MSG,		//聊天数据包
	DT_PRIVATE_MSG,		//聊天数据包
	DT_FLUSH_NAME, //刷新用户
	DT_ADD_NAME,  //添加用户
	DT_DEL_NAME,  //删除用户
};

int main(int argc, char* argv[])
{
	string strName; 
	string strMsg;
	int ipPort;
	int UserCount=0;   //当前在线的用户数量
	int HashCount = 0; //当前哈希表最大数量

	map<int, pair<SOCKADDR_IN, string> >LoginMap; //用来保存登录信息的哈希表<<id><端口信息结构体，用户名>
	map<int, pair<SOCKADDR_IN, string>>::iterator iter;  //用来遍历哈希表的指针

	//初始化WSAD
	InitWSAD();
	//1. 创建socket, 指明要使用的协议什么
	SOCKET sock = socket(
		AF_INET,	//ipv4协议族
		SOCK_DGRAM, //数据报
		IPPROTO_UDP);//udp协议

	if (sock == INVALID_SOCKET)
	{
		printf("创建socket失败 \r\n");
		return 0;
	}
	else
	{
		printf("socket 创建socket成功 \r\n");
	}

	//2.绑定端口，指明本进程端口是什么
	sockaddr_in siServer;
	siServer.sin_family = AF_INET;
	siServer.sin_port = htons(9527);//0x3725;

	siServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//0x0100007f; //127.0.0.1

	int nRet = bind(sock, (sockaddr*)&siServer, sizeof(siServer));
	if (nRet == SOCKET_ERROR)
	{
		printf("端口绑定失败\r\n");
		closesocket(sock);
		return 0;
	}
	else
	{
		printf("绑定地址成功 \r\n");
	}
	printf("服务器已经启动.....\r\n");

	while (1) //用来接受数据，也可以用线程
	{
		//3.收数据
		sockaddr_in siClient;
		int nSizeOfSiRecv = sizeof(siClient);

		char szBuff[MAXSHORT] = { 0 };
		int nBytesRecv = recvfrom(sock,
			szBuff,
			sizeof(szBuff),
			0,
			(sockaddr*)&siClient,
			&nSizeOfSiRecv
		);

		if (nBytesRecv == SOCKET_ERROR)
		{
			closesocket(sock);
			printf("接受数据失败\r\n");
			return 0;
		}
		else {
			printf("收到来自IP地址：%s:%d的 JSON数据：%s \r\n",
				inet_ntoa(siClient.sin_addr),
				ntohs(siClient.sin_port),
				szBuff);
		}

		int port = ntohs(siClient.sin_port);
		//解析收到的json包
		CJsonObject jsonParse;
		bool bRet = jsonParse.Parse(szBuff);

		int TYPE;
		int PrivateID;
		int nId;
		int Number;
		int PreId;

		jsonParse.Get("name", strName); 
		jsonParse.Get("TYPE", TYPE);
		jsonParse.Get("Msg", strMsg);
		jsonParse.Get("PrivateID", PrivateID);
		jsonParse.Get("ID", nId);
		jsonParse.Get("Number", Number);
		jsonParse.Get("PreId", PreId);

		switch (TYPE)
		{
		case DT_LOGIN_OK://登录成功
		{
			//把登录信息保存到哈希表map里
			LoginMap[HashCount] = make_pair(siClient, strName);
			printf(" 客户端 ：%s: %d上线 \r\n",
				inet_ntoa(LoginMap[HashCount].first.sin_addr),
				ntohs(LoginMap[HashCount].first.sin_port)
			);
			HashCount++;
			UserCount++;

			{
				// 发一个登录信息包
				CJsonObject  jsonLogMsg;
				jsonLogMsg.Add("Type", DT_ADD_NAME);
				jsonLogMsg.Add("PreId", HashCount);

				char szLogSend[MAXWORD] = { 0 };
				string strJsonLogMsg = jsonLogMsg.ToString().c_str();
				int szLen = strJsonLogMsg.length();
				strcpy(szLogSend, strJsonLogMsg.c_str());

				nRet = sendto(
					sock,
					szLogSend,
					szLen + 1,
					0,
					(sockaddr*)&LoginMap[HashCount-1].first,
					sizeof(LoginMap[HashCount-1].first)
				);
			}

			//先发送一个删除包，通知客户端清空列表
			{
				CJsonObject  jsonListDelMsg;
				jsonListDelMsg.Add("Type", DT_DEL_NAME);
				for (int i = 0; i < HashCount; i++)
				{
					int p1 = LoginMap[i].first.sin_port;
					if (p1 != 0 )
					{
						char szSend[MAXWORD] = { 0 };
						string strJsonMsg = jsonListDelMsg.ToString().c_str();
						int szLen = strJsonMsg.length();
						strcpy(szSend, strJsonMsg.c_str());

						nRet = sendto(
							sock,
							szSend,
							szLen + 1,
							0,
							(sockaddr*)&LoginMap[i].first,
							sizeof(LoginMap[i].first));
					}
				}
			}

			//解析哈希表的客户端信息，回传用户列表信息
			for (int j = 0; j < HashCount; j++)
			{
				for (int i = 0; i < HashCount; i++)
				{
					int p1 = LoginMap[i].first.sin_port;
					int p2 = LoginMap[j].first.sin_port;
					if (p1 != 0 && p2 != 0)
					{
						CJsonObject  jsonListMsg;
						jsonListMsg.Add("Type", DT_LOGIN_OK);
						jsonListMsg.Add("ID", i);
						jsonListMsg.Add("name", LoginMap[i].second);
						jsonListMsg.Add("Port", LoginMap[i].first.sin_port);
						jsonListMsg.Add("Number",UserCount);

						char szSend[MAXWORD] = { 0 };
						string strJsonMsg = jsonListMsg.ToString().c_str();
						int szLen = strJsonMsg.length();
						strcpy(szSend, strJsonMsg.c_str());

						nRet = sendto(
							sock,
							szSend,
							szLen + 1,
							0,
							(sockaddr*)&LoginMap[j].first,
							sizeof(LoginMap[j].first));

						if (nRet == SOCKET_ERROR)
						{
							printf("发送数据失败 \r\n");
						}
					}
				}
			}
			break;
		case DT_LOGOUT:  //下线
		{
			//先发送一个删除包，通知客户端清空列表
			CJsonObject  jsonListDelMsg;
			jsonListDelMsg.Add("Type", DT_DEL_NAME);
			for (int i = 0; i < HashCount; i++)
			{
				if (LoginMap[i].second != "")
				{
					char szSend[MAXWORD] = { 0 };
					string strJsonMsg = jsonListDelMsg.ToString().c_str();
					int szLen = strJsonMsg.length();
					strcpy(szSend, strJsonMsg.c_str());

					nRet = sendto(
						sock,
						szSend,
						szLen + 1,
						0,
						(sockaddr*)&LoginMap[i].first,
						sizeof(LoginMap[i].first));
				}
			}

			//再重新添加列表
			CJsonObject  jsonListMsg;
			jsonListMsg.Add("Type", DT_LOGOUT);
			int res = LoginMap.erase(nId-1);
			UserCount--;
			if (UserCount > 0)
			{
				//解析哈希表的客户端信息，回传用户列表信息
				for (int j = 0; j < HashCount; j++)
				{
					for (int i = 0; i < HashCount; i++)
					{
						int p1 = LoginMap[i].first.sin_port;
						int p2 = LoginMap[j].first.sin_port;
						if (p1!=0 && p2!=0)
						{
							CJsonObject  jsonListMsg;
							jsonListMsg.Add("Type", DT_LOGOUT);
							jsonListMsg.Add("ID", i);
							jsonListMsg.Add("name", LoginMap[i].second);
							jsonListMsg.Add("Port", LoginMap[i].first.sin_port);
							jsonListMsg.Add("Number", UserCount);

							char szSend[MAXWORD] = { 0 };
							string strJsonMsg = jsonListMsg.ToString().c_str();
							int szLen = strJsonMsg.length();
							strcpy(szSend, strJsonMsg.c_str());

							nRet = sendto(
								sock,
								szSend,
								szLen + 1,
								0,
								(sockaddr*)&LoginMap[j].first,
								sizeof(LoginMap[j].first));

							if (nRet == SOCKET_ERROR)
							{
								printf("发送数据失败 \r\n");
							}
							else
							{
								printf("发送数据到客户端：%s:%d 数据：%s \r\n",
									inet_ntoa(LoginMap[i].first.sin_addr),
									ntohs(LoginMap[i].first.sin_port),
									szSend);
							}
						}				
					}
				}
			}
		}
		break;
		case DT_PUBLIC_MSG:
		{
			//接受传回的数据，发送群聊信息给所有客户端

			CJsonObject jsonEditMsg;
			jsonEditMsg.Add("Type", DT_PUBLIC_MSG);
			jsonEditMsg.Add("Name", strName);
			jsonEditMsg.Add("Show", strMsg);

			string strJsonMsg = jsonEditMsg.ToString().c_str();

			int szLen = strJsonMsg.length();
			char szSend[MAXWORD] = { 0 };
			strcpy(szSend, strJsonMsg.c_str());
			for (int i = 0; i < HashCount; i++)
			{
				if (LoginMap[i].second != "")
				{
					nRet = sendto(
						sock,
						szSend,
						szLen + 1,
						0,
						(sockaddr*)&LoginMap[i].first,
						sizeof(LoginMap[i].first));

					if (nRet == SOCKET_ERROR)
					{
						printf("发送数据失败 \r\n");
						break;
					}
					else
					{
						printf("发送数据到客户端：%s:%d 数据：%s \r\n",
							inet_ntoa(LoginMap[i].first.sin_addr),
							ntohs(LoginMap[i].first.sin_port),
							szSend);
					}
				}
			}

		}
		break;
		case DT_PRIVATE_MSG:
		{
			//发送私聊信息给选中的客户端
			string sz = "用户"  + strName  +"私聊对你说：\r\n  "+    strMsg + "\r\n";

			CJsonObject jsonEditMsg;
			jsonEditMsg.Add("Type", DT_PRIVATE_MSG);
			jsonEditMsg.Add("Name", strName);
			jsonEditMsg.Add("Show", strMsg);

			string strJsonMsg = jsonEditMsg.ToString().c_str();
			int szLen = strJsonMsg.length();
			char szSend[MAXWORD] = { 0 };
		strcpy(szSend, strJsonMsg.c_str());
			nRet = sendto(
				sock,
				szSend,
				szLen + 1,
				0,
				(sockaddr*)&LoginMap[PrivateID].first,
				sizeof(LoginMap[PrivateID].first));

			if (nRet == SOCKET_ERROR)
			{
				printf("发送数据失败 \r\n");
				break;
			}
			else
			{
				printf("发送私聊数据到客户端：%s:%d 数据：%s \r\n",
					inet_ntoa(LoginMap[PrivateID].first.sin_addr),
					ntohs(LoginMap[PrivateID].first.sin_port),
					szSend);
			}
		}
		break;
		case DT_ADD_NAME: //添加用户
		{

		}
		break;
		case DT_DEL_NAME:  //删除用户
		{

		}
		break;
		default:
			break;
		}

		}

	}
}

void InitWSAD()
{
	//初始化
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		return ;
	}

	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2) {
		WSACleanup();
		return ;
	}
}