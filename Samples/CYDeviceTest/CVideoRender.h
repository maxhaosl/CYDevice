#pragma once
#include "afxdialogex.h"

// CVideoRender 对话框

class CVideoRender : public CDialog
{
    DECLARE_DYNAMIC(CVideoRender)

public:
    CVideoRender(CWnd* pParent = nullptr);   // 标准构造函数
    virtual ~CVideoRender();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DLG_VIDEO };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

    DECLARE_MESSAGE_MAP()
};
