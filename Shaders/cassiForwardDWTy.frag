#version 330 core

uniform sampler2D qt_texture;           // THIS TEXTURE HOLDS THE SCANS
uniform     float qt_coefficients[16];  // HOLDS THE DWT FILTER
uniform      vec2 qt_position;          // TOP LEFT CORNER OF FBO
uniform      vec2 qt_offset;            // TOP LEFT CORNER OF INPUT TEXTURE
uniform       int qt_height;            // HOLDS THE SIZE OF THE INCOMING TEXTURE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord   = ivec2(gl_FragCoord.xy - qt_position);
          coord.y = 2*coord.y;

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int row = 0; row < 16; row++){
        qt_fragColor += qt_coefficients[row] * texelFetch(qt_texture, ivec2(coord.x, (coord.y - (row - 8) + 16*qt_height)%qt_height) + ivec2(qt_offset), 0);
    }

    return;
}
