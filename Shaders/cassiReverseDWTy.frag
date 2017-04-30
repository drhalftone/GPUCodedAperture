#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform       int offset;            // SHIFTS THE OUTPUT PIXEL TO INPUT PIXEL COORDINATE
uniform     float coefficientsA[16]; // HOLDS THE LOW-PASS IDWT FILTER
uniform     float coefficientsB[16]; // HOLDS THE HIG-PASS IDWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    ivec2 coord = ivec2(gl_FragCoord.xy) - ivec2(0,1); // FRAGMENT COORDINATE OF SUBJECT PIXEL
    int   iterB = coord.y%2;                           // IS IT AN EVEN OR ODD IMAGE ROW

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int row = -4; row < 4; row++){
        qt_fragColor += coefficientsA[2*row + 8 + iterB] * texelFetch(qt_texture, ivec2(coord.x, coord.y/2 - row), 0);
        qt_fragColor += coefficientsB[2*row + 8 + iterB] * texelFetch(qt_texture, ivec2(coord.x, coord.y/2 + offset - row), 0);
    }

    return;
}
