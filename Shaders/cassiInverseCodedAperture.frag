#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_mask;           // THIS TEXTURE HOLDS THE CODED APERTURE MASK

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    vec4 pixelA, pixelB;
    for (int col = 0; col < 4; col++){
        // GET THE CURRENT INPUT PIXELS FOR CHANNELS 0:3 AND 4:7
        vec4 pixA = texelFetch(qt_texture, ivec2(2*coord.x + 0, coord.y), 0);
        vec4 pixB = texelFetch(qt_texture, ivec2(2*coord.x + 1, coord.y), 0);

        // SKEW THE CURRENT INPUT PIXELS
        pixelA[col] = pixA[col];
        pixelB[col] = pixB[col];
    }
    // GRAB THE CODED APERTURE MASK PIXELS FROM THE MASK TEXTURE
    vec4 maskA = texelFetch(qt_mask, ivec2(2*coord.x + 0, coord.y), 0);
    vec4 maskB = texelFetch(qt_mask, ivec2(2*coord.x + 1, coord.y), 0);

    // CALCULATE MASKED SUM OF TWO PIXELS TO GET MONOCHROME RED IMAGE
    qt_fragColor = vec4(dot(pixelA, maskA) + dot(pixelB, maskB), 0.0, 0.0, 1.0);

    return;
}
