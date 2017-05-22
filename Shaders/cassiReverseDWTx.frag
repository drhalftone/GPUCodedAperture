#version 330 core

uniform sampler2D qt_texture;           // THIS TEXTURE HOLDS THE SCANS
uniform       int qt_width;             // HOLDS THE SIZE OF THE INCOMING TEXTURE

uniform      vec2 qt_offsetA;           // TOP LEFT COORDINATE OF THE LOW-PASS DWT SAMPLES
uniform      vec2 qt_offsetB;           // TOP LEFT COORDINATE OF THE HIGH-PASS DWT SAMPLES
uniform     float qt_coefficientsA[16]; // HOLDS THE LOW-PASS IDWT FILTER
uniform     float qt_coefficientsB[16]; // HOLDS THE HIG-PASS IDWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    ivec2 coord = ivec2(gl_FragCoord.xy) - ivec2(2,0); // FRAGMENT COORDINATE OF SUBJECT PIXEL
    int   iterA = (coord.x+2)%2;                       // IS IT CHANNEL 0:3 OR 4:7
    int   iterB = ((coord.x+4)/2)%2;                   // IS IT AN EVEN OR ODD IMAGE COLUMN

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int col = 0; col < 8; col++){
        int x = ((coord.x/4)*2 + iterA - 2*(col - 4) + qt_width)%qt_width;

        qt_fragColor += qt_coefficientsA[2*col + iterB] * texelFetch(qt_texture, ivec2(x, coord.y) + ivec2(qt_offsetA), 0);
        qt_fragColor += qt_coefficientsB[2*col + iterB] * texelFetch(qt_texture, ivec2(x, coord.y) + ivec2(qt_offsetB), 0);
    }

    return;
}
