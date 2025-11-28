// CVideoRender.cpp: 实现文件
//

#include "pch.h"
#include "CYDeviceTest.h"
#include "afxdialogex.h"
#include "CVideoRender.h"


// CVideoRender 对话框

IMPLEMENT_DYNAMIC(CVideoRender, CDialog)

CVideoRender::CVideoRender(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_VIDEO, pParent)
{

}

CVideoRender::~CVideoRender()
{
}

void CVideoRender::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVideoRender, CDialog)
END_MESSAGE_MAP()


// CVideoRender 消息处理程序
