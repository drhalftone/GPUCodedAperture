#version 330 core

uniform sampler2D qt_textureA;        // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_textureB;        // THIS TEXTURE HOLDS THE SCANS

uniform      vec2 qt_positionA;      // TOP LEFT COORDINATE OF THE LOW-PASS DWT SAMPLES
uniform      vec2 qt_positionB;      // TOP LEFT COORDINATE OF THE HIGH-PASS DWT SAMPLES

uniform     float coefficientsA[16]; // HOLDS THE LOW-PASS IDWT FILTER
uniform     float coefficientsB[16]; // HOLDS THE HIG-PASS IDWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(0,1);  // FRAGMENT COORDINATE OF SUBJECT PIXEL
    int   iterB = coord.y%2;                            // IS IT AN EVEN OR ODD IMAGE ROW

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int row = 0; row < 8; row++){
        qt_fragColor += coefficientsA[2*row + iterB] * texelFetch(qt_textureA, ivec2(coord.x, coord.y/2 - row + 7) + ivec2(qt_positionA), 0);
        qt_fragColor += coefficientsB[2*row + iterB] * texelFetch(qt_textureB, ivec2(coord.x, coord.y/2 - row + 7) + ivec2(qt_positionB), 0);
    }

    return;
}
