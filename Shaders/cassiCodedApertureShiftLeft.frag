#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    int index = coord.x + 8*(coord.x%2);
    for (int col = 0; col < 4; col++){
        // GET THE CURRENT INPUT PIXELS FOR CHANNELS 0:3 AND 4:7
        vec4 pixel = texelFetch(qt_texture, ivec2(index + 2*col, coord.y), 0);

        // SKEW THE CURRENT INPUT PIXELS
        qt_fragColor[col] = pixel[col];
    }

    return;
}
