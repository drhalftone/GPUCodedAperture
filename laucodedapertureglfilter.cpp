/*********************************************************************************
 *                                                                               *
 * Copyright (c) 2017, Dr. Daniel L. Lau                                         *
 * All rights reserved.                                                          *
 *                                                                               *
 * Redistribution and use in source and binary forms, with or without            *
 * modification, are permitted provided that the following conditions are met:   *
 * 1. Redistributions of source code must retain the above copyright             *
 *    notice, this list of conditions and the following disclaimer.              *
 * 2. Redistributions in binary form must reproduce the above copyright          *
 *    notice, this list of conditions and the following disclaimer in the        *
 *    documentation and/or other materials provided with the distribution.       *
 * 3. All advertising materials mentioning features or use of this software      *
 *    must display the following acknowledgement:                                *
 *    This product includes software developed by the <organization>.            *
 * 4. Neither the name of the <organization> nor the                             *
 *    names of its contributors may be used to endorse or promote products       *
 *    derived from this software without specific prior written permission.      *
 *                                                                               *
 * THIS SOFTWARE IS PROVIDED BY Dr. Daniel L. Lau ''AS IS'' AND ANY              *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE        *
 * DISCLAIMED. IN NO EVENT SHALL Dr. Daniel L. Lau BE LIABLE FOR ANY             *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND   *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                  *
 *                                                                               *
 *********************************************************************************/

#include "laucodedapertureglfilter.h"
#include <locale.h>

