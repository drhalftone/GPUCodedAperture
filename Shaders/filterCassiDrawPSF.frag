#version 330 core

uniform sampler2D qt_texture;   // THIS TEXTURE HOLDS THE SCANS
uniform      vec2 qt_offset;    // THIS VEC2 TELLS US HOW TO SHIFT FROM GL_FRAGMENT COORDINATE TO THE TEXTURE COORDINATE
uniform      vec2 qt_offsetA;   // THIS VEC2 TELLS US HOW TO SHIFT FROM GL_FRAGMENT COORDINATE TO THE TEXTURE COORDINATE
uniform      vec2 qt_offsetB;   // THIS VEC2 TELLS US HOW TO SHIFT FROM GL_FRAGMENT COORDINATE TO THE TEXTURE COORDINATE
uniform      vec2 qt_offsetC;   // THIS VEC2 TELLS US HOW TO SHIFT FROM GL_FRAGMENT COORDINATE TO THE TEXTURE COORDINATE
uniform      vec2 qt_offsetD;   // THIS VEC2 TELLS US HOW TO SHIFT FROM GL_FRAGMENT COORDINATE TO THE TEXTURE COORDINATE
uniform      vec2 qt_lambda;    // THIS VEC2 TELLS US HOW TO SHIFT FROM GL_FRAGMENT COORDINATE TO THE TEXTURE COORDINATE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    vec2 coord = vec2(gl_FragCoord.xy - qt_offset);
    vec2 scale = vec2(textureSize(qt_texture, 0));

    // EXTRACT THE PIXELS FROM EACH SURROUNDING PSF TILE
    vec4 pixelA = texture(qt_texture, vec2(coord + qt_offsetA)/scale, 0);
    vec4 pixelB = texture(qt_texture, vec2(coord + qt_offsetB)/scale, 0);
    vec4 pixelC = texture(qt_texture, vec2(coord + qt_offsetC)/scale, 0);
    vec4 pixelD = texture(qt_texture, vec2(coord + qt_offsetD)/scale, 0);

    // INTERPOLATE BETWEEN TILES IN THE LEFT-TO-RIGHT DIRECTION
    pixelA = (qt_lambda.x)*pixelB + (1.0-qt_lambda.x)*pixelA;
    pixelC = (qt_lambda.x)*pixelD + (1.0-qt_lambda.x)*pixelC;

    // INTERPOLATE BETWEEN TILES IN THE UP-AND-DOWN DIRECTION
    qt_fragColor = (qt_lambda.y)*pixelC + (1.0-qt_lambda.y)*pixelA;
    return;
}
