// CYDeviceTestDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "CYDeviceTest.h"
#include "CYDeviceTestDlg.h"
#include "afxdialogex.h"

#include "CYDevice/CYDeviceFatory.hpp"
#include "CYDevice/CYDeviceHelper.hpp"

#include <iostream>
#include <libyuv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

void ConvertYUV420PToRGB24(
    const uint8_t* yPlane,
    const uint8_t* uPlane,
    const uint8_t* vPlane,
    int width,
    int height,
    uint8_t* rgb24Buffer)
{
    int yStride = width;
    int uStride = width / 2;
    int vStride = width / 2;
    int rgbStride = width * 3;

    // libyuv expects I420 layout: Y + U + V
    libyuv::I420ToRGB24(
        yPlane, yStride,
        uPlane, uStride,
        vPlane, vStride,
        rgb24Buffer, rgbStride,
        width, height
    );
}

class CAboutDlg : public CDialog
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

CAboutDlg::CAboutDlg() : CDialog(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

// CCYDeviceTestDlg 对话框

CCYDeviceTestDlg::CCYDeviceTestDlg(CWnd* pParent /*=nullptr*/)
    : CDialog(IDD_CYDEVICETEST_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCYDeviceTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CB_CAMERA, m_cbCamera);
    DDX_Control(pDX, IDC_CB_MIC, m_cbMic);
}

BEGIN_MESSAGE_MAP(CCYDeviceTestDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BTN_CAPTURE_START, &CCYDeviceTestDlg::OnBnClickedBtnCaptureStart)
    ON_BN_CLICKED(IDC_BTN_CAPTURE_STOP, &CCYDeviceTestDlg::OnBnClickedBtnCaptureStop)
END_MESSAGE_MAP()

// CCYDeviceTestDlg 消息处理程序

BOOL CCYDeviceTestDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

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

    //////////////////////////////////////////////////////////////////////////
    uint32_t nCameraCount = 0;
    uint32_t nMicCount = 0;
    m_ptrCameraList = std::make_unique<CYDEVICE_NAMESPACE::TDeviceInfo[]>(20);
    m_ptrMicList = std::make_unique<CYDEVICE_NAMESPACE::TDeviceInfo[]>(20);
    CYDEVICE_NAMESPACE::GetDeviceList(CYDEVICE_NAMESPACE::TYPE_CYDEVICE_VIDEO_INPUT, m_ptrCameraList.get(), &nCameraCount);
    CYDEVICE_NAMESPACE::GetDeviceList(CYDEVICE_NAMESPACE::TYPE_CYDEVICE_AUDIO_INPUT, m_ptrMicList.get(), &nMicCount);
    USES_CONVERSION;
    for (int nIndex = 0; nIndex < nCameraCount; nIndex++)
    {
        m_cbCamera.AddString(A2T(m_ptrCameraList[nIndex].szDeviceName));
    }

    if (nCameraCount > 0) m_cbCamera.SetCurSel(0);

    for (int nIndex = 0; nIndex < nMicCount; nIndex++)
    {
        m_cbMic.AddString(A2T(m_ptrMicList[nIndex].szDeviceName));
    }

    if (nMicCount > 0) m_cbMic.SetCurSel(0);

    //////////////////////////////////////////////////////////////////////////
    m_objVideoRender.Create(IDD_DLG_VIDEO, this);

    CRect rcClient;
    ::GetClientRect(GetDlgItem(IDC_STC_VIDEO)->m_hWnd, &rcClient);
    m_objVideoRender.MoveWindow(rcClient);
    m_objVideoRender.ShowWindow(SW_SHOW);

    m_nWidth = 1024;
    m_nHeight = 768;

    m_pDevice = CYDEVICE_NAMESPACE::CYDeviceFactory::CreateDevice();
    m_pDevice->Init(m_nWidth, m_nHeight, 25, A2T(m_ptrCameraList[0].szDeviceName), A2T(m_ptrCameraList[0].szDeviceId), 44100, A2T(m_ptrMicList[0].szDeviceName), A2T(m_ptrMicList[0].szDeviceId), false);

    m_objGDIPlusRender.Create(m_objVideoRender.m_hWnd, m_nWidth, m_nHeight);
    m_ptrRGBBuffer = std::make_unique<uint8_t[]>(m_nWidth * m_nHeight * 3);
    m_nRGBBufferSize = m_nWidth * m_nHeight * 3;
    return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCYDeviceTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CCYDeviceTestDlg::OnPaint()
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
        CDialog::OnPaint();
    }
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCYDeviceTestDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CCYDeviceTestDlg::OnClose()
{
    if (m_pDevice)
    {
        m_pDevice->StopCapture();
        m_pDevice->UnInit();    
    }
    CYDEVICE_NAMESPACE::CYDeviceFactory::DestroyDevice(m_pDevice);
    CDialog::OnClose();
}

void CCYDeviceTestDlg::OnBnClickedBtnCaptureStart()
{
    if (m_pDevice)
    {
        m_pDevice->StartCapture(this, this);
    }
}

void CCYDeviceTestDlg::OnBnClickedBtnCaptureStop()
{
    if (m_pDevice)
    {
        m_pDevice->StopCapture();
    }
}

void CCYDeviceTestDlg::OnAudioData(float* pBuffer, uint32_t nNumberAudioFrames, uint32_t nChannel, uint64_t nTimeStamps)
{
    OutputDebugStringA("AudioData\r\n");
    return;
}

void CCYDeviceTestDlg::OnVideoData(const unsigned char* pData, int nLen, int nWidth, int nHeight, unsigned long long nTimeStampls)
{
    OutputDebugStringA("VideoData\r\n");


    int w = nWidth, h = nHeight;

    uint8_t* yuv = (uint8_t*)pData;  // 输入数据

    uint8_t* y = yuv;
    uint8_t* u = yuv + w * h;
    uint8_t* v = u + (w * h) / 4;

    ConvertYUV420PToRGB24(y, u, v, w, h, m_ptrRGBBuffer.get());

    m_objGDIPlusRender.PutData(m_ptrRGBBuffer.get(), w, h, 0);
    m_objGDIPlusRender.BeginPaint();
    HDC hdc = ::GetDC(m_objVideoRender.m_hWnd);
    CRect rcWindow;
    m_objVideoRender.GetClientRect(&rcWindow);
    m_objGDIPlusRender.Paint(m_objVideoRender.m_hWnd, hdc, 0, 0, 0, &rcWindow);
    ::ReleaseDC(m_objVideoRender.m_hWnd, hdc);

    
    m_objGDIPlusRender.EndPaint();

    return;
}