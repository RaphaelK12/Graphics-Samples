//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#pragma once

#include <Windows.h>

class GraphicsSample {
public:
    /*
     * Shutdown the graphics api.
     */
    virtual ~GraphicsSample() {}

    /*
     * Initialize graphics API.
     */
    virtual bool initialize(const HINSTANCE hInstnace, const HWND hwnd) = 0;

    /*
     * Render a frame. 
     */
    virtual void render_frame() = 0;

    /*
     * Shutdown the graphics API.
     */
    virtual void shutdown() = 0;
};