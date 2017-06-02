#version 330 core

uniform sampler2D qt_textureA;      // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_textureB;      // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy) ;

    // ITERATE THROUGH EACH PIXEL IN THE BLOCK
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);

            // CALCULATE THE DIFFERENCE BETWEEN PIXELS
            vec4 pixelA = texelFetch(qt_textureA, coord, 0);
            vec4 pixelB = texelFetch(qt_textureB, coord, 0);

            qt_fragColor = min(pixelA, pixelB);

    return;
}
