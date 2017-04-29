#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    int col = coord.x/2 + 4*(coord.x%2);
    for (int c = 0; c < 4; c++){
        // SKEW THE CURRENT INPUT PIXELS
        qt_fragColor[c] = texelFetch(qt_texture, ivec2(col + c, coord.y), 0).r;
    }

    return;
}
