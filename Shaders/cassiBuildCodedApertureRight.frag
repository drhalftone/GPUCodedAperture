#version 330 core

uniform sampler2D qt_codedAperture;        // THIS TEXTURE HOLDS THE SCANS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET FRAGMENT COORDINATE OF SUBJECT PIXEL
    ivec2 coord = ivec2(gl_FragCoord.xy);

    for (int col = 0; col < 4; col++){
        // FIGURE OUT HOW FAR TO SHIFT PIXELS FOR THE CURRENT RGBA OR XYZW CHANNEL OF THE SUBJECT PIXEL
        int offset = 8*(coord.x%2) + 2*col;

        // GRAB THE XYZW + RGBA VALUES FOR THE SHIFTED PIXEL COORDINATE
        vec4 pixelA = texelFetch(qt_codedAperture, ivec2(2*(coord.x/2) + 0 - offset, coord.y), 0);
        vec4 pixelB = texelFetch(qt_codedAperture, ivec2(2*(coord.x/2) + 1 - offset, coord.y), 0);

        // CALCULATE THE SUM OF ONES IN THE SHIFTED PIXEL
        float scale = pixelA.r + pixelA.g + pixelA.b + pixelA.a + pixelB.r + pixelB.g + pixelB.b + pixelB.a + 0.00001;
              scale = float(scale > 0.5)/scale;

        // COPY OVER EITHER FROM THE XYZW OR THE RGBA PIXEL
        if (coord.x%2 == 0){
            qt_fragColor[col] = pixelA[col] * scale;
        } else {
            qt_fragColor[col] = pixelB[col] * scale;
        }
    }

    return;
}
