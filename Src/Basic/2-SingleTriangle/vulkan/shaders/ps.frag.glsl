//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Pixel shader input
layout (location = 0) in vec4 color;

// Pixel shader output
layout (location = 0) out vec4 outColor;

void main() {
    outColor = color;
}