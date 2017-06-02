#version 330 core

uniform sampler2D qt_texture;      // THIS TEXTURE HOLDS THE SCANS
uniform      vec2 qt_blockSize;     // THIS VECTOR HOLDS THE BLOCK SIZE IN ROWS AND COLUMNS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 block = ivec2(qt_blockSize);
    ivec2 coord = ivec2(gl_FragCoord.xy) * block;

    // ITERATE THROUGH EACH PIXEL IN THE BLOCK
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 pixelC = vec4(1.0, 1.0, 1.0, 1.0);
    for (int row = 0; row < block.y; row++){
        for (int col = 0; col < block.x; col++){

            // ACCUMULATE THE NON ZEROS ELEMENTS
           vec4 pixelA = texelFetch(qt_texture, coord + ivec2(col, row), 0);
           vec4 pixelB =  pixelC * vec4(notEqual(pixelA, vec4(0.0, 0.0, 0.0, 0.0)));
            qt_fragColor += pixelB;
        }
    }
       // qt_fragColor = vec4(1.0, 1.0, 0.5, 0.0);
       //    qt_fragColor.r = gl_FragCoord.x;
       //    qt_fragColor.g = gl_FragCoord.y;
       //     qt_fragColor.b = float (coord.x);
      //      qt_fragColor.a = float (coord.y);

     // ivec2 coord = ivec2(gl_FragCoord.xy);
     // qt_fragColor = texelFetch(qt_texture, coord, 0);
     //qt_fragColor =  vec4(1.0, 2.0, 3.0, 4.0);
     //  qt_fragColor.r = gl_FragCoord.y;
     //  qt_fragColor.g = gl_FragCoord.x;

    return;
}
