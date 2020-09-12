<#
    This file is a part of Jiayin's Graphics Samples.
    Copyright(c) 2020 - 2020 by Jiayin Cao - All rights reserved.
#>

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

wget http://45.63.123.194/vulkan_sdk/VulkanSDK_1_2_148_0.zip -OutFile vulkan_1_2_148_0.zip
Expand-Archive .\vulkan_1_2_148_0.zip -DestinationPath .\Dependencies\
rm .\vulkan_1_2_148_0.zip

