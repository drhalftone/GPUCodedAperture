#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform       int offset;            // SHIFTS THE OUTPUT PIXEL TO INPUT PIXEL COORDINATE
uniform       int rangeLimit;        // MAKE SURE WE DON'T GO BEYOND BOUNDS OF INPUT TEXTURE
uniform     float coefficients[16];  // HOLDS THE DWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord   = ivec2(gl_FragCoord.xy);
          coord.x = 2 * (coord.x - offset) - coord.x%2;

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int col = -8; col < 8; col++){
        qt_fragColor += coefficients[col + 8] * texelFetch(qt_texture, ivec2(coord.x - 2*col, coord.y), 0) * float((coord.x - 2*col) < rangeLimit);
    }

    return;
}
