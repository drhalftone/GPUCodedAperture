#version 330 core

uniform sampler2D qt_texture;           // THIS TEXTURE HOLDS THE SCANS
uniform      vec2 qt_position;          // TOP LEFT COORDINATE OF THE TARGET FBO
uniform       int qt_height;            // HOLDS THE SIZE OF THE INCOMING TEXTURE

uniform     float qt_coefficientsA[16]; // HOLDS THE LOW-PASS IDWT FILTER
uniform     float qt_coefficientsB[16]; // HOLDS THE HIG-PASS IDWT FILTER
uniform      vec2 qt_offsetA;           // TOP LEFT COORDINATE OF THE LOW-PASS DWT SAMPLES
uniform      vec2 qt_offsetB;           // TOP LEFT COORDINATE OF THE HIGH-PASS DWT SAMPLES

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    ivec2 coord = ivec2(gl_FragCoord.xy - qt_position) - ivec2(0,1);  // FRAGMENT COORDINATE OF SUBJECT PIXEL
    int   iterB = (coord.y + 2)%2;                                    // IS IT AN EVEN OR ODD IMAGE ROW

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int row = 0; row < 8; row++){
        int y = (coord.y/2 - (row - 4) + qt_height)%qt_height;

        qt_fragColor += qt_coefficientsA[2*row + iterB] * texelFetch(qt_texture, ivec2(coord.x, y) + ivec2(qt_offsetA), 0);
        qt_fragColor += qt_coefficientsB[2*row + iterB] * texelFetch(qt_texture, ivec2(coord.x, y) + ivec2(qt_offsetB), 0);
    }

    return;
}
