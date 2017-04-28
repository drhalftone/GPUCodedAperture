#version 330 core

uniform sampler2D qt_texture;    // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_weights;    // THIS TEXTURE HOLDS THE CODED APERTURE WEIGHTS
uniform sampler2D qt_mask;       // THIS TEXTURE HOLDS THE CODED APERTURE WEIGHTS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    for (int c = 0; c < 4; c++){
        float scaleFactor = texelFetch(qt_texture, ivec2(coord.x/2 - 4*(coord.x%2) - c, coord.y), 0).r;
        qt_fragColor[c] = scaleFactor * texelFetch(qt_texture, ivec2(coord.x/2 - 4*(coord.x%2) - c, coord.y), 0).r;
    }
    qt_fragColor = qt_fragColor * texelFetch(qt_texture, ivec2(coord.x/2 - 4*(coord.x%2) - c, coord.y), 0).r;

    return;
}
