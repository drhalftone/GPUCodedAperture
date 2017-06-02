#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE SCANS

// DEFINE THE INVERSE DCT VECTORS
const vec4 vectors[16] = vec4[16](vec4( 0.35355339,  0.49039264,  0.46193977,  0.41573481), vec4( 0.35355339,  0.27778512,  0.19134172,  0.097545169),
                                  vec4( 0.35355339,  0.41573481,  0.19134172, -0.09754516), vec4(-0.35355339, -0.49039264, -0.46193977, -0.27778512),
                                  vec4( 0.35355339,  0.27778512, -0.19134172, -0.49039264), vec4(-0.35355339,  0.09754516,  0.46193977,  0.41573481),
                                  vec4( 0.35355339,  0.09754516, -0.46193977, -0.27778512), vec4( 0.35355339,  0.41573481, -0.19134172, -0.49039264),
                                  vec4( 0.35355339, -0.09754516, -0.46193977,  0.27778512), vec4( 0.35355339, -0.41573481, -0.19134172,  0.49039264),
                                  vec4( 0.35355339, -0.27778512, -0.19134172,  0.49039264), vec4(-0.35355339, -0.09754516,  0.46193977, -0.41573481),
                                  vec4( 0.35355339, -0.41573481,  0.19134172,  0.09754516), vec4(-0.35355339,  0.49039264, -0.46193977,  0.27778512),
                                  vec4( 0.35355339, -0.49039264,  0.46193977, -0.41573481), vec4( 0.35355339, -0.27778512,  0.19134172, -0.09754516));

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord   = ivec2(gl_FragCoord.xy);

    // FIGURE OUT IF WE NEED THE CALCULATE CHANNELS 0:3 OR 4:7
    int index = 8*(coord.x%2);

    // GRAB ALL EIGHT CHANNELS OF THE CURRENT PIXEL
    vec4 pixelA = texelFetch(qt_texture, ivec2((coord.x/2)*2 + 0, coord.y), 0);
    vec4 pixelB = texelFetch(qt_texture, ivec2((coord.x/2)*2 + 1, coord.y), 0);

    qt_fragColor.r = dot(vectors[index + 0], pixelA) + dot(vectors[index + 1], pixelB);
    qt_fragColor.g = dot(vectors[index + 2], pixelA) + dot(vectors[index + 3], pixelB);
    qt_fragColor.b = dot(vectors[index + 4], pixelA) + dot(vectors[index + 5], pixelB);
    qt_fragColor.a = dot(vectors[index + 6], pixelA) + dot(vectors[index + 7], pixelB);

 //   qt_fragColor = texelFetch(qt_texture, coord, 0);

    return;
}
