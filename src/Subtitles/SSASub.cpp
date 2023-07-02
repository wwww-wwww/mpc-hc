
#include "stdafx.h"
#include "../../include/mpc-hc_config.h"

#pragma comment( lib, "libass" )
#include <ios>
#include <algorithm>
#include <fstream>
#include <codecvt>
#include <Shlwapi.h>
#include "SSASub.h"
#include "STS.h"
#include "Utf8.h"
#include "../DSUtil/PathUtils.h"
#include <sstream>
#include "../DSUtil/DSMPropertyBag.h"
#include  <comutil.h>
#include <ppl.h>

std::string ConsumeAttribute(const char** ppsz_subtitle, std::string& attribute_value) {
    const char* psz_subtitle = *ppsz_subtitle;
    char psz_attribute_name[BUFSIZ];
    char psz_attribute_value[BUFSIZ];

    while (*psz_subtitle == ' ')
        psz_subtitle++;

    size_t attr_len = 0;
    char delimiter;

    while (*psz_subtitle && isalpha(*psz_subtitle)) {
        psz_subtitle++;
        attr_len++;
    }

    if (!*psz_subtitle || attr_len == 0)
        return std::string();

    strncpy_s(psz_attribute_name, psz_subtitle - attr_len, attr_len);
    psz_attribute_name[attr_len] = 0;

    // Skip over to the attribute value
    while (*psz_subtitle && *psz_subtitle != '=')
        psz_subtitle++;

    // Skip the '=' sign
    psz_subtitle++;

    // Aknowledge the delimiter if any
    while (*psz_subtitle && isspace(*psz_subtitle))
        psz_subtitle++;

    if (*psz_subtitle == '\'' || *psz_subtitle == '"') {
        // Save the delimiter and skip it
        delimiter = *psz_subtitle;
        psz_subtitle++;
    } else
        delimiter = 0;

    // Skip spaces, just in case
    while (*psz_subtitle && isspace(*psz_subtitle))
        psz_subtitle++;

    // Skip the first #
    if (*psz_subtitle == '#')
        psz_subtitle++;

    attr_len = 0;
    while (*psz_subtitle && ((delimiter != 0 && *psz_subtitle != delimiter) ||
        (delimiter == 0 && (isalnum(*psz_subtitle) || *psz_subtitle == '#')))) {
        psz_subtitle++;
        attr_len++;
    }

    strncpy_s(psz_attribute_value, psz_subtitle - attr_len, attr_len);
    psz_attribute_value[attr_len] = 0;
    attribute_value.assign(psz_attribute_value);

    // Finally, skip over the final delimiter
    if (delimiter != 0 && *psz_subtitle)
        psz_subtitle++;

    *ppsz_subtitle = psz_subtitle;

    return std::string(psz_attribute_name);
}

