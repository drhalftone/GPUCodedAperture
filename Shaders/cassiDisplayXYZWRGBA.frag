#version 330 core

uniform sampler2D qt_texture;      // THIS TEXTURE HOLDS THE SCANS
uniform       int qt_channel;      // SAYS WHICH CHANNEL TO SHOW

in           vec2 qt_coordinate;   // HOLDS THE TEXTURE COORDINATE FROM THE VERTEX SHADER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(qt_coordinate * textureSize(qt_texture, 0));

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = texelFetch(qt_texture, ivec2(2 * (coord.x/2) + int(qt_channel>3), coord.y), 0);
    qt_fragColor.rgba = vec4(1.0, 1.0, 1.0, 1.0) * qt_fragColor[qt_channel%4];

    return;
}
