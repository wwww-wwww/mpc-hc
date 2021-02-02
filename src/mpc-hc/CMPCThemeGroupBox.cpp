#include "stdafx.h"
#include "CMPCThemeGroupBox.h"
#include "CMPCTheme.h"
#include "CMPCThemeUtil.h"

IMPLEMENT_DYNAMIC(CMPCThemeGroupBox, CStatic)

CMPCThemeGroupBox::CMPCThemeGroupBox()
{

}


CMPCThemeGroupBox::~CMPCThemeGroupBox()
{
}

BEGIN_MESSAGE_MAP(CMPCThemeGroupBox, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()


void CMPCThemeGroupBox::OnPaint()
{
    if (AppIsThemeLoaded()) {

        CPaintDC dc(this);

        CRect r, rborder, rtext;
        GetClientRect(r);
        HDC hDC = ::GetDC(NULL);
        CString text;
        GetWindowText(text);

        CFont* font = GetFont();
        CSize cs = CMPCThemeUtil::GetTextSize(_T("W"), hDC, font);
        ::ReleaseDC(NULL, hDC);

        CBrush fb;
        fb.CreateSolidBrush(CMPCTheme::GroupBoxBorderColor);
        rborder = r;
        rborder.top += cs.cy / 2;
        dc.FrameRect(rborder, &fb);

        if (!text.IsEmpty()) {

            COLORREF oldClr = dc.SetTextColor(CMPCTheme::TextFGColor);
            COLORREF oldBkClr = dc.SetBkColor(CMPCTheme::WindowBGColor);
            CFont* pOldFont = dc.SelectObject(font);

            rtext = r;
            rtext.left += CMPCTheme::GroupBoxTextIndent;

            text += _T(" "); //seems to be the default behavior
            dc.DrawText(text, rtext, DT_TOP | DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL);

            dc.SelectObject(pOldFont);
            dc.SetTextColor(oldClr);
            dc.SetBkColor(oldBkClr);
        }
        fb.DeleteObject();
        ::ReleaseDC(NULL, hDC);
    } else {
        __super::OnPaint();
    }
}
