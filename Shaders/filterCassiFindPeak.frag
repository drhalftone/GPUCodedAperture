#version 330 core

in vec4 qt_fragment;      // THIS VECTOR HOLDS A SPECIFIC POINT LOCATION IN SCAN SPACE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    qt_fragColor = qt_fragment;
    return;
}
