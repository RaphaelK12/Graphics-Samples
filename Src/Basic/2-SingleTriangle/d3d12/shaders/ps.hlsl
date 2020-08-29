//
//  This file is a part of Jiayin's Graphics Samples.
//  Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
//

struct VSOutput{
    float4 color    : COLOR;
};

/*
 * Pixel shader
 * It does nothing but to output the input color.
 */
float4 main(VSOutput ps_in) : SV_Target{
    return ps_in.color;
}