//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

#pragma once

#include <stdlib.h>

struct float3 {
    float x, y, z;
};

/*
 * Each vertex only has position and color in it. For simplicity, position data are already defined in NDC, no transformation is
 * needed in vertex shader anymore.
 */
struct Vertex {
    float3 position;    // clip space position
    float3 color;       // a color for each vertex
};

/*
 * There are only three vertices, it is a triangle in the middle of the screen.
 */
static Vertex g_vertices[] = {
    { {-0.35f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} },
    { { 0.35f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} },
    { { 0.0f,  0.5f,  0.0f}, {1.0f, 0.0f, 0.0f} },
};

/*
 * Indices data of the vertex buffer.
 */
static unsigned int g_indices[] = { 0, 1, 2 };

constexpr unsigned int g_indices_cnt = _countof(g_indices);
constexpr unsigned int g_vertices_cnt = _countof(g_vertices);

// vertex size
constexpr unsigned int g_vertex_size = sizeof(Vertex);
constexpr unsigned int g_total_vertices_size = g_vertex_size * g_vertices_cnt;
constexpr unsigned int g_index_size = sizeof(unsigned int);
constexpr unsigned int g_total_indices_size = g_index_size * g_indices_cnt;