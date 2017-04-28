#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // GET CURRENT PIXEL FROM MONOCHROME IMAGE
    qt_fragColor = texelfetch(qt_texture, ivec2(coord.x/2, coord.y), 0).rrrr;

    return;
}
