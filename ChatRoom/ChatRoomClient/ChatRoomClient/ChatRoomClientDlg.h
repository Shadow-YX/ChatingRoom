
// ChatRoomClientDlg.h: 头文件
//

#pragma once
#include"cJSON.h"
#include "InitSocket.h"
// CChatRoomClientDlg 对话框
class CChatRoomClientDlg : public CDialogEx
{
// 构造
public:
	CChatRoomClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATROOMCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	SOCKET sock;
	sockaddr_in siTo;
	CString strName;
	CString strPort;
	CRITICAL_SECTION m_cs; //同步

	int UserNumber=0; // 保存当前在线人数
	int nItemIdx = 0;
	int isRepeat = 0; //检测是否重名

	enum DataType //枚举发包的json类型
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
	int nItemShow = 0;
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	static DWORD WINAPI  OnRecvThread(LPVOID lpParam);
	void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLogin();//登录
	afx_msg void OnBnClickedQuit();//退出
	afx_msg void OnBnClickedPublic();//群聊
	afx_msg void OnBnClickedPrivate();//私聊
	void InitWSAD();
	void LoadSkin();

	int m_edtPort;
	CEdit m_edtName;
	CString m_edtIP;
	CListCtrl m_listLst;

	CEdit m_edtMsg;
	InitSocket  _InitSock;
	CEdit m_edtNumber;

	CListBox m_listShow;
	afx_msg void OnEnChangeId();
	CEdit m_edtId;
};
