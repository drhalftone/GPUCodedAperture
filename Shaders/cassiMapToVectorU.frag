#version 330 core

uniform sampler2D qt_texture;   // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    qt_fragColor = texelFetch(qt_texture, ivec2(gl_FragCoord.xy), 0);
    qt_fragColor = qt_fragColor * vec4(greaterThan(qt_fragColor, vec4(0.0, 0.0, 0.0, 0.0)));

    return;
}