void ParseSrtLine(std::string& srtLine, const STSStyle& style) {
    const char* psz_subtitle = srtLine.data();
    std::string subtitle_output;

    while (*psz_subtitle) {
        /* HTML extensions */
        if (*psz_subtitle == '<') {
            std::string tagname = GetTag(&psz_subtitle, false);
            if (!tagname.empty()) {
                // Convert tagname to lowercase
                std::transform(tagname.begin(), tagname.end(), tagname.begin(), ::tolower);
                if (tagname == "br") {
                    subtitle_output.append("\\N");
                } else if (tagname == "b") {
                    subtitle_output.append("{\\b1}");
                } else if (tagname == "i") {
                    subtitle_output.append("{\\i1}");
                } else if (tagname == "u") {
                    subtitle_output.append("{\\u1}");
                } else if (tagname == "s") {
                    subtitle_output.append("{\\s1}");
                } else if (tagname == "font") {
                    std::string attribute_name;
                    std::string attribute_value;

                    attribute_name = ConsumeAttribute(&psz_subtitle, attribute_value);
                    while (!attribute_name.empty()) {
                        // Convert attribute_name to lowercase
                        std::transform(attribute_name.begin(), attribute_name.end(), attribute_name.begin(), ::tolower);
                        if (attribute_name == "face") {
                            subtitle_output.append("{\\fn" + attribute_value + "}");
                        } else if (attribute_name == "family") {
                        }
                        if (attribute_name == "size") {
                            double resy = style.SrtResY / 288.0;
                            int font_size = (int)std::round(std::stod(attribute_value) * resy);
                            subtitle_output.append("{\\fs" + std::to_string(font_size) + "}");
                        } else if (attribute_name == "color") {
                            MatchColorSrt(attribute_value);

                            // If color is invalid, use WHITE
                            if ((strtoul(attribute_value.c_str(), NULL, 16) == 0) && (attribute_value != "000000"))
                                attribute_value.assign("FFFFFF");

                            // HTML is RGB and we need BGR for libass
                            swapRGBtoBGR(attribute_value);
                            subtitle_output.append("{\\c&H" + attribute_value + "&}");
                        }
                        attribute_name = ConsumeAttribute(&psz_subtitle, attribute_value);
                    }
                } else {
                    // This is an unknown tag. We need to hide it if it's properly closed, and display it otherwise
                    if (!IsClosed(psz_subtitle, tagname.c_str())) {
                        //subtitle_output.append("<" + tagname + ">");
                    } else {
                    }
                    // In any case, fall through and skip to the closing tag.
                }
                // Skip potential spaces & end tag
                while (*psz_subtitle && *psz_subtitle != '>')
                    psz_subtitle++;
                if (*psz_subtitle == '>')
                    psz_subtitle++;

            } else if (!strncmp(psz_subtitle, "</", 2)) {
                tagname = GetTag(&psz_subtitle, true);
                if (!tagname.empty()) {
                    std::transform(tagname.begin(), tagname.end(), tagname.begin(), ::tolower);
                    if (tagname == "b") {
                        subtitle_output.append("{\\b0}");
                    } else if (tagname == "i") {
                        subtitle_output.append("{\\i0}");
                    } else if (tagname == "u") {
                        subtitle_output.append("{\\u0}");
                    } else if (tagname == "s") {
                        subtitle_output.append("{\\s0}");
                    } else if (tagname == "font") {
                        double resy = style.SrtResY / 288.0;
                        int font_size = (int)std::round(style.fontSize * resy);
                        subtitle_output.append("{\\c}");
                        CT2CA tmpFontName(style.fontName);
                        subtitle_output.append("{\\fn" + std::string(tmpFontName) + "}");
                        subtitle_output.append("{\\fs" + std::to_string(font_size) + "}");
                    } else {
                        // Unknown closing tag. If it is closing an unknown tag, ignore it. Otherwise, display it
                        //subtitle_output.append("</" + tagname + ">");
                    }
                    while (*psz_subtitle == ' ')
                        psz_subtitle++;
                    if (*psz_subtitle == '>')
                        psz_subtitle++;
                }
            } else {
                /* We have an unknown tag, just append it, and move on.
                * The rest of the string won't be recognized as a tag, and
                * we will ignore unknown closing tag
                */
                subtitle_output.push_back('<');
                psz_subtitle++;
            }
        }
        /* MicroDVD extensions */
        /* FIXME:
        *  - Currently, we don't do difference between X and x, and we should:
        *    Capital Letters applies to the whole text and not one line
        *  - We don't support Position and Coordinates
        *  - We don't support the DEFAULT flag (HEADER)
        */
        else if (psz_subtitle[0] == '{' && psz_subtitle[2] == ':' && strchr(&psz_subtitle[2], '}')) {
            const char* psz_tag_end = strchr(&psz_subtitle[2], '}');
            size_t i_len = psz_tag_end - &psz_subtitle[3];

            if (psz_subtitle[1] == 'Y' || psz_subtitle[1] == 'y') {
                if (psz_subtitle[3] == 'i') {
                    subtitle_output.append("{\\i1}");
                    psz_subtitle++;
                }
                if (psz_subtitle[3] == 'b') {
                    subtitle_output.append("{\\b1}");
                    psz_subtitle++;
                }
                if (psz_subtitle[3] == 'u') {
                    subtitle_output.append("{\\u1}");
                    psz_subtitle++;
                }
            } else if ((psz_subtitle[1] == 'C' || psz_subtitle[1] == 'c')
                && psz_subtitle[3] == '$' && i_len >= 7) {
                /* Yes, they use BBGGRR */
                char psz_color[7];
                psz_color[0] = psz_subtitle[4]; psz_color[1] = psz_subtitle[5];
                psz_color[2] = psz_subtitle[6]; psz_color[3] = psz_subtitle[7];
                psz_color[4] = psz_subtitle[8]; psz_color[5] = psz_subtitle[9];
                psz_color[6] = '\0';
                subtitle_output.append("{\\c&H").append(psz_color).append("&}");
            } else if (psz_subtitle[1] == 'F' || psz_subtitle[1] == 'f') {
                std::string font_name(&psz_subtitle[3], i_len);
                subtitle_output.append("{\\fn" + font_name + "}");
            } else if (psz_subtitle[1] == 'S' || psz_subtitle[1] == 's') {
                int size = atoi(&psz_subtitle[3]);
                if (size) {
                    double resy = style.SrtResY / 288.0;
                    int font_size = (int)std::round(size * resy);
                    subtitle_output.append("{\\fs" + std::to_string(font_size) + "}");
                }
            }
            // Hide other {x:y} atrocities, notably {o:x}
            psz_subtitle = psz_tag_end + 1;
        } else {
            if (*psz_subtitle == '\n' || !_strnicmp(psz_subtitle, "\\n", 2)) {
                subtitle_output.append("\\N");

                if (*psz_subtitle == '\n')
                    psz_subtitle++;
                else
                    psz_subtitle += 2;
            } else if (*psz_subtitle == '\r') {
                psz_subtitle++;
            } else {
                subtitle_output.push_back(*psz_subtitle);
                psz_subtitle++;
            }
        }
    }
    srtLine.assign(subtitle_output);
}

