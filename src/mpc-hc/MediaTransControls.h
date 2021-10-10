/*
* (C) 2002-2021 see Authors.txt
*
* This file is part of MPC-HC.
*
* MPC-HC is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* MPC-HC is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once
#include <sdkddkver.h>
#include <atlbase.h>
#include <windows.media.h>

class CMainFrame;

class MediaTransControls {
public:
    MediaTransControls(void) {
        this->controls = nullptr;
        this->updater = nullptr;
        this->video = nullptr;
    }
    ~MediaTransControls(void) {
        if (controls && m_EventRegistrationToken.value) {
            controls->remove_ButtonPressed(m_EventRegistrationToken);
        }
    }
    /**
     * @brief Intitialize the interface
     * @param main 
     * @return 
    */
    bool Init(CMainFrame* main);
    /**
     * @brief Set status to stoped and clear all metadata infromations.
    */
    void stop();
    /**
     * @brief Change status to play status.
    */
    void play();
    CComPtr<ABI::Windows::Media::ISystemMediaTransportControls> controls;
    CComPtr<ABI::Windows::Media::ISystemMediaTransportControlsDisplayUpdater> updater;
    CComPtr<ABI::Windows::Media::IVideoDisplayProperties> video;
    CComPtr<ABI::Windows::Media::IMusicDisplayProperties> audio;
    void loadThumbnail(CString fn);
    void loadThumbnail(BYTE* content, size_t size);
    void loadThumbnailFromUrl(CString url);
    bool IsActive();
protected:
    CMainFrame* m_pMainFrame;
    EventRegistrationToken m_EventRegistrationToken;
    void OnButtonPressed(ABI::Windows::Media::SystemMediaTransportControlsButton button);
};
