#include "stdafx.h"
#include "CMPCThemeScrollBarHelper.h"
#include "CMPCTheme.h"
#include "CMPCThemeUtil.h"

CMPCThemeScrollBarHelper::CMPCThemeScrollBarHelper(CWnd* scrollWindow)
{
    window = scrollWindow;
    pParent = nullptr;
    currentlyClipped = false;
}


CMPCThemeScrollBarHelper::~CMPCThemeScrollBarHelper()
{
}

void CMPCThemeScrollBarHelper::createSB()
{
    pParent = window->GetParent();
    hasVSB = 0 != (window->GetStyle() & WS_VSCROLL); //we have to draw vertical scrollbar because ncpaint is overridden to handle horizontal scrollbar
    hasHSB = 0 != (window->GetStyle() & WS_HSCROLL); //windows dark theme horizontal scrollbar is broken
    if (nullptr != pParent && IsWindow(pParent->m_hWnd)) {
        if (hasVSB && !IsWindow(vertSB.m_hWnd)) {
            VERIFY(vertSB.Create(SBS_VERT | WS_CHILD |
                                 WS_VISIBLE, CRect(0, 0, 0, 0), pParent, 0));
            vertSB.setScrollWindow(window); //we want messages from this SB
        }

        if (hasHSB && !IsWindow(horzSB.m_hWnd)) {
            VERIFY(horzSB.Create(SBS_HORZ | WS_CHILD |
                                 WS_VISIBLE, CRect(0, 0, 0, 0), pParent, 0));
            horzSB.setScrollWindow(window); //we want messages from this SB
        }
    }
}