void srt_header(char (&outBuffer)[1024], const STSStyle& style, const SubRendererSettings& subRendererSettings) {
    double resx = style.SrtResX / 384.0;
    double resy = style.SrtResY / 288.0;

    CT2CA tmpFontName(style.fontName);

    // Generate a standard ass header
    CStringA langTagStr = "";
    if (subRendererSettings.openTypeLangHint[0]) {
        CStringA tagLang(subRendererSettings.openTypeLangHint);
        tagLang.Replace(" ", "");
        langTagStr.Format("Language: %s\n", tagLang.GetBuffer());
    }

    _snprintf_s(outBuffer, _TRUNCATE, "[Script Info]\n"
        "; Script generated by MPC-HC\n"
        "Title: MPC-HC generated file\n"
        "ScriptType: v4.00+\n"
        "WrapStyle: 0\n"
        "ScaledBorderAndShadow: %s\n"
        "Kerning: %s\n"
        "YCbCr Matrix: TV.709\n"
        "PlayResX: %u\n"
        "PlayResY: %u\n"
        "%s" /*language if set*/
        "[V4+ Styles]\n"
        "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, "
        "BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, "
        "BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n"
        "Style: Default,%s,%u,&H%X,&H%X,&H%X,&H%X,0,0,0,0,%lf,%lf,%lf,0,%u,%lf,%lf,%u,%u,%u,%u,0"
        "\n\n[Events]\n"
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n\n",
        style.ScaledBorderAndShadow ? "yes" : "no",
        style.Kerning ? "yes" : "no",
        style.SrtResX, style.SrtResY,
        (LPCSTR)langTagStr,
        std::string(tmpFontName).c_str(), (int)std::round(style.fontSize * resy), style.colors[0],
        style.colors[1], style.colors[2], style.colors[3],
        style.fontScaleX, style.fontScaleY, style.fontSpacing, (style.borderStyle == 1 ? 4 : 1), style.outlineWidthX,
        style.shadowDepthX, style.scrAlignment, (int)std::round(style.marginRect.left * resx),
        (int)std::round(style.marginRect.right * resx), (int)std::round(style.marginRect.top * resy));
}

