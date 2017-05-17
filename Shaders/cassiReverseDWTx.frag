#version 330 core

uniform sampler2D qt_textureA;        // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_textureB;        // THIS TEXTURE HOLDS THE SCANS

uniform     float coefficientsA[16]; // HOLDS THE LOW-PASS IDWT FILTER
uniform     float coefficientsB[16]; // HOLDS THE HIG-PASS IDWT FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    ivec2 coord = ivec2(gl_FragCoord.xy) + ivec2(2,0);  // FRAGMENT COORDINATE OF SUBJECT PIXEL
    int   iterA = coord.x%2;                            // IS IT CHANNEL 0:3 OR 4:7
    int   iterB = (coord.x/2)%2;                        // IS IT AN EVEN OR ODD IMAGE COLUMN

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int col = 0; col < 8; col++){
        qt_fragColor += coefficientsA[2*col + iterB] * texelFetch(qt_textureA, ivec2((coord.x/4)*2 + iterA - 2*(col - 7), coord.y), 0);
        qt_fragColor += coefficientsB[2*col + iterB] * texelFetch(qt_textureB, ivec2((coord.x/4)*2 + iterA - 2*(col - 7), coord.y), 0);
    }

    return;
}
