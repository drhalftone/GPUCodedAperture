#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS
uniform sampler2D qt_codedAperture;  // THIS TEXTURE HOLDS THE CODED APERTURE MASK

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // CREATE VECTORS TO HOLD THE XYZW AND RGBA ALPHA CHANNELS
    vec4 pixelA, pixelB;

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    for (int c = 0; c < 4; c++){
        // GET THE CURRENT INPUT PIXELS FOR CHANNELS 0:3 AND 4:7 AND SKEW THE CURRENT INPUT PIXELS
        pixelA[c] = texelFetch(qt_texture, ivec2(2*coord.x + 0 + 2*c, coord.y), 0)[c];
        pixelB[c] = texelFetch(qt_texture, ivec2(2*coord.x + 8 + 2*c, coord.y), 0)[c];
    }
    // GRAB THE CODED APERTURE MASK PIXELS FROM THE MASK TEXTURE
    vec4 maskA = texelFetch(qt_codedAperture, ivec2(2*coord.x + 0, coord.y), 0);
    vec4 maskB = texelFetch(qt_codedAperture, ivec2(2*coord.x + 1, coord.y), 0);

    // CALCULATE MASKED SUM OF TWO PIXELS TO GET MONOCHROME RED IMAGE
    qt_fragColor = vec4(dot(pixelA, maskA) + dot(pixelB, maskB), 0.0, 0.0, 1.0);

    return;
}