ASS_Track* srt_read_file(ASS_Library* library, CStringW fname, const UINT codePage, const STSStyle& style, const SubRendererSettings& subRendererSettings) {
    std::ifstream srtFile(fname, std::ios::in);
    ASS_Track* track = ass_new_track(library);
    track->name = _strdup(UTF16To8(fname));
    return srt_read_data(library, track, srtFile, codePage, style, subRendererSettings);
}

ASS_Track* srt_read_data(ASS_Library* library, ASS_Track* track, std::istream &stream, const UINT codePage, const STSStyle& style, const SubRendererSettings& subRendererSettings) {
    // Convert SRT to ASS
    std::string lineIn;
    std::string lineOut;
    char inBuffer[1024];
    char outBuffer[1024];
    int start[4], end[4];

    srt_header(outBuffer, style, subRendererSettings);
    ass_process_data(track, outBuffer, static_cast<int>(strnlen_s(outBuffer, sizeof(outBuffer))));

    char BOM[4];
    stream.read(BOM, 3);
    if (stream.fail()) {
        return track;
    }
    bool streamIsUTF8 = true;
    if (BOM[0] == 0xEF && BOM[1] == 0xBB && BOM[2] == 0xBF) { //utf-8 BOM is here for some reason, we don't need it
        //we seeked past it
    } else {
        stream.seekg(std::ios_base::beg);
    }

    while (!stream.eof()) {
        stream.getline(inBuffer, sizeof(inBuffer) - 1);
        lineIn.assign(inBuffer);
        if (lineIn.empty())
            continue;

        if (streamIsUTF8) {
            streamIsUTF8 &= Utf8::isStringValid((const unsigned char*)lineIn.c_str(), lineIn.length());
        }

        // Read the timecodes
        if (sscanf_s(inBuffer, "%d:%2d:%2d%*1[,.]%3d --> %d:%2d:%2d%*1[,.]%3d", &start[0], &start[1],
            &start[2], &start[3], &end[0], &end[1], &end[2], &end[3]) == 8) {
            lineOut.clear();
            stream.getline(inBuffer, sizeof(inBuffer) - 1);
            lineIn.assign(inBuffer);
            while (!lineIn.empty()) {
                if (streamIsUTF8) {
                    streamIsUTF8 &= Utf8::isStringValid((const unsigned char*)lineIn.c_str(), lineIn.length());
                }
                lineOut.append(lineIn);
                stream.getline(inBuffer, sizeof(inBuffer) - 1);
                lineIn.assign(inBuffer);
                if (!lineIn.empty())
                    lineOut.append("\\N");
            }
            //lineOut.append("\n");

            if (!streamIsUTF8) {
                // Convert to UTF-8 only if UTF-8 not detected
                if (codePage != 0)
                    ConvertCPToUTF8(codePage, lineOut);
            }

            ParseSrtLine(lineOut, style);

            CT2CA tmpCustomTags(style.customTags);
            _snprintf_s(outBuffer, _TRUNCATE, "Dialogue: 0,%d:%02d:%02d.%02d,%d:%02d:%02d.%02d,Default,,0,0,0,,{\\blur%u}%s%s",
                start[0], start[1], start[2],
                (int)floor((double)start[3] / 10.0), end[0], end[1],
                end[2], (int)floor((double)end[3] / 10.0), style.fBlur, std::string(tmpCustomTags).c_str(), lineOut.c_str());
            ass_process_data(track, outBuffer, static_cast<int>(strnlen_s(outBuffer, sizeof(outBuffer))));
        }
    }
    ass_process_force_style(track);
    return track;
}

