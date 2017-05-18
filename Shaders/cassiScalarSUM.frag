#version 330 core

uniform sampler2D qt_textureA;      // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_textureB;      // THIS TEXTURE HOLDS THE SCANS
uniform      vec2 qt_blockSize;     // THIS VECTOR HOLDS THE BLOCK SIZE IN ROWS AND COLUMNS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 block = ivec2(qt_blockSize);
    ivec2 coord = ivec2(gl_FragCoord.xy) * block;

    // ITERATE THROUGH EACH PIXEL IN THE BLOCK
    qt_fragColor = ivec4(0.0, 0.0, 0.0, 0.0);
    for (int row = 0; row < block.y; row++){
        for (int col = 0; col < block.x; col++){
            // CALCULATE THE SUM OF PIXELS
            vec4 pixelA = texelFetch(qt_textureA, coord + ivec2(col, row), 0);
            vec4 pixelB = texelFetch(qt_textureB, coord + ivec2(col, row), 0);
            vec4 pixelC = pixelA + pixelB;

            // ACCUMULATE THE SUM OF THE SQUARE OF THE DIFFERENCE VECTOR
            qt_fragColor.r += pixelC.r + pixelC.g + pixelC.b + pixelC.a;
        }
    }

    return;
}
