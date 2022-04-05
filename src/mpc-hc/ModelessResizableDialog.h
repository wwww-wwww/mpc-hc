#pragma once
#include "CMPCThemeResizableDialog.h"
class CModelessResizableDialog : public CMPCThemeResizableDialog {
public:
    CModelessResizableDialog(UINT nIDTemplate, CWnd* pParent);
    void HideDialog(INT_PTR ret);
    DECLARE_MESSAGE_MAP()
protected:
    virtual void OnOK();
    virtual void OnCancel();
};

