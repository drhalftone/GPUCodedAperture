#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_codedAperture;  // THIS TEXTURE HOLDS THE CODED APERTURE MASK

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    for (int col = 0; col < 4; col++){
        qt_fragColor[col] = texelFetch(qt_texture, ivec2(coord.x/2 - 4*(coord.x%2) - col, coord.y), 0).r;
    }
    qt_fragColor = qt_fragColor * texelFetch(qt_codedAperture, coord, 0);

    return;
}
