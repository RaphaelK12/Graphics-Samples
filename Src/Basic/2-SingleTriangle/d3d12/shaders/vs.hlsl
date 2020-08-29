//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

struct Vertex{
    float3 position : POSITION;     // clip space position
    float3 color    : COLOR;
};

struct VSOutput{
    float4 color    : COLOR;
    float4 position : SV_Position;
};

/*
 * Vertex Shader
 * Nothing needs to be done apart from passthrough VS input data.
 */
VSOutput main(Vertex vs_in){
    VSOutput vs_out;

    vs_out.position = float4(vs_in.position, 1.0f);
    vs_out.color = float4(vs_in.color, 1.0f);

    return vs_out;
}