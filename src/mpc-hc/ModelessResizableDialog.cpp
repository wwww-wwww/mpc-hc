#include "stdafx.h"
#include "ModelessResizableDialog.h"
#include "SysVersion.h"

BEGIN_MESSAGE_MAP(CModelessResizableDialog, CMPCThemeResizableDialog)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

CModelessResizableDialog::CModelessResizableDialog(UINT nIDTemplate, CWnd* pParent) : CMPCThemeResizableDialog(nIDTemplate, pParent) {
}

void CModelessResizableDialog::HideDialog(INT_PTR ret) {
    // Just hide the dialog, since it's modeless we don't want to call EndDialog
    if (SysVersion::IsWin10orLater()) {
        //windows 11 bug with peek preview--shows hidden dialogs.  temporarily flag as tool window which is not a taskbar eligble window
        ModifyStyleEx(0, WS_EX_TOOLWINDOW);
        EndDialog(ret);
        //remove bogus style so it renders properly next time
        ModifyStyleEx(WS_EX_TOOLWINDOW, 0);
    } else {
        EndDialog(ret);
    }
}

void CModelessResizableDialog::OnOK() {
    HideDialog(IDOK);
}

void CModelessResizableDialog::OnCancel() {
    HideDialog(IDCANCEL);
}
