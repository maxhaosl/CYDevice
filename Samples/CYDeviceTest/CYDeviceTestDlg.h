// CYDeviceTestDlg.h: 头文件
//

#pragma once

#include "CVideoRender.h"
#include "CYDevice/ICYDevice.hpp"
#include "Render/GDIPlusRender.h"
#include <memory>

// CCYDeviceTestDlg 对话框
class CCYDeviceTestDlg : public CDialog, public CYDEVICE_NAMESPACE::ICYAudioDataCallBack, public CYDEVICE_NAMESPACE::ICYVideoDataCallBack
{
    // 构造
public:
    CCYDeviceTestDlg(CWnd* pParent = nullptr);	// 标准构造函数

    // 对话框数据
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_CYDEVICETEST_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

    virtual void OnAudioData(float* pBuffer, uint32_t nNumberAudioFrames, uint32_t nChannel, uint64_t nTimeStamps) override;
    virtual void OnVideoData(const unsigned char* pData, int nLen, int nWidth, int nHeight, unsigned long long nTimeStampls) override;
    // 实现
protected:
    HICON m_hIcon;

    // 生成的消息映射函数
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()

private:
    CYDEVICE_NAMESPACE::ICYDevice* m_pDevice = nullptr;
    CVideoRender m_objVideoRender;
    GDIPlusRender m_objGDIPlusRender;
    uint32_t m_nRGBBufferSize = 0;
    std::unique_ptr<uint8_t[]> m_ptrRGBBuffer;

    int m_nWidth = 1920;
    int m_nHeight = 1080;

public:
    afx_msg void OnClose();
    CComboBox m_cbCamera;
    CComboBox m_cbMic;

    std::unique_ptr<CYDEVICE_NAMESPACE::TDeviceInfo[]> m_ptrCameraList;
    std::unique_ptr<CYDEVICE_NAMESPACE::TDeviceInfo[]> m_ptrMicList;
    afx_msg void OnBnClickedBtnCaptureStart();
    afx_msg void OnBnClickedBtnCaptureStop();
};
