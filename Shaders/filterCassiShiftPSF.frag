#version 330 core

uniform sampler3D qt_texture;   // THIS TEXTURE HOLDS THE SCANS
uniform       int qt_level;     // THIS INTEGER TELLS US WHAT SCAN LEVEL TO USE
uniform      vec2 qt_offset;    // THIS INTEGER TELLS US HOW LARGE A FILTER KERNEL TO USE
uniform     float qt_threshold; // THIS FLOATING POINT VALUE THRESHOLDS OUT BACKGROUND NOISE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = texelFetch(qt_texture, ivec3(gl_FragCoord.xy + qt_offset, qt_level), 0);
    qt_fragColor = qt_fragColor * float(qt_fragColor.r > qt_threshold);

    return;
}
