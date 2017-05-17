#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform     float coefficients[16];  // HOLDS THE DWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);
          coord.x = 2*coord.x + coord.x%2;

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int col = 0; col < 16; col++){
        qt_fragColor += coefficients[col] * texelFetch(qt_texture, ivec2(coord.x - 2*col, coord.y), 0);
    }

    return;
}
