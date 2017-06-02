#version 330 core

uniform sampler2D qt_texture;      // THIS TEXTURE HOLDS THE SCANS
uniform float         scalar;      // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy) ;

    // ITERATE THROUGH EACH PIXEL IN THE BLOCK
    qt_fragColor = scalar * texelFetch(qt_texture, coord, 0);

    return;
}