ASS_Track* ass_read_fileW(ASS_Library* library, CStringW fname) {
    std::ifstream t(fname, std::ios::in);
    std::stringstream buffer;
    buffer << t.rdbuf();
    return ass_read_memory(library, (char*)buffer.str().c_str(), buffer.str().size(), "UTF-8");
}

void ConvertCPToUTF8(UINT CP, std::string& codepage_str) {
    int size = MultiByteToWideChar(CP, MB_PRECOMPOSED, codepage_str.c_str(),
        (int)codepage_str.length(), nullptr, 0);

    std::wstring utf16_str(size, '\0');
    MultiByteToWideChar(CP, MB_PRECOMPOSED, codepage_str.c_str(),
        (int)codepage_str.length(), &utf16_str[0], size);

    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(),
        (int)utf16_str.length(), nullptr, 0,
        nullptr, nullptr);

    std::string utf8_str(utf8_size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, utf16_str.c_str(),
        (int)utf16_str.length(), &utf8_str[0], utf8_size,
        nullptr, nullptr);
    codepage_str.assign(utf8_str);
}

std::string GetTag(const char** line, bool b_closing) {
    const char* psz_subtitle = *line;

    if (*psz_subtitle != '<')
        return std::string();

    // Skip the '<'
    psz_subtitle++;

    if (b_closing && *psz_subtitle == '/')
        psz_subtitle++;

    // Skip potential spaces
    while (*psz_subtitle == ' ')
        psz_subtitle++;

    // Now we need to verify if what comes next is a valid tag:
    if (!isalpha(*psz_subtitle))
        return std::string();

    size_t tag_size = 1;
    while (isalnum(psz_subtitle[tag_size]) || psz_subtitle[tag_size] == '_')
        tag_size++;

    char psz_tagname[BUFSIZ];
    strncpy_s(psz_tagname, psz_subtitle, tag_size);
    psz_tagname[tag_size] = 0;
    psz_subtitle += tag_size;
    *line = psz_subtitle;

    return std::string(psz_tagname);
}

bool IsClosed(const char* psz_subtitle, const char* psz_tagname) {
    const char* psz_tagpos = StrStrIA(psz_subtitle, psz_tagname);

    if (!psz_tagpos)
        return false;

    // Search for '</' and '>' immediatly before & after (minding the potential spaces)
    const char* psz_endtag = psz_tagpos + strlen(psz_tagname);

    while (*psz_endtag == ' ')
        psz_endtag++;

    if (*psz_endtag != '>')
        return false;

    // Skip back before the tag itself
    psz_tagpos--;
    while (*psz_tagpos == ' ' && psz_tagpos > psz_subtitle)
        psz_tagpos--;

    if (*psz_tagpos-- != '/')
        return false;

    if (*psz_tagpos != '<')
        return false;

    return true;
}

void MatchColorSrt(std::string& fntColor) {
    for (auto i = 0; i < _countof(color_tag); ++i) {
        if (strcmp(fntColor.c_str(), color_tag[i].color) == 0) {
            fntColor.assign(color_tag[i].hex);
            break;
        }
    }
}

std::wstring s2ws(const std::string& str) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

namespace {
    inline POINT GetRectPos(RECT rect) {
        return { rect.left, rect.top };
    }

    inline SIZE GetRectSize(RECT rect) {
        return { rect.right - rect.left, rect.bottom - rect.top };
    }
}


SSAUtil::SSAUtil(CSimpleTextSubtitle* sts)
    : m_STS(sts)
    , m_renderUsingLibass(false)
    , m_openTypeLangHint()
    , m_assloaded(false)
    , m_assfontloaded(false)
    , m_pGraph(nullptr)
    , m_pPin(nullptr)
    , m_ass(nullptr)
    , m_renderer(nullptr)
    , m_track(nullptr)
{
    LoadDefStyle();
}

SSAUtil::~SSAUtil() {
    Unload();
}

