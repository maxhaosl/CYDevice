#pragma once
#include <windows.h>
#include <gdiplus.h>

class GDIPlusRender
{
public:
    GDIPlusRender();
    ~GDIPlusRender();

    virtual bool Create(HWND hWnd, long lWidth, long lHeight);
    virtual bool PutData(unsigned char* pData, long lWidth, long lHeight, int index = 0);
    virtual bool Paint(HWND hWnd, HDC hDC, int x, int y, int index = 0, RECT* rc = NULL);
    virtual bool BeginPaint();
    virtual bool EndPaint();

protected:
    ULONG_PTR gdiplusToken;
    bool m_bInit;
    HMODULE m_hModule;
    HBITMAP m_hBitmap;
    HDC m_hDC;
    int m_nDepth;
    unsigned char* m_pBuffer;
    SIZE m_sizeBMP;
    HWND m_hWnd;

    void Clear();
    bool CreateDIB(int nWidth, int nHeight, int nDepth = 3);
    bool ModelImgToDIB(int nWidth, int nHeight, int nDepth, HWND hWnd, unsigned char** ppBuffer, HDC& hDC,
        HBITMAP& hBitmap, int& w, int& h, int& nDstDepth);
};
