
// ChatRoomClientDlg.cpp: 实现文件
//


#include "framework.h"
#include "ChatRoomClient.h"
#include "ChatRoomClientDlg.h"
#include "afxdialogex.h"
#include"cJSON.h"
#include"CJsonObject.hpp"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
using namespace neb;
using namespace std;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatRoomClientDlg 对话框



CChatRoomClientDlg::CChatRoomClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATROOMCLIENT_DIALOG, pParent)
	, m_edtPort(0)
	, m_edtIP(_T(""))

{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CChatRoomClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, EDT_PORT, m_edtPort);
	DDX_Control(pDX, EDT_NAME, m_edtName);
	DDX_Text(pDX, EDT_IPADDRESS, m_edtIP);
	DDX_Control(pDX, LIST_LST, m_listLst);

	DDX_Control(pDX, EDT_MSG, m_edtMsg);
	DDX_Control(pDX, EDT_NUMBER, m_edtNumber);
	DDX_Control(pDX, LIST_SHOW, m_listShow);
	DDX_Control(pDX, EDT_ID, m_edtId);
}


BEGIN_MESSAGE_MAP(CChatRoomClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(BTN_LOGIN, &CChatRoomClientDlg::OnBnClickedLogin)
	ON_BN_CLICKED(BTN_QUIT, &CChatRoomClientDlg::OnBnClickedQuit)
	ON_BN_CLICKED(BTN_PUBLIC, &CChatRoomClientDlg::OnBnClickedPublic)
	ON_BN_CLICKED(BTN_PRIVATE, &CChatRoomClientDlg::OnBnClickedPrivate)
	ON_EN_CHANGE(EDT_ID, &CChatRoomClientDlg::OnEnChangeId)
END_MESSAGE_MAP()


// CChatRoomClientDlg 消息处理程序

BOOL CChatRoomClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标
	LoadSkin(); //载入皮肤

	//默认IP地址127.0.0.1
	m_edtIP = "127.0.0.1";
	m_edtPort = 9527;
	m_listLst.InsertColumn(0, "id", LVCFMT_LEFT, 31);
	m_listLst.InsertColumn(1, "用户名", LVCFMT_LEFT, 80);
	m_listLst.InsertColumn(2, "端口号", LVCFMT_LEFT, 80);

	m_edtNumber.SetWindowText(NULL);

	UpdateData(FALSE);
	//设置选中一整行的风格
	m_listLst.SetExtendedStyle(LVS_EX_FULLROWSELECT | m_listLst.GetExtendedStyle());
	InitializeCriticalSection(&m_cs); //初始化临界资源对象


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

DWORD CChatRoomClientDlg::OnRecvThread(LPVOID lpParam)
{
	//接受并显示数据到窗口
	CChatRoomClientDlg* pThis = (CChatRoomClientDlg*)lpParam;

	while (true)
	{
		//接受服务器数据并将数据返回到对话框
		sockaddr_in siRecv;
		int nSizeOfSiRecv = sizeof(siRecv);
		char szBuff0[MAXWORD + 1] = { 0 };
		int nBytesRecv = recvfrom(pThis->sock,
			szBuff0,
			sizeof(szBuff0),
			0,
			(sockaddr*)&siRecv,
			&nSizeOfSiRecv);

		if (nBytesRecv == SOCKET_ERROR)
		{
			break;
		}

		//解析返回的Json包

		CJsonObject jsonParse;
		bool bRet = jsonParse.Parse(szBuff0);

		int TYPE;
		int PreId = 0;
		string strName;
		string strMsg;
		string strShow;
		int Number;
		CString LogCount;
		CString strPort;

		int nid;
		int nPort;
		CString LogId;
		jsonParse.Get("name", strName);
		jsonParse.Get("TYPE", TYPE);
		jsonParse.Get("Msg", strMsg);
		jsonParse.Get("Show", strShow);
		jsonParse.Get("Number", Number);
		jsonParse.Get("PreId", PreId);
		jsonParse.Get("ID", nid);
		jsonParse.Get("Port", nPort);

		LogCount.Format("%d", Number);
		LogId.Format("%d",nid+1);
		strPort.Format("%d", nPort);

		switch (TYPE)
		{
		case DT_LOGIN_OK:
		{

				CString strOldMsg;
				//pThis->m_edtShow.GetWindowText(strOldMsg);
				CString strRecvMsg;
				int nItemShow = 0;

				/*
				CString cstrShow1 = "用户 ";
				CString cstrName = strName.c_str();
				CString cstrShow2=" 已经上线！\r\n";
				CString cstrShowstr = cstrShow1 + cstrName + cstrShow2;
				pThis->m_listShow.AddString(cstrShowstr);
				*/

				int nColIdx = 0;
				pThis->m_listLst.InsertItem(pThis->nItemIdx, LogId);
				pThis->m_listLst.SetItemText(pThis->nItemIdx, ++nColIdx, strName.c_str());
				pThis->m_listLst.SetItemText(pThis->nItemIdx++, ++nColIdx, strPort);
				pThis->UserNumber = Number;
				pThis->m_edtNumber.SetWindowText(LogCount);

		}
		break;
		case DT_ADD_NAME://登录信息包
		{
			CString strId;
			strId.Format("%d", PreId);
			pThis->m_edtId.SetWindowText(strId);
		}
		break;
		case DT_LOGOUT:  //下线
		{
			int nColIdx = 0;
			pThis->m_listLst.InsertItem(pThis->nItemIdx, LogId);
			pThis->m_listLst.SetItemText(pThis->nItemIdx, ++nColIdx, strName.c_str());
			pThis->m_listLst.SetItemText(pThis->nItemIdx++, ++nColIdx, strPort);
			pThis->UserNumber = Number;
			pThis->m_edtNumber.SetWindowText(LogCount);
		}
		break;
		case DT_PUBLIC_MSG:
		{
			//将数据返回到对话框

			CString strOldMsg;
			//pThis->m_edtShow.GetWindowText(strOldMsg);
			CString strRecvMsg;
			//pThis->m_edtShow.SetWindowText(strOldMsg+strShow.c_str());

			CString cstrShow1 = "用户 ";
			CString cstrName = strName.c_str();
			CString cstrShow2 = "   ";
			CString cstrShow3 = ":";
			CString cstrShowUser = cstrShow1 + cstrName+cstrShow3;
			CString cstrShowMsg = cstrShow2 + strShow.c_str();
			pThis->m_listShow.AddString(cstrShowUser);
			pThis->m_listShow.AddString(cstrShowMsg);
		}
		break;
		case DT_PRIVATE_MSG:
		{
			CString cstrShow1 = "用户 ";
			CString cstrName = strName.c_str();
			CString cstrName2 = " 私聊对你说:";
			CString cstrShow2 = "   ";
			CString cstrShowUser = cstrShow1+cstrName+cstrName2;
			CString cstrShowMsg = cstrShow2 + strShow.c_str();
			pThis->m_listShow.AddString(cstrShowUser);
			pThis->m_listShow.AddString(cstrShowMsg);
		}
		break;
		case DT_DEL_NAME:  //删除用户
		{
			pThis->nItemIdx = 0;
			pThis->m_listLst.DeleteAllItems();
		}
		break;
		default:
			break;

		}

	}

	return 0;
}


	void CChatRoomClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CChatRoomClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CChatRoomClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CChatRoomClientDlg::OnBnClickedLogin()
{

	if (UserNumber != 0)
	{
		AfxMessageBox("您已登录无法重复登录！");
		return;
	}

	int nColIdx = 0;
	CString strPort;
	CString strId;

	m_edtName.GetWindowText(strName); 
	strPort.Format("%d", m_edtPort);
	strId.Format("%d", nItemIdx + 1);

	//1. 创建socket, 指明要使用的协议什么
	sock = socket(
		AF_INET,	//ipv4协议族
		SOCK_DGRAM, //数据报
		IPPROTO_UDP);//udp协议

	if (sock == INVALID_SOCKET)
	{
		AfxMessageBox("网络连接中断 \r\n");
		return;
	}

	//2. 绑定端口

	siTo.sin_family = AF_INET;
	siTo.sin_port = htons(9527);
	siTo.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");


	//先发送一个登录确认包，让服务器保存登录信息
	CJsonObject jsonMsg;
	jsonMsg.Add("Type", DT_LOGIN_OK );
	jsonMsg.Add("name", strName.GetBuffer());

	CString strJsonMsg = jsonMsg.ToString().c_str();

	int nBytesSend = sendto(sock,
		strJsonMsg.GetBuffer(),
		strJsonMsg.GetLength(),
		0,
		(sockaddr*)&siTo, //客户端的的目标地址和端口
		sizeof(siTo));

	if (nBytesSend == SOCKET_ERROR)
	{
		closesocket(sock);
		AfxMessageBox("发送数据失败 \r\n");
		return;
	}
	

	HANDLE m_pthread = CreateThread(NULL, 0, OnRecvThread, this, 0, NULL); //创建一个接受服务器返回信息的线程
	CloseHandle(m_pthread);


	UpdateData(TRUE);
}

void CChatRoomClientDlg::OnBnClickedQuit()
{
	CString strId;
	m_edtId.GetWindowText(strId);

	if (strId == "")
	{
		AfxMessageBox("亲！请您先登陆哟！");
		return;
	}

	nItemIdx = 0;  //初始化列表

	int  nId = atoi(strId);
	CJsonObject jsonMsg;
	jsonMsg.Add("Type", DT_LOGOUT);
	jsonMsg.Add("ID", nId);

	CString strJsonMsg = jsonMsg.ToString().c_str();

	//发送退出的json包
	int nBytesSend = sendto(sock,
		strJsonMsg.GetBuffer(),
		strJsonMsg.GetLength(),
		0,
		(sockaddr*)&siTo, //客户端的的目标地址和端口
		sizeof(siTo));

	if (nBytesSend == SOCKET_ERROR)
	{
		closesocket(sock);
		AfxMessageBox("发送退出数据失败 \r\n");
		return;
	}

	m_listLst.DeleteAllItems();
	m_listShow.ResetContent();
	m_edtId.SetWindowText("");
	m_edtMsg.SetWindowText("");
	m_edtName.SetWindowText("");
	m_edtNumber.SetWindowText("");

	closesocket(sock);
}


void CChatRoomClientDlg::OnBnClickedPublic()
{
	UpdateData(TRUE);

	m_edtName.GetWindowText(strName);
	if (strName == "")
	{
		AfxMessageBox("亲！请您先登陆哟！");
		return;
	}

	CString strMsg;
	 m_edtMsg.GetWindowText(strMsg);

	CJsonObject jsonMsg;
	jsonMsg.Add("Type", DT_PUBLIC_MSG);
	jsonMsg.Add("name",strName.GetBuffer());
	jsonMsg.Add("msg",strMsg.GetBuffer());

	CString strJsonMsg = jsonMsg.ToString().c_str();
	//发送群聊的json包
	int nBytesSend = sendto(sock,
		strJsonMsg.GetBuffer(),
		strJsonMsg.GetLength(),
		0,
		(sockaddr*)&siTo, //客户端的的目标地址和端口
		sizeof(siTo));

	if (nBytesSend == SOCKET_ERROR)
	{
		closesocket(sock);
		AfxMessageBox("发送数据失败 \r\n");
		return;
	}

}


void CChatRoomClientDlg::OnBnClickedPrivate()
{
	UpdateData(TRUE);

	m_edtName.GetWindowText(strName);
	if (strName == "")
	{
		AfxMessageBox("亲！请您先登陆哟！");
		return;
	}

	CString strMsg;
	m_edtMsg.GetWindowText(strMsg);
		

	//获取选中项
	int nIdx = m_listLst.GetSelectionMark();
	CString strPId = m_listLst.GetItemText(nIdx, 0);
	int nPId = atoi(strPId);
	//发送私聊的json包
	CJsonObject jsonMsg;
	jsonMsg.Add("Type", DT_PRIVATE_MSG);
	jsonMsg.Add("name", strName.GetBuffer());
	jsonMsg.Add("msg", strMsg.GetBuffer());
	jsonMsg.Add("PrivateID",nPId-1);

	CString strJsonMsg = jsonMsg.ToString().c_str();

	int nBytesSend = sendto(sock,
		strJsonMsg.GetBuffer(),
		strJsonMsg.GetLength(),
		0,
		(sockaddr*)&siTo, //客户端的的目标地址和端口
		sizeof(siTo));

	if (nBytesSend == SOCKET_ERROR)
	{
		closesocket(sock);
		AfxMessageBox("发送数据失败 \r\n");
		return;
	}
	
}


void CChatRoomClientDlg::InitWSAD()
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

void CChatRoomClientDlg::LoadSkin()
{

	HMODULE hModule = LoadLibrary(TEXT("MySafeSkin.dll"));
	if (hModule)
	{
		typedef  int  (WINAPI* pMySafeSkin)(void);
		pMySafeSkin MySafeSkin;
		MySafeSkin = (pMySafeSkin)GetProcAddress(hModule, "MySafeSkin");
		MySafeSkin();
	}
	
}




void CChatRoomClientDlg::OnEnChangeId()
{

}