void SSAUtil::SetSubRenderSettings(SubRendererSettings settings) {
    bool wasUsingLibass = subRendererSettings.LibassEnabled(m_STS);
    subRendererSettings = settings;
    if (settings.LibassEnabled(m_STS) || wasUsingLibass) {
        ResetASS();
    }
}

void SSAUtil::DefaultStyleChanged() {
    LoadDefStyle();
    ResetASS();
}


void SSAUtil::ResetASS() {
    if (subRendererSettings.LibassEnabled(m_STS)) { 
        m_renderUsingLibass = true;
        if (!m_STS->m_path.IsEmpty()) {
            LoadASSFile(m_STS->m_subtitleType);
        } else if (!m_trackData.empty()) {
            LoadASSTrack((char*)m_trackData.c_str(), m_trackData.length(), m_STS->m_subtitleType);
        }
    } else {
        if (m_assloaded) {
            Unload();
        }
        m_renderUsingLibass = false;
    }
}

void SSAUtil::InitLibASS() {
    Unload();
    m_assfontloaded = false;
    m_ass = decltype(m_ass)(ass_library_init());
    ass_set_fonts_dir(m_ass.get(), NULL); //initialize it or we get free() errors in debug mode
    m_renderer = decltype(m_renderer)(ass_renderer_init(m_ass.get()));
    m_track = decltype(m_track)(ass_new_track(m_ass.get()));
}

bool SSAUtil::LoadASSFile(Subtitle::SubType subType) {
    if (m_STS->m_path.IsEmpty() || !PathUtils::Exists(m_STS->m_path)) return false;

    InitLibASS();
    ass_set_extract_fonts(m_ass.get(), true);
    ass_set_style_overrides(m_ass.get(), NULL);

    m_renderer = decltype(m_renderer)(ass_renderer_init(m_ass.get()));
    ass_set_use_margins(m_renderer.get(), false);
    ass_set_font_scale(m_renderer.get(), 1.0);

    if (subType == Subtitle::SRT) {
        m_track = decltype(m_track)(srt_read_file(m_ass.get(), m_STS->m_path, defStyle.charSet, defStyle, subRendererSettings));
        if (m_STS->m_storageRes == CSize(0, 0)) {
            m_STS->m_storageRes = CSize(defStyle.SrtResX, defStyle.SrtResY);
        }
    } else { //subType == Subtitle::SSA/ASS
        m_track = decltype(m_track)(ass_read_fileW(m_ass.get(), m_STS->m_path));
        if (m_STS->m_storageRes == CSize(0, 0)) {
            m_STS->m_storageRes = CSize(defStyle.SrtResX, defStyle.SrtResY);
        }
    }

    if (!m_track) return false;

    CT2CA tmpFontName(defStyle.fontName);
    ass_set_fonts(m_renderer.get(), NULL, std::string(tmpFontName).c_str(), ASS_FONTPROVIDER_AUTODETECT, NULL, 0);

    m_assloaded = true;
    m_assfontloaded = true;

    return true;
}

bool SSAUtil::LoadASSTrack(char* data, int size, Subtitle::SubType subType) {
    InitLibASS();

    if (!m_track) return false;

    if (subType == Subtitle::SRT) {
        std::stringstream srtData;
        srtData.write(data, size);
        srt_read_data(m_ass.get(), m_track.get(), srtData, defStyle.charSet, defStyle, subRendererSettings);
    } else { //subType == Subtitle::SSA/ASS
        LoadTrackData(m_track.get(), data, size);
    }
    CT2CA tmpFontName(defStyle.fontName);
    ass_set_fonts(m_renderer.get(), NULL, std::string(tmpFontName).c_str(), ASS_FONTPROVIDER_AUTODETECT, NULL, 0);
    //don't set m_assfontloaded here, in case we can load embedded fonts later?

    m_assloaded = true;
    return true;
}

