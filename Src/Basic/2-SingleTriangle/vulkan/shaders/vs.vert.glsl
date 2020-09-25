//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Vertex input data
layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 inColor;

// VS vertex output
layout (location = 0) out vec4 outColor;

// Vertex shader entry
void main() {
    // simply pass through the vertex data
    gl_Position = vec4(pos.x, -pos.y, pos.z, 1.0f);
    outColor = inColor;
}