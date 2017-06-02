#version 330 core

uniform sampler2D qt_texture;           // THIS TEXTURE HOLDS THE SCANS
uniform     float qt_coefficients[16];  // HOLDS THE DWT FILTER
uniform      vec2 qt_position;          // OFFSET FOR THE FRAGMENT COORDINATE
uniform       int qt_width;             // HOLDS THE SIZE OF THE INCOMING TEXTURE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord   = ivec2(gl_FragCoord.xy - qt_position);
          coord.x = 2*coord.x - coord.x%2;

    // ITERATE THROUGH EACH PIXEL IN THE WINDOW
    qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    for (int col = 0; col < 16; col++){
        qt_fragColor += qt_coefficients[col] * texelFetch(qt_texture, ivec2((coord.x - 2*(col - 8) + 16*qt_width)%qt_width, coord.y), 0);
       // qt_fragColor += qt_coefficients[col] * texelFetch(qt_texture, ivec2(mod((coord.x - 2*(col - 8) + 16*qt_width), qt_width), coord.y), 0);

   //     qt_fragColor.r = float (coord.x - 2*(col - 8) );
   //     qt_fragColor.g = float (coord.x - 2*(col - 8) + 16*qt_width );
   //     qt_fragColor.b = float ( mod((coord.x - 2*(col - 8) + 16*qt_width), qt_width) );
   //     qt_fragColor.a = float ( texelFetch(qt_texture, ivec2(mod((coord.x - 2*(col - 8) + 16*qt_width), qt_width), coord.y), 0).r );
    }
//    qt_fragColor.r = gl_FragCoord.x;
//   qt_fragColor.g = gl_FragCoord.y;
//    qt_fragColor.b = float (coord.x);
//    qt_fragColor.a = float (coord.y);

    return;
}

