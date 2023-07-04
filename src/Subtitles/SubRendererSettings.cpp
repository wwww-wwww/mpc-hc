#include "stdafx.h"
#include "SubRendererSettings.h"
#include "STS.h"
#include "SubtitleHelpers.h"

bool SubRendererSettings::LibassEnabled(CSimpleTextSubtitle* sts) {
#if USE_LIBASS
    if (sts->m_subtitleType == Subtitle::SubType::SRT && renderSRTUsingLibass
        || (sts->m_subtitleType == Subtitle::SubType::SSA || sts->m_subtitleType == Subtitle::SubType::ASS) && renderSSAUsingLibass)
    return true;
#endif
    return false;
}