void SSAUtil::LoadASSFont() {
    if (m_assfontloaded || !m_pPin) return;
    ASS_Library* ass = m_ass.get();
    ASS_Renderer* renderer = m_renderer.get();
    // Try to load fonts in the container
    CComPtr<IAMGraphStreams> graphStreams;
    CComPtr<IDSMResourceBag> bag;
    if (m_pGraph && SUCCEEDED(m_pGraph->QueryInterface(IID_PPV_ARGS(&graphStreams))) &&
        SUCCEEDED(graphStreams->FindUpstreamInterface(m_pPin, IID_PPV_ARGS(&bag), AM_INTF_SEARCH_FILTER))) {
        for (DWORD i = 0; i < bag->ResGetCount(); ++i) {
            _bstr_t name, desc, mime;
            BYTE* pData = nullptr;
            DWORD len = 0;
            if (SUCCEEDED(bag->ResGet(i, &name.GetBSTR(), &desc.GetBSTR(), &mime.GetBSTR(), &pData, &len, nullptr))) {
                if (wcscmp(mime.GetBSTR(), L"application/x-truetype-font") == 0 ||
                    wcscmp(mime.GetBSTR(), L"application/vnd.ms-opentype") == 0) // TODO: more mimes?
                {
                    ass_add_font(ass, (char*)name, (char*)pData, len);
                    // TODO: clear these fonts somewhere?
                }
                CoTaskMemFree(pData);
            }
        }
        m_assfontloaded = true;
        CT2CA tmpFontName(defStyle.fontName);
        ass_set_fonts(renderer, NULL, std::string(tmpFontName).c_str(), ASS_FONTPROVIDER_AUTODETECT, NULL, 0);
    }
}

CRect SSAUtil::GetSPDRect(SubPicDesc& spd) {
    CRect spdRect;
    if (m_STS->m_subtitleType == Subtitle::SubType::SRT && defStyle.relativeTo != STSStyle::VIDEO
        || defStyle.relativeTo == STSStyle::WINDOW) {
        spdRect = CRect(0, 0, spd.w, spd.h);
    } else {
        spdRect = CRect(spd.vidrect);
    }
    return spdRect;
}

STDMETHODIMP SSAUtil::Render(REFERENCE_TIME rt, SubPicDesc& spd, RECT& bbox, CSize& size, CRect& vidRect) {
    if (m_assloaded) {
        if (spd.bpp != 32) {
            ASSERT(FALSE);
            return E_INVALIDARG;
        }

        LoadASSFont();

        vidRect = GetSPDRect(spd);
        size = CSize(vidRect.Width(), vidRect.Height());
        SetFrameSize(vidRect.Width(), vidRect.Height());

        CRect rcDirty;

        if (!RenderFrame(rt / 10000, spd, rcDirty)) {
            return E_FAIL;
        }

        bbox = rcDirty;
        return S_OK;
    }
    return E_POINTER;
}

bool SSAUtil::RenderFrame(long long now, SubPicDesc& spd, CRect& rcDirty) {
    int changed = 1;
    ASS_Image* image = ass_render_frame(m_renderer.get(), m_track.get(), now, &changed);
    if (!image) return false;
    if (changed)
        AssFlatten(image, spd, rcDirty);
    return true;
}