void CMPCThemeScrollBarHelper::setDrawingArea(CRect& cr, CRect& wr, bool clipping)
{
    window->GetClientRect(&cr);
    window->ClientToScreen(&cr);
    window->GetWindowRect(&wr);

    CRect wrOnParent = wr;
    pParent = window->GetParent();
    if (nullptr != pParent) {
        pParent->ScreenToClient(wrOnParent);
    }

    cr.OffsetRect(-wr.left, -wr.top);
    wr.OffsetRect(-wr.left, -wr.top);

    if (clipping) {
        //system metrics doesn't consider other border sizes--client-window works better in all cases so far
        int borderWidth = cr.left - wr.left; //GetSystemMetrics(SM_CXSIZEFRAME) + 1;
        int sbThickness = GetSystemMetrics(SM_CXVSCROLL);
        CRect realWR = wr;
        bool canVSB = sbThickness < wr.Width() - borderWidth * 2; //SB simply disappears if window is that small
        bool canHSB = sbThickness < wr.Height() - borderWidth * 2; //SB simply disappears if window is that small

        if (IsWindow(vertSB.m_hWnd)) {
            if (hasVSB && canVSB) {
                int width = sbThickness, height = realWR.bottom - realWR.top - 2 * borderWidth - (hasHSB ? sbThickness : 0);
                wr.right -= sbThickness + borderWidth; //clip whole SB plus border

                vertSB.MoveWindow(wrOnParent.right - width - borderWidth, wrOnParent.top + borderWidth, width, height);
                vertSB.ShowWindow(SW_SHOW);
                updateScrollInfo();
            } else {
                if (vertSB.IsWindowVisible()) {
                    CRect sbWR;
                    vertSB.GetWindowRect(sbWR);
                    vertSB.ShowWindow(SW_HIDE);
                    window->ScreenToClient(sbWR);
                    window->RedrawWindow(sbWR, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
                }
            }
        }

        if (IsWindow(horzSB.m_hWnd)) {
            if (hasHSB && canHSB) {
                int height = sbThickness, width = realWR.right - realWR.left - 2 * borderWidth - (hasVSB ? sbThickness : 0);
                wr.bottom -= sbThickness + borderWidth; //clip whole SB plus border

                horzSB.MoveWindow(wrOnParent.left + borderWidth, wrOnParent.bottom - height - borderWidth, width, height);
                horzSB.ShowWindow(SW_SHOW);
                updateScrollInfo();
            } else {
                if (horzSB.IsWindowVisible()) {
                    CRect sbWR;
                    horzSB.GetWindowRect(sbWR);
                    horzSB.ShowWindow(SW_HIDE);
                    window->ScreenToClient(sbWR);
                    window->RedrawWindow(sbWR, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
                }
            }
        }
        bool wasClipped = currentlyClipped;

        if (wr != realWR) {
            HRGN iehrgn = CreateRectRgn(wr.left, wr.top, wr.right, wr.bottom);
            window->SetWindowRgn(iehrgn, false);
            currentlyClipped = true;
        } else {
            window->SetWindowRgn(NULL, false);
            currentlyClipped = false;
        }
        if (wasClipped && currentClipRegion != wr) { //we do not repaint during SetWindowRgn, but we need to invalidate areas that were previously clipped
            if (currentClipRegion.right < wr.right) {
                CRect rightRedraw;
                window->GetClientRect(rightRedraw);
                rightRedraw.left = rightRedraw.right - (wr.right - currentClipRegion.right);
                window->RedrawWindow(rightRedraw, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
            if (currentClipRegion.bottom < wr.bottom) {
                CRect bottomRedraw = wr;
                window->GetClientRect(bottomRedraw);
                bottomRedraw.top = bottomRedraw.bottom - (wr.bottom - currentClipRegion.bottom);
                window->RedrawWindow(bottomRedraw, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
        }
        currentClipRegion = wr;
    } else {
        window->SetWindowRgn(NULL, false);
    }
}

void CMPCThemeScrollBarHelper::hideSB()
{
    CRect wr, cr;
    setDrawingArea(cr, wr, true);
}

void CMPCThemeScrollBarHelper::updateScrollInfo(bool invalidate /*=false*/)
{
    if (IsWindow(vertSB.m_hWnd)) {
        vertSB.updateScrollInfo();
        if (invalidate) {
            vertSB.Invalidate();
        }
    }
    if (IsWindow(horzSB.m_hWnd)) {
        horzSB.updateScrollInfo();
        if (invalidate) {
            horzSB.Invalidate();
        }
    }
}


//clistctrl does not seem to scroll when receiving thumb messages, so we handle them here
//this will allow the scrollbar to update as well
//inspired by flyhigh https://www.codeproject.com/Articles/14724/Replace-a-Window-s-Internal-Scrollbar-with-a-custo
//changed to avoid glitchy redraws and only update the scrollbar that has been changed
bool CMPCThemeScrollBarHelper::WindowProc(CListCtrl* list, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_VSCROLL || message == WM_HSCROLL) {
        WORD sbCode = LOWORD(wParam);
        if (sbCode == SB_THUMBTRACK || sbCode == SB_THUMBPOSITION) {
            SCROLLINFO siv = { 0 };
            siv.cbSize = sizeof(SCROLLINFO);
            siv.fMask = SIF_ALL;
            SCROLLINFO sih = siv;
            int nPos = HIWORD(wParam);
            CRect rcClient;
            list->GetClientRect(&rcClient);

            SIZE sizeAll;
            SIZE size = { 0, 0 };
            if (WM_VSCROLL == message) {
                list->GetScrollInfo(SB_VERT, &siv);
                if (siv.nPage == 0) {
                    sizeAll.cy = rcClient.bottom;
                } else {
                    sizeAll.cy = rcClient.bottom * (siv.nMax + 1) / siv.nPage;
                }
                size.cy = sizeAll.cy * (nPos - siv.nPos) / (siv.nMax + 1);
            } else {
                list->GetScrollInfo(SB_HORZ, &sih);
                if (sih.nPage == 0) {
                    sizeAll.cx = rcClient.right;
                } else {
                    sizeAll.cx = rcClient.right * (sih.nMax + 1) / sih.nPage;
                }
                size.cx = sizeAll.cx * (nPos - sih.nPos) / (sih.nMax + 1);
            }
            //adipose: this code is needed to prevent listctrl glitchy drawing.
            //scroll sends a cascade of redraws which are untenable during a thumb drag
            //only one redraw per scroll call this way
            if (nullptr != pParent && IsWindow(pParent->m_hWnd)) {
                pParent->SetRedraw(FALSE);
                list->Scroll(size);
                pParent->SetRedraw();
                list->Invalidate();
                CHeaderCtrl* hdrCtrl = list->GetHeaderCtrl();
                if (nullptr != hdrCtrl) {
                    hdrCtrl->Invalidate();
                }
            }
            return true; //processed
        }
    }
    return false;
}

bool CMPCThemeScrollBarHelper::WindowProc(CTreeCtrl* tree, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_VSCROLL || message == WM_HSCROLL) {
        WORD sbCode = LOWORD(wParam);
        if (sbCode == SB_THUMBTRACK || sbCode == SB_THUMBPOSITION) {
            SCROLLINFO siv = { 0 };
            siv.cbSize = sizeof(SCROLLINFO);
            siv.fMask = SIF_ALL;
            SCROLLINFO sih = siv;
            int nPos = HIWORD(wParam);
            CRect rcClient;
            tree->GetClientRect(&rcClient);
            tree->GetScrollInfo(SB_VERT, &siv);
            tree->GetScrollInfo(SB_HORZ, &sih);

            WPARAM wp = (WPARAM) - 1;
            int lines = 0;
            if (WM_VSCROLL == message) {
                wp = nPos < siv.nPos ? SB_LINEUP : SB_LINEDOWN;
                lines = abs(nPos - siv.nPos);
            } else {
                wp = nPos < sih.nPos ? SB_LINELEFT : SB_LINERIGHT;
                lines = abs(nPos - sih.nPos);
            }

            if (-1 != wp && nullptr != pParent && IsWindow(pParent->m_hWnd)) {
                pParent->SetRedraw(FALSE);
                while (lines-- > 0) {
                    tree->SendMessage(message, wp, 0);
                }
                pParent->SetRedraw();
                tree->Invalidate();
            }
            return true; //processed
        }
    }
    return false;
}

void CMPCThemeScrollBarHelper::themedNcPaintWithSB()
{
    createSB();
    if (IsWindow(vertSB.m_hWnd) || IsWindow(horzSB.m_hWnd)) {
        CRect wr, cr;
        CWindowDC dc(window);
        setDrawingArea(cr, wr, false); //temporarily allow full clipping window
        dc.ExcludeClipRect(&cr);
    }

    doNcPaint(window);

    hideSB(); //set back scrollbar clipping window
}

void CMPCThemeScrollBarHelper::themedNcPaint(CWnd* window, CMPCThemeScrollable* swindow)
{
    if (window->GetStyle() & WS_HSCROLL) {
        SCROLLBARINFO sbHorz = { sizeof(SCROLLBARINFO) };
        if (window->GetScrollBarInfo(OBJID_HSCROLL, &sbHorz)) {
            if (0 == (sbHorz.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_UNAVAILABLE))) {
                swindow->doDefault(); //required to get the horz sb buttons to draw in listctrl
            }
        }
    }

    doNcPaint(window);
    if (window->GetStyle() & WS_VSCROLL) {
        window->SetScrollPos(SB_VERT, window->GetScrollPos(SB_VERT), TRUE);
    }
}

void CMPCThemeScrollBarHelper::doNcPaint(CWnd* window)
{
    CWindowDC dc(window);
    int oldDC = dc.SaveDC();

    CRect wr, cr, clip, corner;
    window->GetWindowRect(wr);
    window->ScreenToClient(wr);

    int sbThickness = GetSystemMetrics(SM_CXVSCROLL);
    int borderWidth = cr.left - wr.left; //GetSystemMetrics(SM_CXSIZEFRAME) + 1;
    bool canVSB = sbThickness < wr.Width() - borderWidth * 2; //SB simply disappears if window is that small
    bool canHSB = sbThickness < wr.Height() - borderWidth * 2; //SB simply disappears if window is that small
    
    window->GetClientRect(&cr);
    int borderThickness = cr.left - wr.left;
    wr.OffsetRect(-wr.left, -wr.top);
    clip = wr; //client rect is insufficient to clip scrollbars
    clip.DeflateRect(borderThickness, borderThickness);
    dc.ExcludeClipRect(clip);
    CBrush brush(CMPCTheme::WindowBorderColorLight); //color used for column sep in explorer
    dc.FillSolidRect(wr, CMPCTheme::ContentBGColor);
    dc.FrameRect(wr, &brush);

    dc.RestoreDC(oldDC);
    if ((window->GetStyle() & (WS_VSCROLL | WS_HSCROLL)) == (WS_VSCROLL | WS_HSCROLL) && canVSB && canHSB) {
        corner = { wr.right - sbThickness - borderThickness, wr.bottom - sbThickness - borderThickness,  wr.right - borderThickness, wr.bottom - borderThickness};
        dc.FillSolidRect(corner, CMPCTheme::ContentBGColor);
    }
}

