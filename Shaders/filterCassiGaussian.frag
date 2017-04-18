#version 330 core

uniform sampler3D qt_texture;   // THIS TEXTURE HOLDS THE SCANS
uniform       int qt_level;     // THIS INTEGER TELLS US WHAT SCAN LEVEL TO USE
uniform       int qt_radius;    // THIS INTEGER TELLS US HOW LARGE A FILTER KERNEL TO USE
uniform     float qt_sigma;     // THIS FLOATING POINT VALUE GIVES US THE SIGMA CONSTANT OF THE GAUSSIAN FILTER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    float cumSumWeghts = 0.0;
    float cumSumPixels = 0.0;
    for (int row = -qt_radius; row <= qt_radius; row++){
        for (int col = -qt_radius; col <= qt_radius; col++){
            vec2 point = vec2(float(row),float(col));
            float weght = exp(-dot(point.xy,point.xy)/(2.0*qt_sigma));
            float pixel = texelFetch(qt_texture, ivec3(coord + ivec2(col,row), qt_level), 0).r;

            cumSumPixels += weght*pixel;
            cumSumWeghts += weght;
        }
    }
    qt_fragColor = vec4(cumSumPixels/cumSumWeghts, 0.0, 0.0, 1.0);

    return;
}
