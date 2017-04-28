#version 330 core

uniform sampler2D qt_texture;    // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_mask;       // THIS TEXTURE HOLDS THE MASK

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // GET THE MASK PIXEL VALUE
    float mask = texelFetch(qt_mask, ivec2(coord.x/2, coord.y), 0).r;

    // APPLY MASK FROM INPUT TEXTURE TO OUTPUT BUFFER OBJECT
    qt_fragColor = mask * texelFetch(qt_texture, coord, 0);

    return;
}