void SSAUtil::AssFlatten(ASS_Image* image, SubPicDesc& spd, CRect& rcDirty) {
    if (image) {
        CRect pRect;
        for (auto i = image; i != nullptr; i = i->next) {
            CRect rect2(i->dst_x, i->dst_y, i->dst_x + i->w, i->dst_y + i->h);
            pRect.UnionRect(pRect, rect2);
        }
        CRect spdRect = GetSPDRect(spd);
        rcDirty.IntersectRect(pRect + spdRect.TopLeft(), spdRect);

        BYTE* pixelBytes = (BYTE*)(spd.bits + spd.pitch * rcDirty.top + rcDirty.left * 4);

        for (auto i = image; i != nullptr; i = i->next) {
            uint32_t iA = 0xff - (i->color & 0x000000ff);
            uint32_t iR = (i->color & 0xff000000) >> 24;
            uint32_t iG = (i->color & 0x00ff0000) >> 16;
            uint32_t iB = (i->color & 0x0000ff00) >> 8;

            auto yOff1 = (ptrdiff_t)i->dst_y - pRect.top;
            concurrency::parallel_for(0, i->h, [&](int y)
                {
                    auto yOff = (yOff1 + y)*spd.pitch;
                    auto yOffStride = y * i->stride;
                    auto xOff1 = ((ptrdiff_t)i->dst_x - pRect.left) * 4;
                    for (int x = 0; x < i->w; ++x, ++yOffStride, xOff1+=4) {
                        BYTE* dst = &pixelBytes[yOff + xOff1];

                        uint32_t srcA = (i->bitmap[yOffStride] * iA) >> 8;
                        uint32_t compA = 0xff - srcA;

                        dst[3] = 0xff - (srcA + (((0xff - dst[3]) * compA) >> 8)); //A.  this is inverted alpha, so we invert it before multiplying and then invert it again
                        dst[2] = (iR * srcA + dst[2] * compA) >> 8; //R
                        dst[1] = (iG * srcA + dst[1] * compA) >> 8; //G
                        dst[0] = (iB * srcA + dst[0] * compA) >> 8; //B

                    }
                }, concurrency::static_partitioner());
        }
    }
}

void SSAUtil::SetFrameSize(int w, int h) {
    ass_set_frame_size(m_renderer.get(), w, h);
}

void SSAUtil::Unload() {
    m_assloaded = false;
    if (m_track) m_track.reset();
    if (m_renderer) m_renderer.reset();
    if (m_ass) {
        m_ass.reset();
    }
}

void SSAUtil::LoadASSSample(char *data, int dataSize, REFERENCE_TIME tStart, REFERENCE_TIME tStop) {
    if (m_renderUsingLibass) {
        if (m_STS->m_subtitleType == Subtitle::SRT) { //received SRT sample, try to use libass to handle
            if (!m_assloaded) { //create ass header
                InitLibASS();

                char outBuffer[1024];
                srt_header(outBuffer, defStyle, subRendererSettings);
                LoadTrackData(m_track.get(), outBuffer, static_cast<int>(strnlen_s(outBuffer, sizeof(outBuffer))));
                m_assloaded = true;
            }

            if (m_assloaded) {
                char subLineData[1024]{};
                strncpy_s(subLineData, _countof(subLineData), data, dataSize);
                std::string str = subLineData;

                // This is the way i use to get a unique id for the subtitle line
                // It will only fail in the case there is 2 or more lines with the same start timecode
                // (Need to check if the matroska muxer join lines in such a case)
                REFERENCE_TIME m_iSubLineCount = tStart / 10000;

                // Change srt tags to ass tags
                ParseSrtLine(str, defStyle);

                // Add the custom tags
                CT2CA tmpCustomTags(defStyle.customTags);
                str.insert(0, std::string(tmpCustomTags));

                // Add blur
                char blur[20]{};
                _snprintf_s(blur, _TRUNCATE, "{\\blur%u}", defStyle.fBlur);
                str.insert(0, blur);

                // ASS in MKV: ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect, Text
                char outBuffer[1024]{};
                _snprintf_s(outBuffer, _TRUNCATE, "%lld,0,Default,Main,0,0,0,,%s", m_iSubLineCount, str.c_str());
                ass_process_chunk(m_track.get(), outBuffer, static_cast<int>(strnlen_s(outBuffer, sizeof(outBuffer))), tStart / 10000, (tStop - tStart) / 10000);
            }
        }
    }
}

void SSAUtil::LoadTrackData(ASS_Track* track, char* data, int size) {
    m_trackData = data;
    ass_process_codec_private(m_track.get(), data, size);
}

void SSAUtil::LoadDefStyle() {
    if (m_STS) {
        m_STS->GetDefaultStyle(defStyle);
    }
}
