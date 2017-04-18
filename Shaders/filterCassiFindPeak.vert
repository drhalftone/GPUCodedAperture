#version 330 core

uniform       int qt_top;        // HOLDS THE ROW COORDINATE FOR THE TOP-LEFT PIXEL
uniform       int qt_left;       // HOLDS THE COLUMN COORDINATE FOR THE TOP-LEFT PIXEL
uniform       int qt_width;      // HOLDS THE WIDTH OF THE CURRENT REGION OF INTEREST
uniform sampler2D qt_texture;    // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
out          vec4 qt_fragment;   // THIS VECTOR HOLDS A SPECIFIC POINT LOCATION IN SCAN SPACE
in          float qt_vertex;     // ATTRIBUTE WHICH HOLDS THE ROW AND COLUMN COORDINATES FOR THE CURRENT PIXEL

void main(void)
{
    // SAVE THE POSITION OF THE SUBJECT COORDINATE TO PASS ONTO THE FRAGMENT
    int col = gl_VertexID % qt_width + qt_left;
    int row = gl_VertexID / qt_width + qt_top;

    // SAVE THE POSITION OF THE SUBJECT COORDINATE TO PASS ONTO THE FRAGMENT
    qt_fragment.x = float(col);
    qt_fragment.y = float(row);
    qt_fragment.z = texelFetch(qt_texture, ivec2(col, row), 0).r;
    qt_fragment.a = 1.0;

    // PASS THE Z-VALUE OF THE CURRENT PIXEL TO THE GL_POSITION FOR THE DEPTH FILTER
    gl_Position = vec4(0.0, 0.0, qt_fragment.z/10.0, 1.0);
}
