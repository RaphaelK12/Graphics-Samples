//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#pragma once

#include "../sample.h"

class VulkanGraphicsSample : public GraphicsSample {
public:
    /*
     * Initialize graphics API.
     */
    bool initialize(const HINSTANCE hInstnace, const HWND hwnd) override;

    /*
     * Render a frame.
     */
    void render_frame() override;

    /*
     * Shutdown the graphics API.
     */
    void shutdown() override;
};