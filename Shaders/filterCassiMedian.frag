#version 330 core

uniform sampler2D qt_texture;    // THIS TEXTURE HOLDS THE SCANS
uniform       int qt_radius;     // THIS INTEGER TELLS US HOW LARGE A FILTER KERNEL TO USE
uniform       int qt_threshold;  // THIS FLOATING POINT VALUE GIVES US THE SIGMA CONSTANT OF THE GAUSSIAN FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    int cumSum = 0;
    for (int row = -qt_radius; row <= qt_radius; row++){
        for (int col = -qt_radius; col <= qt_radius; col++){
            cumSum += int(texelFetch(qt_texture, coord + ivec2(col,row), 0).r > 0.0);
        }
    }
    qt_fragColor  = float(cumSum > qt_threshold) * texelFetch(qt_texture, coord, 0);
    qt_fragColor *= float(qt_fragColor.r > 0.0f);

    return;
}
