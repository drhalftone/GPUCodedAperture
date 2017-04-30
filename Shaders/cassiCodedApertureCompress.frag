#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // GET CURRENT PIXEL FROM MONOCHROME IMAGE
    vec4 pixelA = texelfetch(qt_texture, ivec2(2*coord.x + 0, coord.y), 0);
    vec4 pixelB = texelfetch(qt_texture, ivec2(2*coord.x + 1, coord.y), 0);

    // SUM TOGETHER THE COMPONENTS FROM PIXEL A AND B TO GET SCALAR VALUE
    qt_fragColor.r = pixelA.r + pixelA.g + pixelA.b + pixelA.a + pixelB.r + pixelB.g + pixelB.b + pixelB.a;
    qt_fragColor.g = 0.0;
    qt_fragColor.b = 0.0;
    qt_fragColor.a = 0.0;

    return;
}
