#pragma once
#include <afxwin.h>
class CMPCThemeInlineEdit :
    public CEdit
{
public:
    CMPCThemeInlineEdit();
    virtual ~CMPCThemeInlineEdit();
    CBrush m_brBkgnd;
    void setOverrideX(int x);
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
    afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
private:
    int overrideX;
    bool offsetEnabled;
};

