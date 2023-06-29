#pragma once
#include "OpenTypeLangTags.h"

class CSimpleTextSubtitle;

class SubRendererSettings {
public:
    SubRendererSettings() {}
    virtual ~SubRendererSettings() {}
    bool renderSSAUsingLibass = false;
    bool renderSRTUsingLibass = false;
    bool LibassEnabled(CSimpleTextSubtitle* sts);
    OpenTypeLang::HintStr openTypeLangHint = { 0 };
};