float LAUCodedApertureGLFilter::LoD[16] = {  -0.00338242,  -0.00054213,   0.03169509,   0.00760749,  -0.14329424,  -0.06127336,   0.48135965,   0.77718575,   0.36444189,  -0.05194584,  -0.02721903,   0.04913718,   0.00380875,  -0.01495226,  -0.00030292,   0.00188995 };
float LAUCodedApertureGLFilter::HiD[16] = {  -0.00188995,  -0.00030292,   0.01495226,   0.00380875,  -0.04913718,  -0.02721903,   0.05194584,   0.36444189,  -0.77718575,   0.48135965,   0.06127336,  -0.14329424,  -0.00760749,   0.03169509,   0.00054213,  -0.00338242 };
float LAUCodedApertureGLFilter::LoR[16] = {   0.00188995,  -0.00030292,  -0.01495226,   0.00380875,   0.04913718,  -0.02721903,  -0.05194584,   0.36444189,   0.77718575,   0.48135965,  -0.06127336,  -0.14329424,   0.00760749,   0.03169509,  -0.00054213,  -0.00338242 };
float LAUCodedApertureGLFilter::HiR[16] = {  -0.00338242,   0.00054213,   0.03169509,  -0.00760749,  -0.14329424,   0.06127336,   0.48135965,  -0.77718575,   0.36444189,   0.05194584,  -0.02721903,  -0.04913718,   0.00380875,   0.01495226,  -0.00030292,  -0.00188995 };

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLWidget::LAUCodedApertureGLWidget(unsigned int cols, unsigned int rows, QWidget *parent) : QOpenGLWidget(parent), scan(LAUScan(cols, rows, ColorXYZWRGBA)), dataCube(NULL)
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLWidget::LAUCodedApertureGLWidget(LAUScan scn, QWidget *parent) : QOpenGLWidget(parent), scan(scn), dataCube(NULL)
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLWidget::~LAUCodedApertureGLWidget()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(NAN, NAN, NAN, 0.0f);

    // GET CONTEXT OPENGL-VERSION
    qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
    qDebug() << "OpenGl information: VENDOR:       " << (const char *)glGetString(GL_VENDOR);
    qDebug() << "                    RENDERDER:    " << (const char *)glGetString(GL_RENDERER);
    qDebug() << "                    VERSION:      " << (const char *)glGetString(GL_VERSION);
    qDebug() << "                    GLSL VERSION: " << (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);

    // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
    vertexArrayObject.create();
    vertexArrayObject.bind();

    // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
    vertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer.create();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (vertexBuffer.bind()) {
        vertexBuffer.allocate(16 * sizeof(float));
        float *vertices = (float *)vertexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (vertices) {
            vertices[0]  = -1.0;
            vertices[1]  = -1.0;
            vertices[2]  = 0.0;
            vertices[3]  = 1.0;
            vertices[4]  = +1.0;
            vertices[5]  = -1.0;
            vertices[6]  = 0.0;
            vertices[7]  = 1.0;
            vertices[8]  = +1.0;
            vertices[9]  = +1.0;
            vertices[10] = 0.0;
            vertices[11] = 1.0;
            vertices[12] = -1.0;
            vertices[13] = +1.0;
            vertices[14] = 0.0;
            vertices[15] = 1.0;

            vertexBuffer.unmap();
        } else {
            qDebug() << QString("Unable to map vertexBuffer from GPU.");
        }
        vertexBuffer.release();
    }

    // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
    // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
    indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBuffer.create();
    indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (indexBuffer.bind()) {
        indexBuffer.allocate(6 * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)indexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (indices) {
            indices[0] = 0;
            indices[1] = 1;
            indices[2] = 2;
            indices[3] = 0;
            indices[4] = 2;
            indices[5] = 3;
            indexBuffer.unmap();
        } else {
            qDebug() << QString("indexBuffer buffer mapped from GPU.");
        }
        indexBuffer.release();
    }

    // CREATE THE SHADER FOR DISPLAYING 8-COLOR IMAGES ON SCREEN
    setlocale(LC_NUMERIC, "C");
    program.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMapToVectorV.frag");
    program.link();
    setlocale(LC_ALL, "");

    // DISPLAY EXISTING SCAN
    onUpdateScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::onUpdateScan(LAUScan scn)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scn.isValid() && scn.colors() == 8)  {
        // MAKE A LOCAL COPY OF THE INCOMING SCAN
        scan = scn;

        // MAKE THIS THE CURRENT OPENGL CONTEXT
        makeCurrent();

        // DELETE THE OLD DATE CUBE TEXTURE IF IT EXISTS
        if (dataCube) {
            delete dataCube;
        }

        // CREATE THE GPU SIDE TEXTURE TO HOLD THE 3D DATA CUBE
        dataCube = new QOpenGLTexture(QOpenGLTexture::Target2D);
        dataCube->setSize(2 * scan.width(), scan.height());
        dataCube->setFormat(QOpenGLTexture::RGBA32F);
        dataCube->setWrapMode(QOpenGLTexture::ClampToBorder);
        dataCube->setMinificationFilter(QOpenGLTexture::Nearest);
        dataCube->setMagnificationFilter(QOpenGLTexture::Nearest);
        dataCube->allocateStorage();

        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // UPDATE THE DISPLAY ON SCREEN
        update();
    }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::resizeGL(int w, int h)
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::paintGL()
{
    ;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureWidget::LAUCodedApertureWidget(LAUScan scan, QWidget *parent) : QWidget(parent), glWidget(NULL), codedApertureFilter(NULL)
{
    // INITIALIZE THE CURVATURE FILTER AND
    // CREATE A GLWIDGET TO DISPLAY THE SCAN
    if (scan.isValid()) {
        result = scan.convertToColor(ColorXYZWRGBA);
        codedApertureFilter = new LAUCodedApertureGLFilter(result);
        result = codedApertureFilter->reconstructDataCube(result);
    }
    glWidget = new LAUCodedApertureGLWidget(scan);
    glWidget->setMinimumSize(scan.width(), scan.height());

    this->setLayout(new QVBoxLayout());
    this->setContentsMargins(0, 0, 0, 0);
    this->layout()->addWidget(glWidget);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureWidget::~LAUCodedApertureWidget()
{
    qDebug() << "LAUCodedApertureWidget()::~LAUCodedApertureWidget()";
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLFilter::LAUCodedApertureGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent) : QOpenGLContext(parent)
{
    // INITIALIZE PRIVATE VARIABLES
    numCols = cols;
    numRows = rows;
    playbackColor = color;

    // INITIALIZE PRIVATE VARIABLES IN A STAND ALONE METHOD
    initializeParameters();

    // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
    surface = new QOffscreenSurface();
    ((QOffscreenSurface *)surface)->create();

    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    this->setFormat(surface->format());
    this->create();
    this->initialize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLFilter::LAUCodedApertureGLFilter(LAUScan scan, QWidget *parent) : QOpenGLContext(parent)
{
    // INITIALIZE PRIVATE VARIABLES
    numCols = scan.width();
    numRows = scan.height();
    playbackColor = scan.color();

    // INITIALIZE PRIVATE VARIABLES IN A STAND ALONE METHOD
    initializeParameters();

    // SEE IF THE USER GAVE US A TARGET SURFACE, IF NOT, THEN CREATE AN OFFSCREEN SURFACE BY DEFAULT
    surface = new QOffscreenSurface();
    ((QOffscreenSurface *)surface)->create();

    // NOW SEE IF WE HAVE A VALID PROCESSING CONTEXT FROM THE USER, AND THEN SPIN IT INTO ITS OWN THREAD
    this->setFormat(surface->format());
    this->create();
    this->initialize();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLFilter::~LAUCodedApertureGLFilter()
{
    if (surface && makeCurrent(surface)) {
        if (frameBufferObjectXYZWRGBAa) {
            delete frameBufferObjectXYZWRGBAa;
        }
        if (frameBufferObjectXYZWRGBAb) {
            delete frameBufferObjectXYZWRGBAb;
        }
        if (frameBufferObjectCodedApertureMask) {
            delete frameBufferObjectCodedApertureMask;
        }
        if (frameBufferObjectCodedAperture) {
            delete frameBufferObjectCodedAperture;
        }
        if (dataCube) {
            delete dataCube;
        }
        if (spectralMeasurement) {
            delete spectralMeasurement;
        }
        if (wasInitialized()) {
            vertexArrayObject.release();
        }
        doneCurrent();
        delete surface;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::initialize()
{
    if (makeCurrent(surface)) {
        initializeOpenGLFunctions();
        glClearColor(NAN, NAN, NAN, 0.0f);

        // GET CONTEXT OPENGL-VERSION
        qDebug() << "Really used OpenGl: " << format().majorVersion() << "." << format().minorVersion();
        qDebug() << "OpenGl information: VENDOR:       " << (const char *)glGetString(GL_VENDOR);
        qDebug() << "                    RENDERDER:    " << (const char *)glGetString(GL_RENDERER);
        qDebug() << "                    VERSION:      " << (const char *)glGetString(GL_VERSION);
        qDebug() << "                    GLSL VERSION: " << (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);

        initializeVertices();
        initializeTextures();
        initializeShaders();

        // INITIALIZE THE CODED APERTURE TEXTURE
        setCodedAperture(QImage(":/Images/Images/CASSIMask.bmp"));

        // RELEASE THIS CONTEXT AS THE CURRENT GL CONTEXT
        doneCurrent();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::initializeVertices()
{
    // CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
    vertexArrayObject.create();
    vertexArrayObject.bind();

    // CREATE A BUFFER TO HOLD THE ROW AND COLUMN COORDINATES OF IMAGE PIXELS FOR THE TEXEL FETCHES
    vertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer.create();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (vertexBuffer.bind()) {
        vertexBuffer.allocate(16 * sizeof(float));
        float *vertices = (float *)vertexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (vertices) {
            vertices[0]  = -1.0;
            vertices[1]  = -1.0;
            vertices[2]  = 0.0;
            vertices[3]  = 1.0;
            vertices[4]  = +1.0;
            vertices[5]  = -1.0;
            vertices[6]  = 0.0;
            vertices[7]  = 1.0;
            vertices[8]  = +1.0;
            vertices[9]  = +1.0;
            vertices[10] = 0.0;
            vertices[11] = 1.0;
            vertices[12] = -1.0;
            vertices[13] = +1.0;
            vertices[14] = 0.0;
            vertices[15] = 1.0;

            vertexBuffer.unmap();
        } else {
            qDebug() << QString("Unable to map vertexBuffer from GPU.");
        }
        vertexBuffer.release();
    }

    // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
    // CREATE AN INDEX BUFFER FOR THE INCOMING DEPTH VIDEO DRAWN AS POINTS
    indexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indexBuffer.create();
    indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    if (indexBuffer.bind()) {
        indexBuffer.allocate(6 * sizeof(unsigned int));
        unsigned int *indices = (unsigned int *)indexBuffer.map(QOpenGLBuffer::WriteOnly);
        if (indices) {
            indices[0] = 0;
            indices[1] = 1;
            indices[2] = 2;
            indices[3] = 0;
            indices[4] = 2;
            indices[5] = 3;
            indexBuffer.unmap();
        } else {
            qDebug() << QString("indexBuffer buffer mapped from GPU.");
        }
        indexBuffer.release();
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::initializeShaders()
{
    // CREATE GLSL PROGRAM FOR PROCESSING THE INCOMING VIDEO
    setlocale(LC_NUMERIC, "C");
    programAx.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programAx.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardDWTx.frag");
    programAx.link();

    programAy.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programAy.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardDWTy.frag");
    programAy.link();

    programBx.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programBx.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardDCT.frag");
    programBx.link();

    programBy.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programBy.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiInverseDCT.frag");
    programBy.link();

    programCx.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programCx.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiInverseDWTx.frag");
    programCx.link();

    programCy.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programCy.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiInverseDWTy.frag");
    programCy.link();

    programDx.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programDx.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardCodedAperture.frag");
    programDx.link();

    programDy.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programDy.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiInverseCodedAperture.frag");
    programDy.link();

    programU.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programU.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMapToVectorU.frag");
    programU.link();

    programV.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    programV.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMapToVectorV.frag");
    programV.link();
    setlocale(LC_ALL, "");
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::initializeTextures()
{
    // CREATE THE GPU SIDE TEXTURE TO HOLD THE 3D DATA CUBE
    dataCube = new QOpenGLTexture(QOpenGLTexture::Target2D);
    dataCube->setSize(2 * numCols, numRows);
    dataCube->setFormat(QOpenGLTexture::RGBA32F);
    dataCube->setWrapMode(QOpenGLTexture::ClampToBorder);
    dataCube->setMinificationFilter(QOpenGLTexture::Nearest);
    dataCube->setMagnificationFilter(QOpenGLTexture::Nearest);
    dataCube->allocateStorage();

    // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE CODED APERTURE MEASUREMENT
    spectralMeasurement = new QOpenGLTexture(QOpenGLTexture::Target2D);
    spectralMeasurement->setSize(numCols, numRows);
    spectralMeasurement->setFormat(QOpenGLTexture::R32F);
    spectralMeasurement->setWrapMode(QOpenGLTexture::ClampToBorder);
    spectralMeasurement->setMinificationFilter(QOpenGLTexture::Nearest);
    spectralMeasurement->setMagnificationFilter(QOpenGLTexture::Nearest);
    spectralMeasurement->allocateStorage();

    // CREATE A FRAME BUFFER OBJECT FORMAT INSTANCE FOR THE FBOS
    QOpenGLFramebufferObjectFormat frameBufferObjectFormat;
    frameBufferObjectFormat.setInternalTextureFormat(GL_RGBA32F);

    // CREATE THE INTERMEDIATE FBOS FOR THE FORWARD AND INVERSE TRANSFORMS
    frameBufferObjectXYZWRGBAa = new QOpenGLFramebufferObject(2 * numCols, numRows, frameBufferObjectFormat);
    frameBufferObjectXYZWRGBAa->release();

    frameBufferObjectXYZWRGBAb = new QOpenGLFramebufferObject(2 * numCols, numRows, frameBufferObjectFormat);
    frameBufferObjectXYZWRGBAb->release();

    frameBufferObjectCodedApertureMask = new QOpenGLFramebufferObject(2 * numCols, numRows, frameBufferObjectFormat);
    frameBufferObjectCodedApertureMask->release();

    // CREATE THE FINAL FBO FOR HOLDING THE MONOCHROME OUTPUT
    frameBufferObjectFormat.setInternalTextureFormat(GL_R32F);
    frameBufferObjectCodedAperture = new QOpenGLFramebufferObject(numCols, numRows, frameBufferObjectFormat);
    frameBufferObjectCodedAperture->release();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::initializeParameters()
{
    frameBufferObjectXYZWRGBAa = NULL;
    frameBufferObjectXYZWRGBAb = NULL;
    frameBufferObjectCodedApertureMask = NULL;
    frameBufferObjectCodedAperture = NULL;
    spectralMeasurement = NULL;
    dataCube = NULL;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::forwardDWCTransform(LAUScan scan)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() != 8)  {
        return (LAUScan());
    }

    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programAx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width() / 2, frameBufferObjectXYZWRGBAa->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        dataCube->bind();
                        programAx.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAx.setUniformValueArray("coefficients", LoD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAx.setUniformValue("rangeLimit", dataCube->width());
                        programAx.setUniformValue("offset", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programAx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programAx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // SET THE VIEW PORT TO THE RIGHT-HALF OF THE IMAGE FOR HIGH-PASS FILTERING
                        glViewport(frameBufferObjectXYZWRGBAa->width() / 2, 0, frameBufferObjectXYZWRGBAa->width() / 2, frameBufferObjectXYZWRGBAa->height());

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAx.setUniformValueArray("coefficients", HiD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAx.setUniformValue("offset", (frameBufferObjectXYZWRGBAa->width() / 2));

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programAx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programAy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width(), frameBufferObjectXYZWRGBAb->height() / 2);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
                        programAy.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAy.setUniformValueArray("coefficients", LoD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAy.setUniformValue("rangeLimit", dataCube->height());
                        programAy.setUniformValue("offset", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programAy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programAy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // SET THE VIEW PORT TO THE RIGHT-HALF OF THE IMAGE FOR HIGH-PASS FILTERING
                        glViewport(0, frameBufferObjectXYZWRGBAa->height() / 2, frameBufferObjectXYZWRGBAa->width(), frameBufferObjectXYZWRGBAa->height() / 2);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAy.setUniformValueArray("coefficients", HiD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAy.setUniformValue("offset", (frameBufferObjectXYZWRGBAa->height() / 2));

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programAy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programAx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width() / 4, frameBufferObjectXYZWRGBAa->height() / 2);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAa->width() / 4, frameBufferObjectXYZWRGBAa->height() / 2);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
                        programAx.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAx.setUniformValueArray("coefficients", LoD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAx.setUniformValue("rangeLimit", frameBufferObjectXYZWRGBAb->width() / 2);
                        programAx.setUniformValue("offset", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programAx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programAx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // SET THE VIEW PORT TO THE RIGHT-HALF OF THE IMAGE FOR HIGH-PASS FILTERING
                        glViewport(frameBufferObjectXYZWRGBAa->width() / 4, 0, frameBufferObjectXYZWRGBAa->width() / 4, frameBufferObjectXYZWRGBAa->height() / 2);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAx.setUniformValueArray("coefficients", HiD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAx.setUniformValue("offset", (frameBufferObjectXYZWRGBAa->width() / 4));

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programAx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programAy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width() / 2, frameBufferObjectXYZWRGBAb->height() / 4);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAb->width() / 2, frameBufferObjectXYZWRGBAb->height() / 4);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
                        programAy.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAy.setUniformValueArray("coefficients", LoD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAy.setUniformValue("rangeLimit", frameBufferObjectXYZWRGBAa->height() / 2);
                        programAy.setUniformValue("offset", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programAy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programAy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // SET THE VIEW PORT TO THE RIGHT-HALF OF THE IMAGE FOR HIGH-PASS FILTERING
                        glViewport(0, frameBufferObjectXYZWRGBAa->height() / 4, frameBufferObjectXYZWRGBAa->width() / 2, frameBufferObjectXYZWRGBAa->height() / 4);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAy.setUniformValueArray("coefficients", HiD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAy.setUniformValue("offset", (frameBufferObjectXYZWRGBAa->height() / 4));

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programAy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programAx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width() / 8, frameBufferObjectXYZWRGBAa->height() / 4);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAa->width() / 8, frameBufferObjectXYZWRGBAa->height() / 4);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
                        programAx.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAx.setUniformValueArray("coefficients", LoD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAx.setUniformValue("rangeLimit", frameBufferObjectXYZWRGBAb->width() / 4);
                        programAx.setUniformValue("offset", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programAx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programAx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // SET THE VIEW PORT TO THE RIGHT-HALF OF THE IMAGE FOR HIGH-PASS FILTERING
                        glViewport(frameBufferObjectXYZWRGBAa->width() / 8, 0, frameBufferObjectXYZWRGBAa->width() / 8, frameBufferObjectXYZWRGBAa->height() / 4);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAx.setUniformValueArray("coefficients", HiD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAx.setUniformValue("offset", (frameBufferObjectXYZWRGBAa->width() / 8));

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programAx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programAy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width() / 4, frameBufferObjectXYZWRGBAb->height() / 8);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAb->width() / 4, frameBufferObjectXYZWRGBAb->height() / 8);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
                        programAy.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAy.setUniformValueArray("coefficients", LoD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAy.setUniformValue("rangeLimit", frameBufferObjectXYZWRGBAa->height() / 4);
                        programAy.setUniformValue("offset", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programAy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programAy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // SET THE VIEW PORT TO THE RIGHT-HALF OF THE IMAGE FOR HIGH-PASS FILTERING
                        glViewport(0, frameBufferObjectXYZWRGBAa->height() / 8, frameBufferObjectXYZWRGBAa->width() / 4, frameBufferObjectXYZWRGBAa->height() / 8);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programAy.setUniformValueArray("coefficients", HiD, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programAy.setUniformValue("offset", (frameBufferObjectXYZWRGBAa->height() / 8));

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programAy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programBx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width(), frameBufferObjectXYZWRGBAa->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
                        programBx.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programBx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programBx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programBx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)scan.pointer());
        doneCurrent();
    }

    // RETURN THE UPDATED SCAN
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::reverseDWCTransform(LAUScan scan)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() != 8)  {
        return (LAUScan());
    }

    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programBy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width(), frameBufferObjectXYZWRGBAb->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        dataCube->bind();
                        programBy.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programBy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programBy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programBy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programCx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width() / 4, frameBufferObjectXYZWRGBAa->height() / 4);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAa->width() / 4, frameBufferObjectXYZWRGBAa->height() / 4);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
                        programCx.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programCx.setUniformValueArray("coefficientsA", LoR, 16, 1);
                        programCx.setUniformValueArray("coefficientsB", HiR, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programCx.setUniformValue("offset", frameBufferObjectXYZWRGBAa->width() / 8);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programCx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programCx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programCx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programCy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width() / 4, frameBufferObjectXYZWRGBAb->height() / 4);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAb->width() / 4, frameBufferObjectXYZWRGBAb->height() / 4);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
                        programCy.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programCy.setUniformValueArray("coefficientsA", LoR, 16, 1);
                        programCy.setUniformValueArray("coefficientsB", HiR, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programCy.setUniformValue("offset", frameBufferObjectXYZWRGBAb->height() / 8);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programCy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programCy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programCy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programCx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width() / 2, frameBufferObjectXYZWRGBAa->height() / 2);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAa->width() / 2, frameBufferObjectXYZWRGBAa->height() / 2);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
                        programCx.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programCx.setUniformValueArray("coefficientsA", LoR, 16, 1);
                        programCx.setUniformValueArray("coefficientsB", HiR, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programCx.setUniformValue("offset", frameBufferObjectXYZWRGBAa->width() / 4);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programCx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programCx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programCx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programCy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width() / 2, frameBufferObjectXYZWRGBAb->height() / 2);
                        glScissor(0, 0, frameBufferObjectXYZWRGBAb->width() / 2, frameBufferObjectXYZWRGBAb->height() / 2);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
                        programCy.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programCy.setUniformValueArray("coefficientsA", LoR, 16, 1);
                        programCy.setUniformValueArray("coefficientsB", HiR, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programCy.setUniformValue("offset", frameBufferObjectXYZWRGBAb->height() / 4);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programCy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programCy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programCy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programCx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width(), frameBufferObjectXYZWRGBAa->height());
                        glScissor(0, 0, frameBufferObjectXYZWRGBAa->width(), frameBufferObjectXYZWRGBAa->height());

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
                        programCx.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programCx.setUniformValueArray("coefficientsA", LoR, 16, 1);
                        programCx.setUniformValueArray("coefficientsB", HiR, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programCx.setUniformValue("offset", frameBufferObjectXYZWRGBAa->width() / 2);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programCx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programCx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programCx.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programCy.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width(), frameBufferObjectXYZWRGBAb->height());
                        glScissor(0, 0, frameBufferObjectXYZWRGBAb->width(), frameBufferObjectXYZWRGBAb->height());

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
                        programCy.setUniformValue("qt_texture", 0);

                        // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                        programCy.setUniformValueArray("coefficientsA", LoR, 16, 1);
                        programCy.setUniformValueArray("coefficientsB", HiR, 16, 1);

                        // SET THE SCALE AND OFFSET VALUES TO MAP INPUT PIXELS TO OUTPUT PIXELS
                        programCy.setUniformValue("offset", frameBufferObjectXYZWRGBAb->height() / 2);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programCy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programCy.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programCy.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)scan.pointer());
        doneCurrent();
    }

    // RETURN THE UPDATED SCAN
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::forwardCodedAperture(LAUScan scan)
{
    // CREATE AN OUTPUT SCAN
    LAUScan result = LAUScan();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() == 1) {
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            spectralMeasurement->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
            // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
            if (frameBufferObjectXYZWRGBAa && frameBufferObjectXYZWRGBAa->bind()) {
                if (programDx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, frameBufferObjectXYZWRGBAa->width(), frameBufferObjectXYZWRGBAa->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                            glActiveTexture(GL_TEXTURE0);
                            spectralMeasurement->bind();
                            programDx.setUniformValue("qt_texture", 0);
                            programDx.setUniformValue("qt_scale", 1.0f);

                            // SET SCALE FACTOR FOR INVERTING CODED APERTURE
                            programDx.setUniformValue("qt_scale", 1.0f);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(programDx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            programDx.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    programDx.release();
                }
                frameBufferObjectXYZWRGBAa->release();
            }

            // CREATE A NEW MONOCHROME SCAN
            result = LAUScan(scan.width(), scan.height(), ColorXYZWRGBA);
            glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)result.pointer());
        }
    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::reverseCodedAperture(LAUScan scan)
{
    // CREATE AN OUTPUT SCAN
    LAUScan result = LAUScan();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() == 8) {
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
            // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
            if (frameBufferObjectCodedAperture && frameBufferObjectCodedAperture->bind()) {
                if (programDy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, frameBufferObjectCodedAperture->width(), frameBufferObjectCodedAperture->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                            glActiveTexture(GL_TEXTURE0);
                            dataCube->bind();
                            programDy.setUniformValue("qt_texture", 0);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE CODED APERTURE MASK
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, frameBufferObjectCodedApertureMask->texture());
                            programDy.setUniformValue("qt_mask", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(programDy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            programDy.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    programDy.release();
                }
                frameBufferObjectCodedAperture->release();
            }

            // CREATE A NEW MONOCHROME SCAN
            result = LAUScan(scan.width(), scan.height(), ColorGray);
            glBindTexture(GL_TEXTURE_2D, frameBufferObjectCodedAperture->texture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (unsigned char *)result.pointer());
        }
    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::reconstructDataCube(LAUScan ideal)
{
    // THE INCOMING SCAN IS THE COMPLETE, IDEAL, PERFECT 3D DATA CUBE
    // WE WANT TO GENERATE A CODED APERTURE ENCODING AND THEN RECONSTRUCT THIS SCAN
    // CALCULATING THE MEAN SQUARED ERROR STEP BY STEP

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (ideal.colors() != 8)  {
        return (LAUScan());
    }

    // INITIALIZE VARIABLES FOR MANAGING GPSR ALGORITHM
    stopCriterion = SCSmallStepsInNormOfDifference;
    initialization = InitAllZeros;
    debias = false;
    verbose = false;
    monotone = true;
    continuation = false;
    tolA = 0.01;
    tolD = 0.0001;
    alphaMin = 1e-30;
    alphaMax = 1e30;
    maxIterA = 10000;
    minIterA = 5;
    maxIterD = 200;
    minIterD = 5;
    continuationSteps = 0;

    // SO LET'S START BY GENERATING OUR CODED APERTURE ENCODING
    LAUScan vectorY = reverseCodedAperture(ideal);
    vectorY.save("/tmp/vectorY.tif");

    // NOW CALCULATE THE INITIAL ESTIMATE (LINE 290 OF GPSR_BB SCRIPT)
    LAUScan vectorXi = forwardTransform(vectorY);
    vectorXi.save("/tmp/vectorXi.tif");

    // CALL METHOD FOR CALCULATING THE INITIAL TAU PARAMETER ACCORDING TO  0.5 * max(abs(AT(y)))
    firstTau = maxAbsValue(vectorXi) / 2.0f;

    // INITIALIZE U AND V VECTORS (LINES 345 AND 346 OF GPSR_BB SCRIPT)
    LAUScan vectorU = computeVectorU(vectorXi);
    vectorU.save("/tmp/vectorU.tif");

    LAUScan vectorV = computeVectorV(vectorXi);
    vectorV.save("/tmp/vectorV.tif");

    // GET THE NUMBER OF NON-ZERO ELEMENTS IN X (LINE 350 OF GPSR_BB SCRIPT)
    int nonZeroCount = nonZeroElements(vectorXi);

    // CALCULATE RESIDUE (LINE 402 OF GPSR_BB SCRIPT)
    LAUScan vectorAofX = reverseTransform(vectorXi);
    vectorAofX.save("/tmp/vectorAofX.tif");

    LAUScan vectorResidue = subtractScans(vectorY, vectorAofX);
    vectorResidue.save("/tmp/vectorResidue.tif");

    // CALCULATE THE GRADIENT BASED ON THE FORWARD TRANSFORM OF THE RESIDUE
    LAUScan vectorGradient = forwardTransform(vectorResidue);
    vectorGradient.save("/tmp/vectorGradient.tif");

    // COMPUTE THE INITIAL GRADIENT AND THE USEFUL QUANTITY RESID_BASE (LINE 452 OF GPSR_BB SCRIPT)
    LAUScan vectorResidueBase = subtractScans(vectorY, vectorResidue);
    vectorResidueBase.save("/tmp/vectorResidueBase.tif");

    // RETURN NOTHING, FOR NOW
    return (LAUScan());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::computeVectorU(LAUScan scan)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() != 8)  {
        return (LAUScan());
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (frameBufferObjectXYZWRGBAa->bind()) {
            if (programU.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAa->width(), frameBufferObjectXYZWRGBAa->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        dataCube->bind();
                        programU.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programU.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programU.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programU.release();
            }
            frameBufferObjectXYZWRGBAa->release();
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAa->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)scan.pointer());
        doneCurrent();
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::computeVectorV(LAUScan scan)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() != 8)  {
        return (LAUScan());
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (frameBufferObjectXYZWRGBAb->bind()) {
            if (programV.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectXYZWRGBAb->width(), frameBufferObjectXYZWRGBAb->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        dataCube->bind();
                        programV.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programV.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programV.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programV.release();
            }
            frameBufferObjectXYZWRGBAb->release();
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        glBindTexture(GL_TEXTURE_2D, frameBufferObjectXYZWRGBAb->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)scan.pointer());
        doneCurrent();
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::maxAbsValue(LAUScan scan)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (scan.colors() != 8) {
        return (NAN);
    }

    // CREATE SSE VECTOR TO HOLD THE MAXIMUM VALUES OVER TWO VEC4s
    __m128 maxVec = _mm_set1_ps(-1e9f);

    // WE NEED THIS TO PERFORM THE ABSOLUTE VALUE OPERATION FOR SINGLE PRECISION FLOATING POINT
    static const __m128 sgnVec = _mm_set1_ps(-0.0f);

    // ITERATE THROUGH EACH PIXEL COMPARING WITH THE MAX VECTOR AT EACH LOAD
    int index = 0;
    float *buffer = (float *)scan.constPointer();
    for (unsigned int row = 0; row < numRows; row++) {
        for (unsigned int col = 0; col < numCols; col++) {
            maxVec = _mm_max_ps(maxVec, _mm_andnot_ps(sgnVec, _mm_load_ps(buffer + index + 0)));
            maxVec = _mm_max_ps(maxVec, _mm_andnot_ps(sgnVec, _mm_load_ps(buffer + index + 4)));
            index += 8;
        }
    }

    // EXTRACT THE FLOATS FROM THE VEC4
    float a, b, c, d;
    *(int *)&a = _mm_extract_ps(maxVec, 0);
    *(int *)&b = _mm_extract_ps(maxVec, 1);
    *(int *)&c = _mm_extract_ps(maxVec, 2);
    *(int *)&d = _mm_extract_ps(maxVec, 3);

    // FIND THE LARGEST SCALAR VALUE
    return (qMax(a, qMax(b, qMax(c, d))));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
int LAUCodedApertureGLFilter::nonZeroElements(LAUScan scan)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (scan.colors() != 8) {
        return (NAN);
    }

    // CREATE SSE VECTOR TO HOLD THE SUM OF LOGICAL VALUES OVER TWO VEC4s
    __m128i pixVec = _mm_set1_epi32(0);

    // WE NEED THIS TO PERFORM THE COMPARE WITH ZERO VALUE OPERATION FOR SINGLE PRECISION FLOATING POINT
    static const __m128i zerVec = _mm_set1_ps(0.0f);

    // ITERATE THROUGH EACH PIXEL COMPARING WITH THE ZERO VECTOR AT EACH LOAD
    // AND ADDING THE LOGICAL RESULT TO OUR INTEGER ACCUMULATION VECTOR
    int index = 0;
    float *buffer = (float *)scan.constPointer();
    for (unsigned int row = 0; row < numRows; row++) {
        for (unsigned int col = 0; col < numCols; col++) {
            pixVec = _mm_add_epi32(pixVec, _mm_cmpneq_ps(zerVec, _mm_load_ps(buffer + index + 0)));
            pixVec = _mm_add_epi32(pixVec, _mm_cmpneq_ps(zerVec, _mm_load_ps(buffer + index + 4)));
            index += 8;
        }
    }
    pixVec = _mm_hadd_epi32(pixVec, pixVec);
    pixVec = _mm_hadd_epi32(pixVec, pixVec);
    return (_mm_extract_ps(pixVec, 0));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::subtractScans(LAUScan scanA, LAUScan scanB)
{
    // MAKE SURE WE HAVE TWO 8-CHANNEL IMAGES AND THAT THEY ARE BOTH THE SAME SIZE
    if (scanA.colors() != scanB.colors() || scanA.width() != scanB.width() || scanA.height() != scanB.height()) {
        return (LAUScan());
    }

    // CREATE AN OUTPUT SCAN
    LAUScan result = LAUScan(scanA.width(), scanA.height(), scanA.color());

    // GRAB THE POINTERS TO THE TWO INPUT BUFFERS AND THE OUTPUT BUFFER
    float *bufferA = (float *)scanA.constPointer();
    float *bufferB = (float *)scanB.constPointer();
    float *bufferR = (float *)result.constPointer();

    // ITERATE THROUGH EACH PIXEL SUBSTRACTING VECTORS FROM EACHOTHER
    if (scanA.colors() == 1) {
        int index = 0;
        for (unsigned int row = 0; row < numRows; row++) {
            for (unsigned int col = 0; col < numCols; col += 4) {
                // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
                // AND STORE THE RESULTING VECTORS TO MEMORY
                _mm_stream_ps(bufferR + index, _mm_sub_ps(_mm_load_ps(bufferA + index), _mm_load_ps(bufferB + index)));

                // UPDATE THE INDEX TO POINT TO THE NEXT PIXEL COMPOSED OF 8 FLOATS
                index += 4;
            }
        }
    } else if (scanA.colors() == 4) {
        int index = 0;
        for (unsigned int row = 0; row < numRows; row++) {
            for (unsigned int col = 0; col < numCols; col++) {
                // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
                // AND STORE THE RESULTING VECTORS TO MEMORY
                _mm_stream_ps(bufferR + index, _mm_sub_ps(_mm_load_ps(bufferA + index), _mm_load_ps(bufferB + index)));

                // UPDATE THE INDEX TO POINT TO THE NEXT PIXEL COMPOSED OF 8 FLOATS
                index += 4;
            }
        }
    } else if (scanA.colors() == 8) {
        int index = 0;
        for (unsigned int row = 0; row < numRows; row++) {
            for (unsigned int col = 0; col < numCols; col++) {
                // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
                // AND STORE THE RESULTING VECTORS TO MEMORY
                _mm_stream_ps(bufferR + index + 0, _mm_sub_ps(_mm_load_ps(bufferA + index + 0), _mm_load_ps(bufferB + index + 0)));
                _mm_stream_ps(bufferR + index + 4, _mm_sub_ps(_mm_load_ps(bufferA + index + 4), _mm_load_ps(bufferB + index + 4)));

                // UPDATE THE INDEX TO POINT TO THE NEXT PIXEL COMPOSED OF 8 FLOATS
                index += 8;
            }
        }
    }
    // RETURN THE RESULTING SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::computeMSE(LAUScan scanA, LAUScan scanB)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (scanA.colors() != 8 || scanB.colors() != 8) {
        return (NAN);
    }

    // CREATE SSE VECTOR TO HOLD THE ACCUMULATED SUM OF ERRORS
    __m128 mseVec = _mm_set1_ps(0.0f);

    // ITERATE THROUGH EACH PIXEL COMPARING WITH THE MAX VECTOR AT EACH LOAD
    int index = 0;
    float *bufferA = (float *)scanA.constPointer();
    float *bufferB = (float *)scanB.constPointer();
    for (unsigned int row = 0; row < numRows; row++) {
        for (unsigned int col = 0; col < numCols; col++) {
            // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
            __m128 pixA = _mm_sub_ps(_mm_load_ps(bufferA + index + 0), _mm_load_ps(bufferB + index + 0));
            __m128 pixB = _mm_sub_ps(_mm_load_ps(bufferA + index + 4), _mm_load_ps(bufferB + index + 4));

            // ADD THE SQUARED ERROR VECTOR TO OUR ACCUMULATED MSE VECTOR
            mseVec = _mm_add_ps(mseVec, _mm_mul_ps(pixA, pixA));
            mseVec = _mm_add_ps(mseVec, _mm_mul_ps(pixB, pixB));

            index += 8;
        }
    }

    // EXTRACT THE FLOATS FROM THE VEC4
    float a, b, c, d;
    *(int *)&a = _mm_extract_ps(mseVec, 0);
    *(int *)&b = _mm_extract_ps(mseVec, 1);
    *(int *)&c = _mm_extract_ps(mseVec, 2);
    *(int *)&d = _mm_extract_ps(mseVec, 3);

    // FIND THE LARGEST SCALAR VALUE
    return (a + b + c + d);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::setCodedAperture(QImage image)
{
    if (surface && makeCurrent(surface)) {
        // CREATE AN OPENGL TEXTURE TO HOLD THE CODED APERTURE
        QOpenGLTexture *codedAperture = new QOpenGLTexture(image);
        codedAperture->setWrapMode(QOpenGLTexture::ClampToBorder);
        codedAperture->setMinificationFilter(QOpenGLTexture::Nearest);
        codedAperture->setMagnificationFilter(QOpenGLTexture::Nearest);

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
        // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
        if (frameBufferObjectCodedApertureMask && frameBufferObjectCodedApertureMask->bind()) {
            if (programDx.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, frameBufferObjectCodedApertureMask->width(), frameBufferObjectCodedApertureMask->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        codedAperture->bind();
                        programDx.setUniformValue("qt_texture", 0);
                        programDx.setUniformValue("qt_scale", 1.0f);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(programDx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        programDx.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                programDx.release();
            }
            frameBufferObjectCodedApertureMask->release();
        }

        // DELETE THE OPENGL TEXTURE HOLDING THE CODED APERTURE
        delete codedAperture;
    }
}
