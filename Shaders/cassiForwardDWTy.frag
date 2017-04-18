#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform       int offset;            // ARE WE PROCESSING THE LEFT OR RIGHT HALF OF THE OUTPUT IMAGE
uniform       int rangeLimit;        // MAKE SURE WE DON'T GO BEYOND BOUNDS OF INPUT TEXTURE
uniform     float coefficients[16];  // HOLDS THE DWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord   = ivec2(gl_FragCoord.xy);
          coord.y = 2 * (coord.y - offset);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int row = -8; row < 8; row++){
        qt_fragColor += coefficients[row + 8] * texelFetch(qt_texture, ivec2(coord.x, coord.y - row), 0) * float((coord.y - row) < rangeLimit);
    }

    return;
}
