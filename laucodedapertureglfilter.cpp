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
LAUCodedApertureWidget::LAUCodedApertureWidget(LAUScan scn, QWidget *parent) : QWidget(parent), scan(scn), glWidget(NULL), codedApertureFilter(NULL)
{
    // INITIALIZE THE CURVATURE FILTER AND
    // CREATE A GLWIDGET TO DISPLAY THE SCAN
    if (scn.isValid()) {
        codedApertureFilter = new LAUCodedApertureGLFilter(scn.convertToColor(ColorXYZWRGBA));
    }
    glWidget = new LAUCodedApertureGLWidget(scn);
    glWidget->setMinimumSize(scn.width(), scn.height());

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
void LAUCodedApertureWidget::onSetCodedAperture()
{
    QSettings settings;
    QString directory = settings.value("LAUCodedApertureWidget::lastUsedDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
    QString filename = QFileDialog::getOpenFileName(0, QString("Load image from disk (*.bmp)"), directory, QString("*.bmp"));
    if (filename.isEmpty() == false) {
        LAUScan::lastUsedDirectory = QFileInfo(filename).absolutePath();
        settings.setValue("LAUCodedApertureWidget::lastUsedDirectory", LAUScan::lastUsedDirectory);
        if (codedApertureFilter) {
            codedApertureFilter->setCodedAperture(QImage(filename));
        }
    } else {
        return;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureWidget::smoothedScan()
{
    if (codedApertureFilter == NULL) {
        return (LAUScan());
    }

    //LAUScan result = codedApertureFilter->reconstructDataCube(scan);
    LAUScan result = codedApertureFilter->reconstructDataCubeGPU(scan);
    if (inspectScan(result, this)) {
        return (result);
    }
    return (LAUScan());
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
bool LAUCodedApertureWidget::inspectScan(LAUScan scan, QWidget *parent)
{
    if (scan.isValid()) {
        LAUCodedApertureGLWidget *widget = new LAUCodedApertureGLWidget(scan);
        widget->setMinimumSize(scan.width(), scan.height());

        QDialog dialog(parent);
        dialog.setLayout(new QVBoxLayout());
        dialog.setContentsMargins(0, 0, 0, 0);
        dialog.layout()->addWidget(widget);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), &dialog, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), &dialog, SLOT(reject()));

        dialog.layout()->addWidget(buttonBox);
        if (dialog.exec() == QDialog::Accepted) {
            return (true);
        }
    }
    return (false);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLWidget::LAUCodedApertureGLWidget(unsigned int cols, unsigned int rows, QWidget *parent) : QOpenGLWidget(parent), channel(0), scan(LAUScan(cols, rows, ColorXYZWRGBA)), dataCube(NULL)
{
    this->setFocusPolicy(Qt::StrongFocus);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUCodedApertureGLWidget::LAUCodedApertureGLWidget(LAUScan scn, QWidget *parent) : QOpenGLWidget(parent), channel(0), scan(scn), dataCube(NULL)
{
    this->setFocusPolicy(Qt::StrongFocus);
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
void LAUCodedApertureGLWidget::wheelEvent(QWheelEvent *event)
{
    // CHANGE THE ZOOM FACTOR BASED ON HOW MUCH WHEEL HAS MOVED
    channel += qRound((float)event->angleDelta().y() / 160.0);

    // UPDATE THE PROJECTION MATRIX SINCE WE CHANGED THE ZOOM FACTOR
    update();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::onUpdateScan(LAUScan scn)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scn.isValid())  {
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
        if (scan.colors() > 4) {
            dataCube->setSize(2 * scan.width(), scan.height());
        } else {
            dataCube->setSize(scan.width(), scan.height());
        }
        dataCube->setFormat(QOpenGLTexture::RGBA32F);
        dataCube->setWrapMode(QOpenGLTexture::ClampToBorder);
        dataCube->setMinificationFilter(QOpenGLTexture::Nearest);
        dataCube->setMagnificationFilter(QOpenGLTexture::Nearest);
        dataCube->allocateStorage();

        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        if (scan.colors() == 1) {
            dataCube->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());
        } else if (scan.colors() % 3 == 0) {
            dataCube->setData(QOpenGLTexture::RGB, QOpenGLTexture::Float32, (const void *)scan.constPointer());
        } else if (scan.colors() % 4 == 0) {
            dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());
        }

        // UPDATE THE DISPLAY ON SCREEN
        update();
    }
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
    prgrm.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiDisplayXYZWRGBA.vert");
    prgrm.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiDisplayXYZWRGBA.frag");
    prgrm.link();
    setlocale(LC_ALL, "");

    // DISPLAY EXISTING SCAN
    onUpdateScan(scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::resizeGL(int w, int h)
{
    // Get the Desktop Widget so that we can get information about multiple monitors connected to the system.
    QDesktopWidget *dkWidget = QApplication::desktop();
    QList<QScreen *> screenList = QGuiApplication::screens();
    qreal devicePixelRatio = screenList[dkWidget->screenNumber(this)]->devicePixelRatio();
    localHeight = h * devicePixelRatio;
    localWidth = w * devicePixelRatio;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLWidget::paintGL()
{
    // ENABLE THE DEPTH FILTER
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // BIND THE GLSL PROGRAMS RESPONSIBLE FOR CONVERTING OUR FRAME BUFFER
    // OBJECT TO AN XYZ+TEXTURE POINT CLOUD FOR DISPLAY ON SCREEN
    if (dataCube && prgrm.bind()) {
        glViewport(0, 0, localWidth, localHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // BIND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (vertexBuffer.bind()) {
            if (indexBuffer.bind()) {
                // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                glActiveTexture(GL_TEXTURE0);
                dataCube->bind();
                prgrm.setUniformValue("qt_texture", 0);
                prgrm.setUniformValue("qt_channel", channel % scan.colors());

                glVertexAttribPointer(prgrm.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                prgrm.enableAttributeArray("qt_vertex");
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                prgrm.release();
            }
            vertexBuffer.release();
        }
        prgrm.release();
    }
    return;
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
        while (dataCubeFBOs.isEmpty() == false) {
            delete dataCubeFBOs.takeFirst();
        }
        while (spectralMeasurementFBOs.isEmpty() == false) {
            delete spectralMeasurementFBOs.takeFirst();
        }
        if (txtScalarA) {
            delete txtScalarA;
        }
        if (txtScalarB) {
            delete txtScalarB;
        }
        if (fboScalarA) {
            delete fboScalarA;
        }
        if (fboScalarB) {
            delete fboScalarB;
        }
        if (fboScalarC) {
            delete fboScalarC;
        }
        if (fboSpectralScalarA) {
            delete fboScalarA;
        }
        if (fboSpectralScalarB) {
            delete fboScalarB;
        }
        if (fboSpectralScalarC) {
            delete fboScalarC;
        }
        if (fboSpectralModel) {
            delete fboScalarC;
        }

        if (fboDataCubeA) {
            delete fboDataCubeA;
        }
        if (fboDataCubeB) {
            delete fboDataCubeB;
        }
        if (txtCodeAper) {
            delete txtCodeAper;
        }
        if (fboCodeAperLeft) {
            delete fboCodeAperLeft;
        }
        if (fboCodeAperRight) {
            delete fboCodeAperRight;
        }
        if (fboSpectralModel) {
            delete fboSpectralModel;
        }
        if (dataCube) {
            delete dataCube;
        }
        if (csDataCube) {
            delete csDataCube;
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
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

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
    prgrmForwardDCT.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmForwardDCT.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardDCT.frag");
    prgrmForwardDCT.link();

    prgrmReverseDCT.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmReverseDCT.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiReverseDCT.frag");
    prgrmReverseDCT.link();

    prgrmForwardDWTx.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmForwardDWTx.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardDWTx.frag");
    prgrmForwardDWTx.link();

    prgrmForwardDWTy.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmForwardDWTy.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardDWTy.frag");
    prgrmForwardDWTy.link();

    prgrmReverseDWTx.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmReverseDWTx.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiReverseDWTx.frag");
    prgrmReverseDWTx.link();

    prgrmReverseDWTy.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmReverseDWTy.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiReverseDWTy.frag");
    prgrmReverseDWTy.link();

    prgrmForwardCodedAperture.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmForwardCodedAperture.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiForwardCAT.frag");
    prgrmForwardCodedAperture.link();

    prgrmReverseCodedAperture.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmReverseCodedAperture.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiReverseCAT.frag");
    prgrmReverseCodedAperture.link();

    prgrmU.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmU.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMapToVectorU.frag");
    prgrmU.link();

    prgrmV.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
    prgrmV.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMapToVectorV.frag");
    prgrmV.link();

    prgrmAccumMSE.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAccumulativeMSE.vert");
    prgrmAccumMSE.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAccumulativeMSE.frag");
    prgrmAccumMSE.link();

    prgrmAccumNZE.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAccumulativeNoneZeros.vert");
    prgrmAccumNZE.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAccumulativeNoneZeros.frag");
    prgrmAccumNZE.link();

    prgrmAccumMAX.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAccumulativeMAX.vert");
    prgrmAccumMAX.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAccumulativeMAX.frag");
    prgrmAccumMAX.link();

    prgrmAccumMIN.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAccumulativeMIN.vert");
    prgrmAccumMIN.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAccumulativeMIN.frag");
    prgrmAccumMIN.link();

    prgrmAccumSUM.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAccumulativeSUM.vert");
    prgrmAccumSUM.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAccumulativeSUM.frag");
    prgrmAccumSUM.link();

    prgrmSubtract.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiSubtract.vert");
    prgrmSubtract.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiSubtract.frag");
    prgrmSubtract.link();

    prgrmAdd.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAdd.vert");
    prgrmAdd.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAdd.frag");
    prgrmAdd.link();

    prgrmInnerProduct.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiInnerProduct.vert");
    prgrmInnerProduct.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiInnerProduct.frag");
    prgrmInnerProduct.link();

    prgrmSumScans.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiSumScans.vert");
    prgrmSumScans.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiSumScans.frag");
    prgrmSumScans.link();

    prgrmCreateScan.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiCreateScan.vert");
    prgrmCreateScan.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiCreateScan.frag");
    prgrmCreateScan.link();

    prgrmMultiply.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiMultiply.vert");
    prgrmMultiply.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMultiply.frag");
    prgrmMultiply.link();

    prgrmMax.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiMax.vert");
    prgrmMax.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMax.frag");
    prgrmMax.link();

    prgrmMin.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiMin.vert");
    prgrmMin.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiMin.frag");
    prgrmMin.link();

    prgrmAbsMax.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiAccumulativeABSMAX.vert");
    prgrmAbsMax.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiAccumulativeABSMAX.frag");
    prgrmAbsMax.link();

    prgrmCopyScan.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiCopyScan.vert");
    prgrmCopyScan.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiCopyScan.frag");
    prgrmCopyScan.link();


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

    // CREATE A COMPRESSED SPACE DATA CUBE TEXTURE
    csDataCube = new QOpenGLTexture(QOpenGLTexture::Target2D);
    csDataCube->setSize(2 * numCols, numRows);
    csDataCube->setFormat(QOpenGLTexture::RGBA32F);
    csDataCube->setWrapMode(QOpenGLTexture::ClampToBorder);
    csDataCube->setMinificationFilter(QOpenGLTexture::Nearest);
    csDataCube->setMagnificationFilter(QOpenGLTexture::Nearest);
    csDataCube->allocateStorage();

    // CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE CODED APERTURE MEASUREMENT
    spectralMeasurement = new QOpenGLTexture(QOpenGLTexture::Target2D);
    spectralMeasurement->setSize(numCols, numRows);
    spectralMeasurement->setFormat(QOpenGLTexture::R32F);
    spectralMeasurement->setWrapMode(QOpenGLTexture::ClampToBorder);
    spectralMeasurement->setMinificationFilter(QOpenGLTexture::Nearest);
    spectralMeasurement->setMagnificationFilter(QOpenGLTexture::Nearest);
    spectralMeasurement->allocateStorage();

    // CREATE GPU SIDE TEXTURES FOR SCALAR FILTERING OPERATIONS
    txtScalarA = new QOpenGLTexture(QOpenGLTexture::Target2D);
    txtScalarA->setSize(2 * numCols, numRows);
    txtScalarA->setFormat(QOpenGLTexture::RGBA32F);
    txtScalarA->allocateStorage();

    txtScalarB = new QOpenGLTexture(QOpenGLTexture::Target2D);
    txtScalarB->setSize(2 * numCols, numRows);
    txtScalarB->setFormat(QOpenGLTexture::RGBA32F);
    txtScalarB->allocateStorage();

    // CREATE A FRAME BUFFER OBJECT FORMAT INSTANCE FOR THE FBOS
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGBA32F);

    // CREATE THE INTERMEDIATE FBOS FOR THE FORWARD AND INVERSE TRANSFORMS
    fboDataCubeA = new QOpenGLFramebufferObject(2 * numCols, numRows, fboFormat);
    fboDataCubeA->release();

    fboDataCubeB = new QOpenGLFramebufferObject(2 * numCols, numRows, fboFormat);
    fboDataCubeB->release();

    fboCodeAperLeft = new QOpenGLFramebufferObject(2 * numCols, numRows, fboFormat);
    fboCodeAperLeft->release();

    fboCodeAperRight = new QOpenGLFramebufferObject(2 * numCols, numRows, fboFormat);
    fboCodeAperRight->release();

    // CREATE FRAME BUFFER OBJECTS FOR ACCUMULATIVE SUM OPERATIONS BY GENERATING
    // TWO LAYERS OF BUFFERS THAT REDUCE THE SIZE OF THE TARGET BUFFER BY FACTORS
    // OF EIGTH IN THE HEIGHT AND WIDTH DIMENSIONS
    int sWidth = (int)qCeil((float)(fboDataCubeA->width() / 8.0f));
    int sHeight = (int)qCeil((float)(fboDataCubeA->height() / 8.0f));

    fboScalarA = new QOpenGLFramebufferObject(sWidth, sHeight, fboFormat);
    fboScalarA->release();

    sWidth = (int)qCeil((float)(sWidth / 8.0f));
    sHeight = (int)qCeil((float)(sHeight / 8.0f));

    fboScalarB = new QOpenGLFramebufferObject(sWidth, sHeight, fboFormat);
    fboScalarB->release();

    fboScalarC = new QOpenGLFramebufferObject(1, 1, fboFormat);
    fboScalarC->release();

    // CREATE THE FINAL FBO FOR HOLDING THE MONOCHROME OUTPUT
    fboFormat.setInternalTextureFormat(GL_R32F);
    fboSpectralModel = new QOpenGLFramebufferObject(numCols, numRows, fboFormat);
    fboSpectralModel->release();

    sWidth = (int)qCeil((float)(fboSpectralModel->width() / 8.0f));
    sHeight = (int)qCeil((float)(fboSpectralModel->height() / 8.0f));

    fboSpectralScalarA = new QOpenGLFramebufferObject(sWidth, sHeight, fboFormat);
    fboSpectralScalarA->release();

    sWidth = (int)qCeil((float)(sWidth / 8.0f));
    sHeight = (int)qCeil((float)(sHeight / 8.0f));

    fboSpectralScalarB = new QOpenGLFramebufferObject(sWidth, sHeight, fboFormat);
    fboSpectralScalarB->release();

    fboSpectralScalarC = new QOpenGLFramebufferObject(1, 1, fboFormat);
    fboSpectralScalarC->release();

    // CREATE A LIST OF FBOS FOR USE AS NEEDED SO THAT ANYTHING IN THE LIST
    // IS AVAILABLE FOR USE WHILE ANYTHING NOT IN THE LIST IS BEING USED
    for (int n = 0; n < 40; n++) {
    QOpenGLFramebufferObject *fbo = NULL;
    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setInternalTextureFormat(GL_RGBA32F);

    fbo = new QOpenGLFramebufferObject(2 * numCols, numRows, fboFormat);
    fbo->release();
    dataCubeFBOs << fbo;
    }

    for (int n = 0; n < 10; n++) {
    QOpenGLFramebufferObject *fbo = NULL;
    QOpenGLFramebufferObjectFormat fboFormat;

    fboFormat.setInternalTextureFormat(GL_R32F);
    fbo = new QOpenGLFramebufferObject(numCols, numRows, fboFormat);
    fbo->release();
    spectralMeasurementFBOs << fbo;
    }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::initializeParameters()
{
    dataCube = NULL;
    csDataCube = NULL;
    txtScalarA = NULL;
    txtScalarB = NULL;
    fboScalarA = NULL;
    fboScalarB = NULL;
    fboScalarC = NULL;
    txtCodeAper = NULL;
    fboDataCubeA = NULL;
    fboDataCubeB = NULL;
    fboCodeAperLeft = NULL;
    fboCodeAperRight = NULL;
    fboSpectralModel = NULL;
    spectralMeasurement = NULL;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::setCodedAperture(QImage image)
{
    if (surface && makeCurrent(surface)) {
        QOpenGLShaderProgram prgrmA;
        prgrmA.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
        prgrmA.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiBuildCATLeft.frag");
        prgrmA.link();

        QOpenGLShaderProgram prgrmB;
        prgrmB.addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/Shaders/Shaders/cassiVertexShader.vert");
        prgrmB.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/Shaders/cassiBuildCATRight.frag");
        prgrmB.link();

        // CREATE AN OPENGL TEXTURE TO HOLD THE CODED APERTURE
        if (txtCodeAper) {
            delete txtCodeAper;
        }
        txtCodeAper = new QOpenGLTexture(image);
        txtCodeAper->setWrapMode(QOpenGLTexture::ClampToBorder);
        txtCodeAper->setMinificationFilter(QOpenGLTexture::Nearest);
        txtCodeAper->setMagnificationFilter(QOpenGLTexture::Nearest);

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
        // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
        if (fboCodeAperLeft && fboCodeAperLeft->bind()) {
            if (prgrmA.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboCodeAperLeft->width(), fboCodeAperLeft->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        txtCodeAper->bind();
                        prgrmA.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmA.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmA.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmA.release();
            }
            fboCodeAperLeft->release();
        }

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
        // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
        if (fboCodeAperRight && fboCodeAperRight->bind()) {
            if (prgrmB.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboCodeAperRight->width(), fboCodeAperRight->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE CODED APERTURE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboCodeAperLeft->texture());
                        prgrmB.setUniformValue("qt_codedAperture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmB.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmB.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmB.release();
            }
            fboCodeAperRight->release();
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::reconstructDataCubeGPU(LAUScan ideal)
{
    // THE INCOMING SCAN IS THE COMPLETE, IDEAL, PERFECT 3D DATA CUBE
    // WE WANT TO GENERATE A CODED APERTURE ENCODING AND THEN RECONSTRUCT THIS SCAN
    // CALCULATING THE MEAN SQUARED ERROR STEP BY STEP
    LAUScan result;

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    //result = forwardDWCTransform(ideal);
    //result = reverseDWCTransform(result);
    //qDebug() << computeMSE(ideal, result);
    //return (result);

    // INITIALIZE VARIABLES FOR MANAGING GPSR ALGORITHM
    //stopCriterion = SCSmallStepsInNormOfDifference;
    stopCriterion = StopCriterion(1);
    initialization = InitAllZeros;
    debias = false;
    verbose = true;
    monotone = true;
    continuation = false;
    tolA = 0.01;
    tolD = 0.0001;
    alphaMin = 1e-30;
    alphaMax = 1e30;
    maxIterA = 10000;
    minIterA = 3;
    maxIterD = 200;
    minIterD = 5;
    continuationSteps = 0;
    lambda = 1;
    alpha = 1;

    // SO LET'S START BY GENERATING OUR CODED APERTURE ENCODING
    //LAUScan vectorY = reverseCodedAperture(ideal);
    QOpenGLFramebufferObject * fboY = spectralMeasurementFBOs.takeFirst();
    firstreverseCodedAperture(ideal, fboY);

    //LAUScan vectorW = forwardCodedAperture(vectorY);
    QOpenGLFramebufferObject * fboW = dataCubeFBOs.takeFirst();
    forwardCodedAperture(fboY, fboW);

    //LAUScan vectorZ = reverseCodedAperture(vectorW);
    QOpenGLFramebufferObject * fboZ = spectralMeasurementFBOs.takeFirst();
    reverseCodedAperture(fboW, fboZ);


    // NOW CALCULATE THE INITIAL ESTIMATE (LINE 290 OF GPSR_BB SCRIPT)
    //LAUScan vectorXi = forwardTransform(vectorY);
    //LAUScan vectorX = forwardTransform(vectorY);
    QOpenGLFramebufferObject *  fboXi = dataCubeFBOs.takeFirst();
    QOpenGLFramebufferObject *  fbooutcodedapt = dataCubeFBOs.takeFirst();
    forwardCodedAperture( fboY, fbooutcodedapt);
    forwardDWCTransform(fbooutcodedapt, fboXi);

    QOpenGLFramebufferObject *  fboX = dataCubeFBOs.takeFirst();
    forwardCodedAperture( fboY, fbooutcodedapt);
    forwardDWCTransform(fbooutcodedapt, fboX);
    dataCubeFBOs.append(fbooutcodedapt);

    // CALL METHOD FOR CALCULATING THE INITIAL TAU PARAMETER ACCORDING TO  0.5 * max(abs(AT(y)))
    //firstTau = maxAbsValue(vectorXi) / 2.0f;
    firstTau = 0.38;

    // INITIALIZE U AND V VECTORS (LINES 345 AND 346 OF GPSR_BB SCRIPT)
    //LAUScan vectorU = computeVectorU(vectorXi);
    QOpenGLFramebufferObject *  fboU = dataCubeFBOs.takeFirst();
    computeVectorU(fboXi, fboU);

    //LAUScan vectorV = computeVectorV(vectorXi);
    QOpenGLFramebufferObject *  fboV = dataCubeFBOs.takeFirst();
    computeVectorV(fboXi, fboV);

    // GET THE NUMBER OF NON-ZERO ELEMENTS IN X (LINE 350 OF GPSR_BB SCRIPT)
    //int nonZeroCount = nonZeroElements(vectorXi);
    int nonZeroCount = nonZeroElements(fboXi);

    // GET THE GROUND TRUTH X
    //LAUScan grtruth = forwardDWCTransform(ideal);
    QOpenGLFramebufferObject *  fbogrtruth = dataCubeFBOs.takeFirst();
    firstforwardDWCTransform(ideal, fbogrtruth);

    // CALCULATE RESIDUE (LINE 402 OF GPSR_BB SCRIPT)
    //LAUScan vectorAofX = reverseTransform(vectorXi);
    QOpenGLFramebufferObject *  fboAofX = spectralMeasurementFBOs.takeFirst();
    QOpenGLFramebufferObject * fbooutdwt  = dataCubeFBOs.takeFirst();
    reverseDWCTransform(fboXi, fbooutdwt);
    reverseCodedAperture(fbooutdwt, fboAofX);
    dataCubeFBOs.append(fbooutdwt);

    //LAUScan vectorResidue = subtractScans(vectorY, vectorAofX);
    QOpenGLFramebufferObject *  fboResidue = spectralMeasurementFBOs.takeFirst();
    subtractScans(fboY, fboAofX, fboResidue);
    spectralMeasurementFBOs.append(fboAofX);

    iter = 1;
    alpha = 1;

    //COMPUTE INITIAL VALUE OF THE OBJECTIVE FUNCTION (LINE 438 OF GPSR_BB SCRIPT)
    //float f = objectiveFun(vectorResidue, vectorU, vectorV, firstTau);
    float f = objectiveFun(fboResidue, fboU, fboV, firstTau);

    //float mse = computeMSE(grtruth, vectorXi);
    float mse = computeMSE(fbogrtruth, fboXi);

    if (verbose) {
        qDebug() << "Setting firstTau =" << firstTau;
        qDebug() << "Initial MSE =" << mse;
        qDebug() << "Initial obj =" << f << ", nonzeros =" << nonZeroCount;
    }

    // COMPUTE THE INITIAL GRADIENT AND THE USEFUL QUANTITY RESID_BASE (LINE 452 OF GPSR_BB SCRIPT)
    //LAUScan vectorResidueBase = subtractScans(vectorY, vectorResidue);
    QOpenGLFramebufferObject *  fboResidueBase = spectralMeasurementFBOs.takeFirst();
    subtractScans(fboY, fboResidue, fboResidueBase);

    //CONTROL VARIABLE FOR THE OUTER LOOP AND ITER COUNTER
    int keep_going = 1;

    //(LINE 461 OF GPSR_BB SCRIPT)
    while (keep_going) {
        // CALCULATE THE GRADIENT BASED ON THE FORWARD TRANSFORM OF THE RESIDUE_BASE(LINE 464 OF GPSR_BB SCRIPT)
        //LAUScan vectorGradient = forwardTransform(vectorResidueBase);
        QOpenGLFramebufferObject *  fboGradient = dataCubeFBOs.takeFirst();
        QOpenGLFramebufferObject *  fbooutcodedapt = dataCubeFBOs.takeFirst();
        forwardCodedAperture( fboResidueBase, fbooutcodedapt);
        forwardDWCTransform(fbooutcodedapt, fboGradient);
        dataCubeFBOs.append(fbooutcodedapt);


        //LAUScan scantau = createScan(firstTau, vectorGradient);
        QOpenGLFramebufferObject *  fbotau = dataCubeFBOs.takeFirst();
        createScan(firstTau, fboGradient, fbotau);

        //LAUScan term = subtractScans(vectorGradient, vectorX);
        QOpenGLFramebufferObject *  fboterm = dataCubeFBOs.takeFirst();
        subtractScans(fboGradient, fboX, fboterm);
        dataCubeFBOs.append(fboGradient);

        //LAUScan gradu = addScans(term, scantau);
        QOpenGLFramebufferObject *  fbogradu = dataCubeFBOs.takeFirst();
        addScans(fboterm, fbotau, fbogradu);

        //LAUScan gradv = subtractScans(scantau, term);
        QOpenGLFramebufferObject *  fbogradv = dataCubeFBOs.takeFirst();
        subtractScans(fbotau, fboterm, fbogradv);

        dataCubeFBOs.append(fbotau);
        dataCubeFBOs.append(fboterm);


        //PROJECTION AND COMPUTTATION OF SEARCH DIRECTION VECTOR(LINE 471 OF GPSR_BB SCRIPT)
        //LAUScan du = subtractScans(maxScans(subtractScans(vectorU, multiplyScans(alpha, gradu)), createScan(0, gradu)), vectorU);
        //fbodu = subtractScans(maxScans(subtractScans(fboU, multiplyScans(alpha, fbogradu)), createScan(0, fbogradu)), fboU);
        QOpenGLFramebufferObject *  fbodu = dataCubeFBOs.takeFirst();
        QOpenGLFramebufferObject *  fbomul = dataCubeFBOs.takeFirst();
        QOpenGLFramebufferObject *  fbosub = dataCubeFBOs.takeFirst();
        QOpenGLFramebufferObject *  fbocrt = dataCubeFBOs.takeFirst();
        QOpenGLFramebufferObject *  fbomax = dataCubeFBOs.takeFirst();
        multiplyScans(alpha, fbogradu, fbomul);
        subtractScans(fboU, fbomul, fbosub);
        createScan(0, fbogradu, fbocrt);
        maxScans(fbosub, fbocrt, fbomax);
        subtractScans(fbomax, fboU, fbodu);

        //LAUScan dv = subtractScans(maxScans(subtractScans(vectorV, multiplyScans(alpha, gradv)), createScan(0, gradv)), vectorV);
        //fbodv = subtractScans(maxScans(subtractScans(fboV, multiplyScans(alpha, fbogradv)), createScan(0, fbogradv)), fboV);
        QOpenGLFramebufferObject *  fbodv = dataCubeFBOs.takeFirst();
        multiplyScans(alpha, fbogradv, fbomul);
        subtractScans(fboV, fbomul, fbosub);
        createScan(0, fbogradv, fbocrt);
        maxScans(fbosub, fbocrt, fbomax);
        subtractScans(fbomax, fboV, fbodv);

        dataCubeFBOs.append(fbomul);
        dataCubeFBOs.append(fbosub);
        dataCubeFBOs.append(fbocrt);
        dataCubeFBOs.append(fbomax);

        //LAUScan dx = subtractScans(du, dv);
        QOpenGLFramebufferObject *  fbodx = dataCubeFBOs.takeFirst();
        subtractScans(fbodu, fbodv, fbodx);

        //LAUScan old_u(vectorU);
        QOpenGLFramebufferObject *  fboold_u = dataCubeFBOs.takeFirst();
        copyfbo(fboU, fboold_u);

        //LAUScan old_v(vectorV);
        QOpenGLFramebufferObject *  fboold_v = dataCubeFBOs.takeFirst();
        copyfbo(fboV, fboold_v);

        //CALCULATE USEFUL MATRIX-VECTOR PRODUCT INVOLVING dx (LINE 478 OF GPSR_BB SCRIPT)
        //LAUScan auv = reverseTransform(dx);
        // fboauv = reverseTransform(fbodx);
        QOpenGLFramebufferObject *  fboauv = spectralMeasurementFBOs.takeFirst();
        QOpenGLFramebufferObject * fbooutdwt = dataCubeFBOs.takeFirst();
        reverseDWCTransform(fbodx, fbooutdwt);
        reverseCodedAperture(fbooutdwt, fboauv);
        dataCubeFBOs.append(fbooutdwt);

        //float dGd = innerProduct(auv, auv);
        float dGd = innerProduct(fboauv, fboauv);

        if (monotone == true) {
            //float lambda0 = - (innerProduct(gradu, du) + innerProduct(gradv, dv)) / (1e-300 + dGd);
            float lambda0 = - (innerProduct(fbogradu, fbodu) + innerProduct(fbogradv, fbodv)) / (1e-300 + dGd);
            if (lambda0 < 0) {
                qDebug() << "ERROR: lambda0 = " << lambda0 << "Negative. Quit";
                return (0);
            }
            lambda = qMin(lambda0, 1.0f);
        } else {
            lambda = 1;
        }


        //(LINE 494 OF GPSR_BB SCRIPT)
        //vectorU = addScans(old_u, multiplyScans(lambda, du));
        //fboU = addScans(fboold_u, multiplyScans(lambda, fbodu));
        QOpenGLFramebufferObject * fbomul1  = dataCubeFBOs.takeFirst();
        multiplyScans(lambda, fbodu, fbomul1);
        addScans(fboold_u, fbomul1, fboU);


        //vectorV = addScans(old_v, multiplyScans(lambda, dv));
        //fboV = addScans(fboold_v, multiplyScans(lambda, fbodv));
        multiplyScans(lambda, fbodv, fbomul1);
        addScans(fboold_v, fbomul1, fboV);
        dataCubeFBOs.append(fbomul1);


        //LAUScan UVmin = minScans(vectorU, vectorV);
        QOpenGLFramebufferObject *  fboUVmin = dataCubeFBOs.takeFirst();
        minScans(fboU, fboV, fboUVmin);

        //vectorU = subtractScans(vectorU, UVmin);
        subtractScans(fboU, fboUVmin, fboU);

        //vectorV = subtractScans(vectorV, UVmin);
        subtractScans(fboV, fboUVmin, fboV);

        dataCubeFBOs.append(fboUVmin);

        //vectorXi = subtractScans(vectorU, vectorV);
        subtractScans(fboU, fboV, fboXi);

        //CALCULATE NONZERO PATTERN AND NUMBER OF NONZEROS(LINE 502 OF GPSR_BB SCRIPT)
        int prev_nonZeroCount = nonZeroCount;

        //int nonZeroCount = nonZeroElements(vectorXi);
        int nonZeroCount = nonZeroElements(fboXi);

        //UPDATE RESIDUAL AND FUNCTION(LINE 507 OF GPSR_BB SCRIPT)
        //vectorResidue = subtractScans(subtractScans(vectorY, vectorResidueBase), multiplyScans(lambda, auv));
        //fboResidue = subtractScans(subtractScans(fboY, fboResidueBase), multiplyScans(lambda, fboauv));
        QOpenGLFramebufferObject *  fbosub1 = spectralMeasurementFBOs.takeFirst();
        QOpenGLFramebufferObject *  fbomul2 = spectralMeasurementFBOs.takeFirst();
        subtractScans(fboY, fboResidueBase, fbosub1);
        multiplyScans(lambda, fboauv, fbomul2);
        subtractScans(fbosub1, fbomul2, fboResidue);
        spectralMeasurementFBOs.append(fbosub1);
        spectralMeasurementFBOs.append(fbomul2);


        float prev_f = f;
        //f = objectiveFun(vectorResidue, vectorU, vectorV, firstTau);
        f = objectiveFun(fboResidue, fboU, fboV, firstTau);


        // COMPUTER NEW ALPHA(LINE 513 OF GPSR_BB SCRIPT)
        //float dd = innerProduct(du, du) + innerProduct(dv, dv);
        float dd = innerProduct(fbodu, fbodu) + innerProduct(fbodv, fbodv);

        dataCubeFBOs.append(fbodu);
        dataCubeFBOs.append(fbodv);

        if (dGd <= 0) {
            qDebug() << "nonpositive curvature detected dGd = " << dGd;
            alpha = alphaMax;
        } else {
            alpha = qMin(alphaMax, qMax(alphaMin, dd / dGd));
        }

        //LAUScan vectorResidueBase = addScans(vectorResidueBase, multiplyScans(lambda, auv));
        //fboResidueBase = addScans(fboResidueBase, multiplyScans(lambda, fboauv));
        QOpenGLFramebufferObject *  fbomul3 = spectralMeasurementFBOs.takeFirst();
        multiplyScans(lambda, fboauv, fbomul3);
        addScans(fboResidueBase, fbomul3, fboResidueBase);

        spectralMeasurementFBOs.append(fbomul3);
        spectralMeasurementFBOs.append(fboauv);

        if (verbose) {
            qDebug() << "Iter = " << iter << ", obj = " << f << ", lambda = " << lambda << ", alpha = " << alpha << ", nonezeros = " << nonZeroCount << ", MSE= " << mse;
        }

        // UPDATE ITERATION COUNTS (LINE 530 OF GPSR_BB SCRIPT)
        iter = iter + 1;
        mse = computeMSE(fbogrtruth, fboXi);

        // FINAL RECONSTRUCTED SNAPSHOT ON CASSI BY SOLVED X
        result = firstreverseDWCTransform(fboXi);


        if (stopCriterion != 2){
            dataCubeFBOs.append(fbodx);

        }

        if (stopCriterion != 3){
            dataCubeFBOs.append(fboold_u);
            dataCubeFBOs.append(fboold_v);
            dataCubeFBOs.append(fbogradu);
            dataCubeFBOs.append(fbogradv);
        }

        //(LINE 539 OF GPSR_BB SCRIPT)
        switch (stopCriterion) {
            // CRITERION BASED ON THE CHANGE OF THE NUMBER OF NONZERO COMPONENTS OF THE ESTIMATION
            case 0: {
                float num_changes_active = prev_nonZeroCount - nonZeroCount;
                float criterionActiveSet;
                if (nonZeroCount >= 1) {
                    criterionActiveSet = num_changes_active;
                } else {
                    criterionActiveSet = tolA / 2;
                }
                keep_going = (criterionActiveSet > tolA);
                if (verbose) {
                    qDebug() << "Delta nonzeros = " << criterionActiveSet << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE RELATIVE VARIATION OF THE OBJECTIVE FUNCTION
            case 1: {
                float criterionObjective = fabs(f - prev_f) / prev_f;
                keep_going = (criterionObjective > tolA);
                if (verbose) {
                    qDebug() << "Delta obj. = " << criterionObjective << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE RELATIVE NORM OF STEP TAKEN
            case 2: {
                float delta_x_criterion = sqrt(innerProduct(fbodx, fbodx)) / sqrt(innerProduct(fboXi, fboXi));
                keep_going = (delta_x_criterion > tolA);
                if (verbose) {
                    qDebug() << "Norm(delta x)/norm(x) = " << delta_x_criterion << "target = " << tolA;
                }
                dataCubeFBOs.append(fbodx);
                break;
            }
            // CRITERION BASED ON "LCP" - AGAIN BASED ON THE PREVIOUS ITERATE. MAKE IT RELATIVE TO THE NORM OF X
            case 3: {
               // float CriterionLCP = qMax(maxAbsValue(minScans(fbogradu, fboold_u)), maxAbsValue(minScans(fbogradv, fboold_v)));
                QOpenGLFramebufferObject *  fbominu = dataCubeFBOs.takeFirst();
                QOpenGLFramebufferObject *  fbominv = dataCubeFBOs.takeFirst();
                minScans(fbogradu, fboold_u, fbominu);
                minScans(fbogradv, fboold_v, fbominv);
                float CriterionLCP = qMax(maxAbsValue(fbominu), maxAbsValue(fbominv));

                CriterionLCP = CriterionLCP / qMax(1e-6f, qMax(maxAbsValue(fboold_u), maxAbsValue(fboold_v)));
                keep_going = (CriterionLCP > tolA);
                if (verbose) {
                    qDebug() << "LCP = " << CriterionLCP << "target = " << tolA;
                }
                dataCubeFBOs.append(fboold_u);
                dataCubeFBOs.append(fboold_v);
                dataCubeFBOs.append(fbogradu);
                dataCubeFBOs.append(fbogradv);
                dataCubeFBOs.append(fbominu);
                dataCubeFBOs.append(fbominv);

                break;
            }
            // CRITERION BASED ON THE TARGET VALUE OF TOLA
            case 4: {
                keep_going = (f > tolA);
                if (verbose) {
                    qDebug() << "Objective = " << f << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE RELATIVE NORM OF STEP TAKEN
            case 5: {
                float delta_x_criterion_dd = sqrt(dd) / sqrt(innerProduct(fboXi, fboXi));
                keep_going = (delta_x_criterion_dd > tolA);
                if (verbose) {
                    qDebug() << "Norm(delta x)/norm(x) = " << delta_x_criterion_dd << "target = " << tolA;
                }
                break;
            }
            default: {
                qDebug() << "Unknown stopping criterion";
                break;
            }
        }

        if (iter < minIterA) {
            keep_going = 1;
        } else if (iter > maxIterA) {
            keep_going = 0;
        }

        if (verbose && keep_going == 0) {
            qDebug() << "Finished the main algorithm!";
            qDebug() << "Results:";
            qDebug() << "||A x - y ||_2^2 =  " << innerProduct(fboResidue, fboResidue);
            qDebug() << "||x||_1 = " << sumAbsValue(fboU) + sumAbsValue(fboV);
            qDebug() << "Objective function = " << f;
            qDebug() << "Number of non-zero components = " << nonZeroCount;
       }
    }

    spectralMeasurementFBOs.append(fboResidue);
    spectralMeasurementFBOs.append(fboResidueBase);
    spectralMeasurementFBOs.append(fboY);
    spectralMeasurementFBOs.append(fboZ);
    dataCubeFBOs.append(fboU);
    dataCubeFBOs.append(fboV);
    dataCubeFBOs.append(fboW);
    dataCubeFBOs.append(fboXi);
    dataCubeFBOs.append(fboX);
    dataCubeFBOs.append(fbogrtruth);

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
    LAUScan result;

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    //result = forwardDWCTransform(ideal);
    //result = reverseDWCTransform(result);
    //qDebug() << computeMSE(ideal, result);
    //return (result);

    // INITIALIZE VARIABLES FOR MANAGING GPSR ALGORITHM
    //stopCriterion = SCSmallStepsInNormOfDifference;
    stopCriterion = StopCriterion(1);
    initialization = InitAllZeros;
    debias = false;
    verbose = true;
    monotone = true;
    continuation = false;
    tolA = 0.01;
    tolD = 0.0001;
    alphaMin = 1e-30;
    alphaMax = 1e30;
    maxIterA = 10000;
    minIterA = 3;
    maxIterD = 200;
    minIterD = 5;
    continuationSteps = 0;
    lambda = 1;
    alpha = 1;

    // SO LET'S START BY GENERATING OUR CODED APERTURE ENCODING
    LAUScan vectorY = reverseCodedAperture(ideal);
    LAUScan vectorW = forwardCodedAperture(vectorY);
    LAUScan vectorZ = reverseCodedAperture(vectorW);

    // NOW CALCULATE THE INITIAL ESTIMATE (LINE 290 OF GPSR_BB SCRIPT)
    LAUScan vectorXi = forwardTransform(vectorY);
    LAUScan vectorX = forwardTransform(vectorY);

    // CALL METHOD FOR CALCULATING THE INITIAL TAU PARAMETER ACCORDING TO  0.5 * max(abs(AT(y)))
    //firstTau = maxAbsValue(vectorXi) / 2.0f;
    firstTau = 0.38;

    // INITIALIZE U AND V VECTORS (LINES 345 AND 346 OF GPSR_BB SCRIPT)
    LAUScan vectorU = computeVectorU(vectorXi);
    LAUScan vectorV = computeVectorV(vectorXi);

    // GET THE NUMBER OF NON-ZERO ELEMENTS IN X (LINE 350 OF GPSR_BB SCRIPT)
    int nonZeroCount = nonZeroElements(vectorXi);

    // GET THE GROUND TRUTH X
    LAUScan grtruth = forwardDWCTransform(ideal);

    // CALCULATE RESIDUE (LINE 402 OF GPSR_BB SCRIPT)
    LAUScan vectorAofX = reverseTransform(vectorXi);
    LAUScan vectorResidue = subtractScans(vectorY, vectorAofX);
    vectorResidue.save(QString((save_dir) + QString("vectorResidue.tif")));

    iter = 1;
    alpha = 1;
    //COMPUTE INITIAL VALUE OF THE OBJECTIVE FUNCTION (LINE 438 OF GPSR_BB SCRIPT)
    float f = objectiveFun(vectorResidue, vectorU, vectorV, firstTau);
    float mse = computeMSE(grtruth, vectorXi);
    if (verbose) {
        qDebug() << "Setting firstTau =" << firstTau;
        qDebug() << "Initial MSE =" << mse;
        qDebug() << "Initial obj =" << f << ", nonzeros =" << nonZeroCount;
    }

    // COMPUTE THE INITIAL GRADIENT AND THE USEFUL QUANTITY RESID_BASE (LINE 452 OF GPSR_BB SCRIPT)
    LAUScan vectorResidueBase = subtractScans(vectorY, vectorResidue);

    //CONTROL VARIABLE FOR THE OUTER LOOP AND ITER COUNTER
    int keep_going = 1;

    //(LINE 461 OF GPSR_BB SCRIPT)
    while (keep_going) {
        // CALCULATE THE GRADIENT BASED ON THE FORWARD TRANSFORM OF THE RESIDUE_BASE(LINE 464 OF GPSR_BB SCRIPT)
        LAUScan vectorGradient = forwardTransform(vectorResidueBase);
        LAUScan scantau = createScan(firstTau, vectorGradient);
        LAUScan term = subtractScans(vectorGradient, vectorX);
        LAUScan gradu = addScans(term, scantau);
        LAUScan gradv = subtractScans(scantau, term);

        //PROJECTION AND COMPUTTATION OF SEARCH DIRECTION VECTOR(LINE 471 OF GPSR_BB SCRIPT)
        LAUScan du = subtractScans(maxScans(subtractScans(vectorU, multiplyScans(alpha, gradu)), createScan(0, gradu)), vectorU);
        LAUScan dv = subtractScans(maxScans(subtractScans(vectorV, multiplyScans(alpha, gradv)), createScan(0, gradv)), vectorV);
        LAUScan dx = subtractScans(du, dv);
        LAUScan old_u(vectorU);
        LAUScan old_v(vectorV);

        //CALCULATE USEFUL MATRIX-VECTOR PRODUCT INVOLVING dx (LINE 478 OF GPSR_BB SCRIPT)
        LAUScan auv = reverseTransform(dx);
        float dGd = innerProduct(auv, auv);

        if (monotone == true) {
            float lambda0 = - (innerProduct(gradu, du) + innerProduct(gradv, dv)) / (1e-300 + dGd);
            if (lambda0 < 0) {
                qDebug() << "ERROR: lambda0 = " << lambda0 << "Negative. Quit";
                return (LAUScan());
            }
            lambda = qMin(lambda0, 1.0f);
        } else {
            lambda = 1;
        }

        //(LINE 494 OF GPSR_BB SCRIPT)
        vectorU = addScans(old_u, multiplyScans(lambda, du));
        vectorV = addScans(old_v, multiplyScans(lambda, dv));

        LAUScan UVmin = minScans(vectorU, vectorV);

        vectorU = subtractScans(vectorU, UVmin);
        vectorV = subtractScans(vectorV, UVmin);
        vectorXi = subtractScans(vectorU, vectorV);

        //CALCULATE NONZERO PATTERN AND NUMBER OF NONZEROS(LINE 502 OF GPSR_BB SCRIPT)
        int prev_nonZeroCount = nonZeroCount;
        int nonZeroCount = nonZeroElements(vectorXi);

        //UPDATE RESIDUAL AND FUNCTION(LINE 507 OF GPSR_BB SCRIPT)
        vectorResidue = subtractScans(subtractScans(vectorY, vectorResidueBase), multiplyScans(lambda, auv));
        float prev_f = f;
        f = objectiveFun(vectorResidue, vectorU, vectorV, firstTau);

        // COMPUTER NEW ALPHA(LINE 513 OF GPSR_BB SCRIPT)
        float dd = innerProduct(du, du) + innerProduct(dv, dv);

        if (dGd <= 0) {
            qDebug() << "nonpositive curvature detected dGd = " << dGd;
            alpha = alphaMax;
        } else {
            alpha = qMin(alphaMax, qMax(alphaMin, dd / dGd));
        }

        LAUScan temp_vectorResidueBase = addScans(vectorResidueBase, multiplyScans(lambda, auv));
        vectorResidueBase = temp_vectorResidueBase;

        if (verbose) {
            qDebug() << "Iter = " << iter << ", obj = " << f << ", lambda = " << lambda << ", alpha = " << alpha << ", nonezeros = " << nonZeroCount << ", MSE= " << mse;
        }

        // UPDATE ITERATION COUNTS (LINE 530 OF GPSR_BB SCRIPT)
        iter = iter + 1;
        mse = computeMSE(grtruth, vectorXi);

        // FINAL RECONSTRUCTED SNAPSHOT ON CASSI BY SOLVED X
        result = reverseDWCTransform(vectorXi);

        //(LINE 539 OF GPSR_BB SCRIPT)
        switch (stopCriterion) {
            // CRITERION BASED ON THE CHANGE OF THE NUMBER OF NONZERO COMPONENTS OF THE ESTIMATION
            case 0: {
                float num_changes_active = prev_nonZeroCount - nonZeroCount;
                float criterionActiveSet;
                if (nonZeroCount >= 1) {
                    criterionActiveSet = num_changes_active;
                } else {
                    criterionActiveSet = tolA / 2;
                }
                keep_going = (criterionActiveSet > tolA);
                if (verbose) {
                    qDebug() << "Delta nonzeros = " << criterionActiveSet << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE RELATIVE VARIATION OF THE OBJECTIVE FUNCTION
            case 1: {
                float criterionObjective = fabs(f - prev_f) / prev_f;
                keep_going = (criterionObjective > tolA);
                if (verbose) {
                    qDebug() << "Delta obj. = " << criterionObjective << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE RELATIVE NORM OF STEP TAKEN
            case 2: {
                float delta_x_criterion = sqrt(innerProduct(dx, dx)) / sqrt(innerProduct(vectorXi, vectorXi));
                keep_going = (delta_x_criterion > tolA);
                if (verbose) {
                    qDebug() << "Norm(delta x)/norm(x) = " << delta_x_criterion << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON "LCP" - AGAIN BASED ON THE PREVIOUS ITERATE. MAKE IT RELATIVE TO THE NORM OF X
            case 3: {
                float CriterionLCP = qMax(maxAbsValue(minScans(gradu, old_u)), maxAbsValue(minScans(gradv, old_v)));
                CriterionLCP = CriterionLCP / qMax(1e-6f, qMax(maxAbsValue(old_u), maxAbsValue(old_v)));
                keep_going = (CriterionLCP > tolA);
                if (verbose) {
                    qDebug() << "LCP = " << CriterionLCP << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE TARGET VALUE OF TOLA
            case 4: {
                keep_going = (f > tolA);
                if (verbose) {
                    qDebug() << "Objective = " << f << "target = " << tolA;
                }
                break;
            }
            // CRITERION BASED ON THE RELATIVE NORM OF STEP TAKEN
            case 5: {
                float delta_x_criterion_dd = sqrt(dd) / sqrt(innerProduct(vectorXi, vectorXi));
                keep_going = (delta_x_criterion_dd > tolA);
                if (verbose) {
                    qDebug() << "Norm(delta x)/norm(x) = " << delta_x_criterion_dd << "target = " << tolA;
                }
                break;
            }
            default: {
                qDebug() << "Unknown stopping criterion";
                break;
            }
        }

        if (iter < minIterA) {
            keep_going = 1;
        } else if (iter > maxIterA) {
            keep_going = 0;
        }

        if (verbose && keep_going == 0) {
            qDebug() << "Finished the main algorithm!";
            qDebug() << "Results:";
            qDebug() << "||A x - y ||_2^2 =  " << innerProduct(vectorResidue, vectorResidue);
            qDebug() << "||x||_1 = " << sumAbsValue(vectorU) + sumAbsValue(vectorV);
            qDebug() << "Objective function = " << f;
            qDebug() << "Number of non-zero components = " << nonZeroCount;
        }
    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void  LAUCodedApertureGLFilter::firstforwardDWCTransform(LAUScan scan, QOpenGLFramebufferObject *fboDataCubeB, int levels)
{
    // CREATE A RETURN SCAN
    //LAUScan result = LAUScan();

    // FIND THE LARGEST VALUE OF LEVELS SO THAT THE DECOMPOSED IMAGE
    // HAS AND INTEGER NUMBER OF ROWS AND COLUMNS
    if (levels == -1) {
        int rows = scan.height();
        int cols = scan.width();
        while (1) {
            levels++;
            if (rows % 2 || cols % 2) {
                break;
            }
            rows /= 2;
            cols /= 2;
        }
    }

    QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();
    //QOpenGLFramebufferObject * fboDataCubeB  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() == 8 && makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (fboDataCubeB->bind()) {
            if (prgrmForwardDCT.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());
                        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        dataCube->bind();
                        prgrmForwardDCT.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmForwardDCT.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmForwardDCT.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmForwardDCT.release();
            }
            fboDataCubeB->release();
        }

        // RECORD THE SIZE OF THE INCOMING TEXTURE THAT WE INTEND TO DECOMPOSE
        QSize size = QSize(scan.width() * 2, scan.height());
        for (int lvl = 0; lvl < levels; lvl++) {
            if (fboDataCubeA->bind()) {
                if (prgrmForwardDWTx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
                            prgrmForwardDWTx.setUniformValue("qt_texture", 0);
                            prgrmForwardDWTx.setUniformValue("qt_width", size.width());

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardDWTx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardDWTx.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width() / 2, size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTx.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTx.setUniformValue("qt_position", QPointF(QPoint(0, 0)));

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width() / 2, 0, size.width() / 2, size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTx.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTx.setUniformValue("qt_position", QPointF(QPoint(size.width() / 2, 0)));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardDWTx.release();
                }
                fboDataCubeA->release();
            }

            // RESIZE THE SIZE OBJECT TO ACCOUNT FOR NEW DECOMPOSITION LAYER
            size = size / 2;

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboDataCubeB->bind()) {
                if (prgrmForwardDWTy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
                            prgrmForwardDWTy.setUniformValue("qt_texture", 0);
                            prgrmForwardDWTy.setUniformValue("qt_height", size.height() * 2);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardDWTy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardDWTy.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF(0.0, 0.0));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF(0.0, 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, size.height(), size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF(0.0, (float)size.height()));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF(0.0, 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), 0, size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF((float)size.width(), 0.0));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF((float)size.width(), 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), size.height(), size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF((float)size.width(), (float)size.height()));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF((float)size.width(), 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardDWTy.release();
                }
                fboDataCubeB->release();
            }
        }
   }
   dataCubeFBOs.append(fboDataCubeA);
   // RETURN THE UPDATED SCAN
  //  return (fboDataCubeB);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void  LAUCodedApertureGLFilter::forwardDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeB, int levels)
{
    // CREATE A RETURN SCAN
    //LAUScan result = LAUScan();

    // FIND THE LARGEST VALUE OF LEVELS SO THAT THE DECOMPOSED IMAGE
    // HAS AND INTEGER NUMBER OF ROWS AND COLUMNS
    if (levels == -1) {
        int rows = fbo->height();
        int cols = fbo->width()/2;
        while (1) {
            levels++;
            if (rows % 2 || cols % 2) {
                break;
            }
            rows /= 2;
            cols /= 2;
        }
    }

    QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();
    //QOpenGLFramebufferObject * fboDataCubeB  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (fboDataCubeB->bind()) {
            if (prgrmForwardDCT.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                       // dataCube->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmForwardDCT.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmForwardDCT.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmForwardDCT.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmForwardDCT.release();
            }
            fboDataCubeB->release();
        }

        // RECORD THE SIZE OF THE INCOMING TEXTURE THAT WE INTEND TO DECOMPOSE
        QSize size = QSize(fbo->width(), fbo->height());
        for (int lvl = 0; lvl < levels; lvl++) {
            if (fboDataCubeA->bind()) {
                if (prgrmForwardDWTx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
                            prgrmForwardDWTx.setUniformValue("qt_texture", 0);
                            prgrmForwardDWTx.setUniformValue("qt_width", size.width());

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardDWTx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardDWTx.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width() / 2, size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTx.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTx.setUniformValue("qt_position", QPointF(QPoint(0, 0)));

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width() / 2, 0, size.width() / 2, size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTx.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTx.setUniformValue("qt_position", QPointF(QPoint(size.width() / 2, 0)));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardDWTx.release();
                }
                fboDataCubeA->release();
            }

            // RESIZE THE SIZE OBJECT TO ACCOUNT FOR NEW DECOMPOSITION LAYER
            size = size / 2;

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboDataCubeB->bind()) {
                if (prgrmForwardDWTy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
                            prgrmForwardDWTy.setUniformValue("qt_texture", 0);
                            prgrmForwardDWTy.setUniformValue("qt_height", size.height() * 2);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardDWTy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardDWTy.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF(0.0, 0.0));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF(0.0, 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, size.height(), size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF(0.0, (float)size.height()));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF(0.0, 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), 0, size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF((float)size.width(), 0.0));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF((float)size.width(), 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), size.height(), size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF((float)size.width(), (float)size.height()));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF((float)size.width(), 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardDWTy.release();
                }
                fboDataCubeB->release();
            }
        }

    }
    dataCubeFBOs.append(fboDataCubeA);
    // RETURN THE UPDATED SCAN
//   return (fboDataCubeB);
}
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::forwardDWCTransform(LAUScan scan, int levels)
{
    // CREATE A RETURN SCAN
    LAUScan result = LAUScan();

    // FIND THE LARGEST VALUE OF LEVELS SO THAT THE DECOMPOSED IMAGE
    // HAS AND INTEGER NUMBER OF ROWS AND COLUMNS
    if (levels == -1) {
        int rows = scan.height();
        int cols = scan.width();
        while (1) {
            levels++;
            if (rows % 2 || cols % 2) {
                break;
            }
            rows /= 2;
            cols /= 2;
        }
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() == 8 && makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (fboDataCubeB->bind()) {
            if (prgrmForwardDCT.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        dataCube->bind();
                        prgrmForwardDCT.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmForwardDCT.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmForwardDCT.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmForwardDCT.release();
            }
            fboDataCubeB->release();
        }

        // RECORD THE SIZE OF THE INCOMING TEXTURE THAT WE INTEND TO DECOMPOSE
        QSize size = QSize(scan.width() * 2, scan.height());
        for (int lvl = 0; lvl < levels; lvl++) {
            if (fboDataCubeA->bind()) {
                if (prgrmForwardDWTx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
                            prgrmForwardDWTx.setUniformValue("qt_texture", 0);
                            prgrmForwardDWTx.setUniformValue("qt_width", size.width());

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardDWTx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardDWTx.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width() / 2, size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTx.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTx.setUniformValue("qt_position", QPointF(QPoint(0, 0)));

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width() / 2, 0, size.width() / 2, size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTx.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTx.setUniformValue("qt_position", QPointF(QPoint(size.width() / 2, 0)));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardDWTx.release();
                }
                fboDataCubeA->release();
            }

            // RESIZE THE SIZE OBJECT TO ACCOUNT FOR NEW DECOMPOSITION LAYER
            size = size / 2;

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboDataCubeB->bind()) {
                if (prgrmForwardDWTy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
                            prgrmForwardDWTy.setUniformValue("qt_texture", 0);
                            prgrmForwardDWTy.setUniformValue("qt_height", size.height() * 2);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardDWTy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardDWTy.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF(0.0, 0.0));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF(0.0, 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, size.height(), size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF(0.0, (float)size.height()));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF(0.0, 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), 0, size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", LoD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF((float)size.width(), 0.0));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF((float)size.width(), 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), size.height(), size.width(), size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmForwardDWTy.setUniformValueArray("qt_coefficients", HiD, 16, 1);
                            prgrmForwardDWTy.setUniformValue("qt_position", QPointF((float)size.width(), (float)size.height()));
                            prgrmForwardDWTy.setUniformValue("qt_offset", QPointF((float)size.width(), 0.0));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardDWTy.release();
                }
                fboDataCubeB->release();
            }
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        result = LAUScan(fboDataCubeA->width() / 2, fboDataCubeA->height(), ColorXYZWRGBA);
        glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)result.pointer());

        // RELEASE THE CURRENT OPENGL CONTEXT
        doneCurrent();
    }
    // RETURN THE UPDATED SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::firstreverseDWCTransform(QOpenGLFramebufferObject *fbo, int levels)
{
    // CREATE A RETURN SCAN
    LAUScan result = LAUScan();

    // FIND THE LARGEST VALUE OF LEVELS SO THAT THE DECOMPOSED IMAGE
    // HAS AND INTEGER NUMBER OF ROWS AND COLUMNS
    if (levels == -1) {
        int rows = fbo->height();
        int cols = fbo->width()/2;
        while (1) {
            levels++;
            if (rows % 2 || cols % 2) {
                break;
            }
            rows /= 2;
            cols /= 2;
        }
    }

    QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();
    QOpenGLFramebufferObject * fboDataCubeB  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (fboDataCubeB->bind()) {
            if (prgrmReverseDCT.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        //csDataCube->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmReverseDCT.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmReverseDCT.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmReverseDCT.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmReverseDCT.release();
            }
            fboDataCubeB->release();
        }

        // RECORD THE SIZE OF THE INCOMING TEXTURE THAT WE INTEND TO DECOMPOSE
        QSize size =  QSize(fbo->width(), fbo->height());
        for (int lvl = 0; lvl < levels; lvl++) {
            size = size / 2;
        }

        // ITERATE THROUGH ALL LEVELS OF THE DWT
        for (int lvl = 0; lvl < levels; lvl++) {
            if (fboDataCubeA->bind()) {
                if (prgrmReverseDWTy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
                            prgrmReverseDWTy.setUniformValue("qt_texture", 0);

                            // TELL THE SHADER HOW MANY ROWS ARE IN EACH COMPONENT TEXTURE
                            prgrmReverseDWTy.setUniformValue("qt_height", size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmReverseDWTy.setUniformValueArray("qt_coefficientsA", LoR, 16, 1);
                            prgrmReverseDWTy.setUniformValueArray("qt_coefficientsB", HiR, 16, 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseDWTy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseDWTy.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), 2 * size.height());

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTy.setUniformValue("qt_position", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetA", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetB", QPointF(QPoint(0, size.height())));

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), 0, size.width(), 2 * size.height());

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTy.setUniformValue("qt_position", QPointF(QPoint(size.width(), 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetA", QPointF(QPoint(size.width(), 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetB", QPointF(QPoint(size.width(), size.height())));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseDWTy.release();
                }
                fboDataCubeA->release();
            }

            // RESIZE THE SIZE OBJECT TO ACCOUNT FOR NEW DECOMPOSITION LAYER
            size = size * 2;

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboDataCubeB->bind()) {
                if (prgrmReverseDWTx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), size.height());

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
                            prgrmReverseDWTx.setUniformValue("qt_texture", 0);
                            prgrmReverseDWTx.setUniformValue("qt_width", size.width() / 2);

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmReverseDWTx.setUniformValueArray("qt_coefficientsA", LoR, 16, 1);
                            prgrmReverseDWTx.setUniformValueArray("qt_coefficientsB", HiR, 16, 1);

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTx.setUniformValue("qt_offsetA", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTx.setUniformValue("qt_offsetB", QPointF(QPoint(size.width() / 2, 0)));

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseDWTx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseDWTx.enableAttributeArray("qt_vertex");

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseDWTx.release();
                }
                fboDataCubeB->release();
            }
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        result = LAUScan(fboDataCubeB->width() / 2, fboDataCubeB->height(), ColorXYZWRGBA);
        glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)result.pointer());

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
       //doneCurrent();
    }

    // RETURN THE UPDATED SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::reverseDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeB, int levels)
{
    // CREATE A RETURN SCAN
    //LAUScan result = LAUScan();

    // FIND THE LARGEST VALUE OF LEVELS SO THAT THE DECOMPOSED IMAGE
    // HAS AND INTEGER NUMBER OF ROWS AND COLUMNS
    if (levels == -1) {
        int rows = fbo->height();
        int cols = fbo->width()/2;
        while (1) {
            levels++;
            if (rows % 2 || cols % 2) {
                break;
            }
            rows /= 2;
            cols /= 2;
        }
    }

    QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();
   //QOpenGLFramebufferObject * fboDataCubeB  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (fboDataCubeB->bind()) {
            if (prgrmReverseDCT.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        //csDataCube->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmReverseDCT.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmReverseDCT.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmReverseDCT.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmReverseDCT.release();
            }
            fboDataCubeB->release();
        }

        // RECORD THE SIZE OF THE INCOMING TEXTURE THAT WE INTEND TO DECOMPOSE
        QSize size =  QSize(fbo->width(), fbo->height());
        for (int lvl = 0; lvl < levels; lvl++) {
            size = size / 2;
        }

        // ITERATE THROUGH ALL LEVELS OF THE DWT
        for (int lvl = 0; lvl < levels; lvl++) {
            if (fboDataCubeA->bind()) {
                if (prgrmReverseDWTy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
                            prgrmReverseDWTy.setUniformValue("qt_texture", 0);

                            // TELL THE SHADER HOW MANY ROWS ARE IN EACH COMPONENT TEXTURE
                            prgrmReverseDWTy.setUniformValue("qt_height", size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmReverseDWTy.setUniformValueArray("qt_coefficientsA", LoR, 16, 1);
                            prgrmReverseDWTy.setUniformValueArray("qt_coefficientsB", HiR, 16, 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseDWTy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseDWTy.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), 2 * size.height());

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTy.setUniformValue("qt_position", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetA", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetB", QPointF(QPoint(0, size.height())));

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), 0, size.width(), 2 * size.height());

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTy.setUniformValue("qt_position", QPointF(QPoint(size.width(), 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetA", QPointF(QPoint(size.width(), 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetB", QPointF(QPoint(size.width(), size.height())));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseDWTy.release();
                }
                fboDataCubeA->release();
            }

            // RESIZE THE SIZE OBJECT TO ACCOUNT FOR NEW DECOMPOSITION LAYER
            size = size * 2;

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboDataCubeB->bind()) {
                if (prgrmReverseDWTx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), size.height());

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
                            prgrmReverseDWTx.setUniformValue("qt_texture", 0);
                            prgrmReverseDWTx.setUniformValue("qt_width", size.width() / 2);

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmReverseDWTx.setUniformValueArray("qt_coefficientsA", LoR, 16, 1);
                            prgrmReverseDWTx.setUniformValueArray("qt_coefficientsB", HiR, 16, 1);

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTx.setUniformValue("qt_offsetA", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTx.setUniformValue("qt_offsetB", QPointF(QPoint(size.width() / 2, 0)));

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseDWTx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseDWTx.enableAttributeArray("qt_vertex");

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseDWTx.release();
                }
                fboDataCubeB->release();
            }
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
       //doneCurrent();
    }
    dataCubeFBOs.append(fboDataCubeA);
    // RETURN THE UPDATED SCAN
    //return (fboDataCubeB);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::reverseDWCTransform(LAUScan scan, int levels)
{
    // CREATE A RETURN SCAN
    LAUScan result = LAUScan();

    // FIND THE LARGEST VALUE OF LEVELS SO THAT THE DECOMPOSED IMAGE
    // HAS AND INTEGER NUMBER OF ROWS AND COLUMNS
    if (levels == -1) {
        int rows = scan.height();
        int cols = scan.width();
        while (1) {
            levels++;
            if (rows % 2 || cols % 2) {
                break;
            }
            rows /= 2;
            cols /= 2;
        }
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() == 8 && makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT, SHADER, AND VBOS FOR CALCULATING THE DCT ACROSS CHANNELS
        if (fboDataCubeB->bind()) {
            if (prgrmReverseDCT.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        csDataCube->bind();
                        prgrmReverseDCT.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmReverseDCT.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmReverseDCT.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmReverseDCT.release();
            }
            fboDataCubeB->release();
        }

        // RECORD THE SIZE OF THE INCOMING TEXTURE THAT WE INTEND TO DECOMPOSE
        QSize size = QSize(scan.width() * 2, scan.height());
        for (int lvl = 0; lvl < levels; lvl++) {
            size = size / 2;
        }

        // ITERATE THROUGH ALL LEVELS OF THE DWT
        for (int lvl = 0; lvl < levels; lvl++) {
            if (fboDataCubeA->bind()) {
                if (prgrmReverseDWTy.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
                            prgrmReverseDWTy.setUniformValue("qt_texture", 0);

                            // TELL THE SHADER HOW MANY ROWS ARE IN EACH COMPONENT TEXTURE
                            prgrmReverseDWTy.setUniformValue("qt_height", size.height());

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmReverseDWTy.setUniformValueArray("qt_coefficientsA", LoR, 16, 1);
                            prgrmReverseDWTy.setUniformValueArray("qt_coefficientsB", HiR, 16, 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseDWTy.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseDWTy.enableAttributeArray("qt_vertex");

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), 2 * size.height());

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTy.setUniformValue("qt_position", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetA", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetB", QPointF(QPoint(0, size.height())));

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(size.width(), 0, size.width(), 2 * size.height());

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTy.setUniformValue("qt_position", QPointF(QPoint(size.width(), 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetA", QPointF(QPoint(size.width(), 0)));
                            prgrmReverseDWTy.setUniformValue("qt_offsetB", QPointF(QPoint(size.width(), size.height())));

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseDWTy.release();
                }
                fboDataCubeA->release();
            }

            // RESIZE THE SIZE OBJECT TO ACCOUNT FOR NEW DECOMPOSITION LAYER
            size = size * 2;

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboDataCubeB->bind()) {
                if (prgrmReverseDWTx.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, size.width(), size.height());

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
                            prgrmReverseDWTx.setUniformValue("qt_texture", 0);
                            prgrmReverseDWTx.setUniformValue("qt_width", size.width() / 2);

                            // SET THE COEFFICIENTS TO THE LOW-PASS DECOMPOSITION SYMMLET 8 FILTER
                            prgrmReverseDWTx.setUniformValueArray("qt_coefficientsA", LoR, 16, 1);
                            prgrmReverseDWTx.setUniformValueArray("qt_coefficientsB", HiR, 16, 1);

                            // SET THE TOP RIGHT CORNER OF THE COMPONENT TEXTURES
                            prgrmReverseDWTx.setUniformValue("qt_offsetA", QPointF(QPoint(0, 0)));
                            prgrmReverseDWTx.setUniformValue("qt_offsetB", QPointF(QPoint(size.width() / 2, 0)));

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseDWTx.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseDWTx.enableAttributeArray("qt_vertex");

                            // DRAW THE TRIANGLES TO ACTIVATE THE PROCESS
                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseDWTx.release();
                }
                fboDataCubeB->release();
            }
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        result = LAUScan(fboDataCubeB->width() / 2, fboDataCubeB->height(), ColorXYZWRGBA);
        glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)result.pointer());
        result.save(QString((save_dir) + QString("result.tif")));

//        LAUMemoryObject  idwt(fboDataCubeB->width(), fboDataCubeB->height(), 4, sizeof(float));
//        glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
//        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)idwt.pointer());
//        idwt.save(QString((save_dir) + QString("idwt.tif")));


        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        doneCurrent();
    }

    // RETURN THE UPDATED SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::forwardCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboDataCubeA)
{
    // CREATE AN OUTPUT SCAN
    //LAUScan result = LAUScan();
     //QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
     if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
           // spectralMeasurement->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
            // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
            if (fboDataCubeA && fboDataCubeA->bind()) {
                if (prgrmForwardCodedAperture.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                            glActiveTexture(GL_TEXTURE0);
                            //spectralMeasurement->bind();
                            glBindTexture(GL_TEXTURE_2D, fbo->texture());
                            prgrmForwardCodedAperture.setUniformValue("qt_texture", 0);

                            // BIND THE CODED APERTURE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, fboCodeAperRight->texture());
                            prgrmForwardCodedAperture.setUniformValue("qt_codedAperture", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardCodedAperture.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardCodedAperture.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardCodedAperture.release();
                }
                fboDataCubeA->release();
            }
        }
  //  return (fboDataCubeA);
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
            if (fboDataCubeA && fboDataCubeA->bind()) {
                if (prgrmForwardCodedAperture.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                            glActiveTexture(GL_TEXTURE0);
                            spectralMeasurement->bind();
                            prgrmForwardCodedAperture.setUniformValue("qt_texture", 0);

                            // BIND THE CODED APERTURE TEXTURE FROM THE FRAME BUFFER OBJECT
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, fboCodeAperRight->texture());
                            prgrmForwardCodedAperture.setUniformValue("qt_codedAperture", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmForwardCodedAperture.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmForwardCodedAperture.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmForwardCodedAperture.release();
                }
                fboDataCubeA->release();
            }

            // CREATE A NEW MONOCHROME SCAN
            result = LAUScan(scan.width(), scan.height(), ColorXYZWRGBA);
            glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)result.pointer());
        }
    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::firstreverseCodedAperture(LAUScan scan,  QOpenGLFramebufferObject * fboSpectralModel)
{
    // CREATE AN OUTPUT SCAN
    //LAUScan result = LAUScan();

     //QOpenGLFramebufferObject * fboSpectralModel  = spectralMeasurementFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (scan.colors() == 8) {
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
            // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
            if (fboSpectralModel && fboSpectralModel->bind()) {
                if (prgrmReverseCodedAperture.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboSpectralModel->width(), fboSpectralModel->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                            glActiveTexture(GL_TEXTURE0);
                            dataCube->bind();
                            prgrmReverseCodedAperture.setUniformValue("qt_texture", 0);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE CODED APERTURE MASK
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, fboCodeAperLeft->texture());
                            prgrmReverseCodedAperture.setUniformValue("qt_codedAperture", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseCodedAperture.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseCodedAperture.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseCodedAperture.release();
                }
                fboSpectralModel->release();
            }
        }
    }
    // RETURN A NEW MONOCHROME SCAN

   // return (fboSpectralModel);
}

 /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
void LAUCodedApertureGLFilter::reverseCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboSpectralModel)
  {
      // CREATE AN OUTPUT SCAN
      //LAUScan result = LAUScan();
     //QOpenGLFramebufferObject * fboSpectralModel  = spectralMeasurementFBOs.takeFirst();

      // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
      if (fbo->width() == 2 * numCols) {
          if (makeCurrent(surface)) {
              // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
              //dataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

              // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH THE SHADER
              // AND VBOS FOR BUILDING THE SKEWED CODED APERATURE MASK FBO
              if (fboSpectralModel && fboSpectralModel->bind()) {
                  if (prgrmReverseCodedAperture.bind()) {
                      if (vertexBuffer.bind()) {
                          if (indexBuffer.bind()) {
                              // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                              glViewport(0, 0, fboSpectralModel->width(), fboSpectralModel->height());
                              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                              // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                              glActiveTexture(GL_TEXTURE0);
                              //dataCube->bind();
                              glBindTexture(GL_TEXTURE_2D, fbo->texture());
                              prgrmReverseCodedAperture.setUniformValue("qt_texture", 0);

                              // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE CODED APERTURE MASK
                              glActiveTexture(GL_TEXTURE1);
                              glBindTexture(GL_TEXTURE_2D, fboCodeAperLeft->texture());
                              prgrmReverseCodedAperture.setUniformValue("qt_codedAperture", 1);

                              // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                              glVertexAttribPointer(prgrmReverseCodedAperture.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                              prgrmReverseCodedAperture.enableAttributeArray("qt_vertex");

                              glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                              // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                              indexBuffer.release();
                          }
                          vertexBuffer.release();
                      }
                      prgrmReverseCodedAperture.release();
                  }
                  fboSpectralModel->release();
              }

          }
      }
   //   return (fboSpectralModel);
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
            if (fboSpectralModel && fboSpectralModel->bind()) {
                if (prgrmReverseCodedAperture.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboSpectralModel->width(), fboSpectralModel->height());
                            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE INPUT IMAGE
                            glActiveTexture(GL_TEXTURE0);
                            dataCube->bind();
                            prgrmReverseCodedAperture.setUniformValue("qt_texture", 0);

                            // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT FOR THE CODED APERTURE MASK
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_2D, fboCodeAperLeft->texture());
                            prgrmReverseCodedAperture.setUniformValue("qt_codedAperture", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmReverseCodedAperture.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmReverseCodedAperture.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmReverseCodedAperture.release();
                }
                fboSpectralModel->release();
            }

            // CREATE A NEW MONOCHROME SCAN
            result = LAUScan(scan.width(), scan.height(), ColorGray);
            glBindTexture(GL_TEXTURE_2D, fboSpectralModel->texture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (unsigned char *)result.pointer());
        }
    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::computeVectorU(QOpenGLFramebufferObject *fbo,  QOpenGLFramebufferObject * fboDataCubeA)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (fbo->width() != 2* numCols)  {
        qDebug()<<"someting wrong with U";
    }

    //QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeA->bind()) {
            if (prgrmU.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        //csDataCube->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmU.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmU.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmU.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmU.release();
            }
            fboDataCubeA->release();
        }
    }
  //  return (fboDataCubeA);
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
        csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeA->bind()) {
            if (prgrmU.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        csDataCube->bind();
                        prgrmU.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmU.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmU.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmU.release();
            }
            fboDataCubeA->release();
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        glBindTexture(GL_TEXTURE_2D, fboDataCubeA->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)scan.pointer());
        //scan.save(QString((save_dir) + QString("scan.tif")));
        //doneCurrent();
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::computeVectorV(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboDataCubeB)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (fbo->width() != 2* numCols)  {
        qDebug()<<"someting wrong with V";
    }

    //QOpenGLFramebufferObject * fboDataCubeB  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeB->bind()) {
            if (prgrmV.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        //csDataCube->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmV.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmV.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmV.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmV.release();
            }
            fboDataCubeB->release();
        }

    }
 //   return (fboDataCubeB);
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
        csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeB->bind()) {
            if (prgrmV.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeB->width(), fboDataCubeB->height());

                        // BIND THE TEXTURE FROM THE FRAME BUFFER OBJECT
                        glActiveTexture(GL_TEXTURE0);
                        csDataCube->bind();
                        prgrmV.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmV.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmV.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmV.release();
            }
            fboDataCubeB->release();
        }

        // DOWNLOAD THE GPU RESULT BACK TO THE CPU
        glBindTexture(GL_TEXTURE_2D, fboDataCubeB->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)scan.pointer());
        doneCurrent();
    }
    return (scan);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::maxAbsValue(QOpenGLFramebufferObject *fbo)
{

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

        // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
        if (fboSpectralScalarC->bind()) {
            if (prgrmAbsMax.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboSpectralScalarC->width(), fboSpectralScalarC->height());
                        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        //txtScalarA->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmAbsMax.setUniformValue("qt_texture", 0);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmAbsMax.setUniformValue("qt_blockSize", QPointF(QPoint(2 * numCols, numRows)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAbsMax.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAbsMax.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAbsMax.release();
            }
            fboSpectralScalarC->release();
        }
    }
    float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
    glBindTexture(GL_TEXTURE_2D, fboSpectralScalarC->texture());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

    // COMBINE THE VEC4 INTO A SINGLE SCALAR
     return(qMax(mse[0], qMax(mse[1], qMax(mse[2], mse[3]))));
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
    for (unsigned int row = 0; row < scan.height(); row++) {
        for (unsigned int col = 0; col < scan.width(); col++) {
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
float LAUCodedApertureGLFilter::sumAbsValue(QOpenGLFramebufferObject *fbo)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

        // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
        if (fboSpectralScalarC->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboSpectralScalarC->width(), fboSpectralScalarC->height());
                        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        //txtScalarA->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(2 * numCols, numRows)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboSpectralScalarC->release();
        }
    }
    float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
    glBindTexture(GL_TEXTURE_2D, fboSpectralScalarC->texture());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

    // COMBINE THE VEC4 INTO A SINGLE SCALAR
    return(mse[0] + mse[1] + mse[2] + mse[3]);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::sumAbsValue(LAUScan scan)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (scan.colors() != 8) {
        return (NAN);
    }

    // CREATE SSE VECTOR TO HOLD THE SUM VALUES OVER TWO VEC4s
    __m128 sumVec = _mm_set1_ps(0.0f);

    // WE NEED THIS TO PERFORM THE ABSOLUTE VALUE OPERATION FOR SINGLE PRECISION FLOATING POINT
    static const __m128 sgnVec = _mm_set1_ps(-0.0f);

    // ITERATE THROUGH EACH PIXEL COMPARING WITH THE MAX VECTOR AT EACH LOAD
    int index = 0;
    float *buffer = (float *)scan.constPointer();
    for (unsigned int row = 0; row < scan.height(); row++) {
        for (unsigned int col = 0; col < scan.width(); col++) {
            sumVec = _mm_add_ps(sumVec, _mm_andnot_ps(sgnVec, _mm_load_ps(buffer + index + 0)));
            sumVec = _mm_add_ps(sumVec, _mm_andnot_ps(sgnVec, _mm_load_ps(buffer + index + 4)));
            index += 8;
        }
    }

    // EXTRACT THE FLOATS FROM THE VEC4
    float a, b, c, d;
    *(int *)&a = _mm_extract_ps(sumVec, 0);
    *(int *)&b = _mm_extract_ps(sumVec, 1);
    *(int *)&c = _mm_extract_ps(sumVec, 2);
    *(int *)&d = _mm_extract_ps(sumVec, 3);

    return (a + b + c + d);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
 int LAUCodedApertureGLFilter::nonZeroElements(QOpenGLFramebufferObject *fbo)
 {
     // CREATE A VARIABLE TO HOLD THE RETURNED VALUE
     float result = NAN;

     // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
     if (makeCurrent(surface)) {

         // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
         if (fboScalarA->bind()) {
             if (prgrmAccumNZE.bind()) {
                 if (vertexBuffer.bind()) {
                     if (indexBuffer.bind()) {
                         // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                         glViewport(0, 0, fboScalarA->width(), fboScalarA->height());
                         //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                         // BIND THE TEXTURES FOR THE FILTERING OPERATION
                         glActiveTexture(GL_TEXTURE0);
                         //txtScalarA->bind();
                         glBindTexture(GL_TEXTURE_2D, fbo->texture());
                         prgrmAccumNZE.setUniformValue("qt_texture", 0);

                         // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                         prgrmAccumNZE.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                         // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                         glVertexAttribPointer(prgrmAccumNZE.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                         prgrmAccumNZE.enableAttributeArray("qt_vertex");

                         glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                         // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                         indexBuffer.release();
                     }
                     vertexBuffer.release();
                 }
                 prgrmAccumNZE.release();
             }
             fboScalarA->release();
         }

         // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
         // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
         if (fboScalarB->bind()) {
             if (prgrmAccumSUM.bind()) {
                 if (vertexBuffer.bind()) {
                     if (indexBuffer.bind()) {
                         // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                         glViewport(0, 0, fboScalarB->width(), fboScalarB->height());
                         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                         // BIND THE TEXTURES FOR THE FILTERING OPERATION
                         glActiveTexture(GL_TEXTURE0);
                         glBindTexture(GL_TEXTURE_2D, fboScalarA->texture());
                         prgrmAccumSUM.setUniformValue("qt_texture", 0);

                         // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                         prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                         // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                         glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                         prgrmAccumSUM.enableAttributeArray("qt_vertex");

                         glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                         // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                         indexBuffer.release();
                     }
                     vertexBuffer.release();
                 }
                 prgrmAccumSUM.release();
             }
             fboScalarB->release();
         }

         // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
         // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
         if (fboScalarC->bind()) {
             if (prgrmAccumSUM.bind()) {
                 if (vertexBuffer.bind()) {
                     if (indexBuffer.bind()) {
                         // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                         glViewport(0, 0, fboScalarC->width(), fboScalarC->height());
                         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                         // BIND THE TEXTURES FOR THE FILTERING OPERATION
                         glActiveTexture(GL_TEXTURE0);
                         glBindTexture(GL_TEXTURE_2D, fboScalarB->texture());
                         prgrmAccumSUM.setUniformValue("qt_texture", 0);

                         // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                         prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(fboScalarB->width(), fboScalarB->height())));

                         // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                         glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                         prgrmAccumSUM.enableAttributeArray("qt_vertex");

                         glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                         // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                         indexBuffer.release();
                     }
                     vertexBuffer.release();
                 }
                 prgrmAccumSUM.release();
             }
             fboScalarC->release();
         }

         // CREATE A VECTOR TO HOLD THE INCOMING GPU FRAME BUFFER OBJECT
         float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

         // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
         glBindTexture(GL_TEXTURE_2D, fboScalarC->texture());
         glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

         // COMBINE THE VEC4 INTO A SINGLE SCALAR
         result = mse[0] + mse[1] + mse[2] + mse[3];

         // RELEASE THE CURRENT OPENGL CONTEXT
         //doneCurrent();
     }
     return (result);
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
    static const __m128 zerVec = _mm_set1_ps(0.0f);

    // ITERATE THROUGH EACH PIXEL COMPARING WITH THE ZERO VECTOR AT EACH LOAD
    // AND ADDING THE LOGICAL RESULT TO OUR INTEGER ACCUMULATION VECTOR
    int index = 0;
    float *buffer = (float *)scan.constPointer();
    for (unsigned int row = 0; row < scan.height(); row++) {
        for (unsigned int col = 0; col < scan.width(); col++) {
            pixVec = _mm_add_epi32(pixVec, _mm_castps_si128(_mm_cmpneq_ps(zerVec, _mm_load_ps(buffer + index + 0))));
            pixVec = _mm_add_epi32(pixVec, _mm_castps_si128(_mm_cmpneq_ps(zerVec, _mm_load_ps(buffer + index + 4))));
            index += 8;
        }
    }
    pixVec = _mm_hadd_epi32(pixVec, pixVec);
    pixVec = _mm_hadd_epi32(pixVec, pixVec);
    return (qAbs(_mm_extract_epi32(pixVec, 0)));
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::objectiveFun(QOpenGLFramebufferObject *fboResidue, QOpenGLFramebufferObject *fboU, QOpenGLFramebufferObject *fboV, float tau)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (fboU->width() != 2 * numCols || fboV->width() != 2 * numCols) {
        return (NAN);
    }
    float dataterm = innerProduct(fboResidue, fboResidue);
    float regulterm = sumScans(fboU, fboV);
    float f = 0.5 * dataterm + tau * regulterm;

    return (f);

}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::sumScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB)
{
    // CREATE A VARIABLE TO HOLD THE RETURNED VALUE
    float result = NAN;

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

         // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
        if (fboScalarA->bind()) {
            if (prgrmSumScans.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboScalarA->width(), fboScalarA->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        //txtScalarA->bind();
                        glBindTexture(GL_TEXTURE_2D, fboA->texture());
                        prgrmSumScans.setUniformValue("qt_textureA", 0);

                        glActiveTexture(GL_TEXTURE1);
                        //txtScalarB->bind();
                        glBindTexture(GL_TEXTURE_2D, fboB->texture());
                        prgrmSumScans.setUniformValue("qt_textureB", 1);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmSumScans.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmSumScans.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmSumScans.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmSumScans.release();
            }
            fboScalarA->release();
        }

        // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
        // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
        if (fboScalarB->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                        glViewport(0, 0, fboScalarB->width(), fboScalarB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboScalarA->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboScalarB->release();
        }

        // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
        // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
        if (fboScalarC->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                        glViewport(0, 0, fboScalarC->width(), fboScalarC->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboScalarB->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(fboScalarB->width(), fboScalarB->height())));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboScalarC->release();
        }

        // CREATE A VECTOR TO HOLD THE INCOMING GPU FRAME BUFFER OBJECT
        float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
        glBindTexture(GL_TEXTURE_2D, fboScalarC->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

        // COMBINE THE VEC4 INTO A SINGLE SCALAR
        result = mse[0] + mse[1] + mse[2] + mse[3];

        // RELEASE THE CURRENT OPENGL CONTEXT
        //doneCurrent();
    }
    return (result);
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::objectiveFun(LAUScan vectorResidue, LAUScan vectorU, LAUScan vectorV, float tau)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (vectorU.colors() != 8 || vectorV.colors() != 8) {
        return (NAN);
    }

    float dataterm = innerProduct(vectorResidue, vectorResidue);

    // CREATE REGULARIZATION VECTOR TO HOLD THE ACCUMULATED SUM OF REGULARIZATION TERM
    __m128 reguVecU = _mm_set1_ps(0.0f);
    __m128 reguVecV = _mm_set1_ps(0.0f);

    // ITERATE THROUGH EACH PIXEL COMPARING WITH THE MAX VECTOR AT EACH LOAD
    int index = 0;
    float *bufferA = (float *)vectorU.constPointer();
    float *bufferB = (float *)vectorV.constPointer();
    for (unsigned int row = 0; row < vectorU.height(); row++) {
        for (unsigned int col = 0; col < vectorU.width(); col++) {
            // GRAB THE VALUE
            __m128 pixUA = _mm_load_ps(bufferA + index + 0);
            __m128 pixUB = _mm_load_ps(bufferA + index + 4);

            __m128 pixVA = _mm_load_ps(bufferB + index + 0);
            __m128 pixVB = _mm_load_ps(bufferB + index + 4);

            // ADD THE NORML1 TERM
            reguVecU = _mm_add_ps(reguVecU, _mm_add_ps(pixUA, pixUB));
            reguVecV = _mm_add_ps(reguVecV, _mm_add_ps(pixVA, pixVB));

            index += 8;
        }
    }

    // EXTRACT THE FLOATS FROM THE VEC4
    float au, bu, cu, du;
    *(int *)&au = _mm_extract_ps(reguVecU, 0);
    *(int *)&bu = _mm_extract_ps(reguVecU, 1);
    *(int *)&cu = _mm_extract_ps(reguVecU, 2);
    *(int *)&du = _mm_extract_ps(reguVecU, 3);

    float av, bv, cv, dv;
    *(int *)&av = _mm_extract_ps(reguVecV, 0);
    *(int *)&bv = _mm_extract_ps(reguVecV, 1);
    *(int *)&cv = _mm_extract_ps(reguVecV, 2);
    *(int *)&dv = _mm_extract_ps(reguVecV, 3);

    float u = au + bu + cu + du ;
    float v = av + bv + cv + dv ;
    float f = 0.5 * dataterm + tau * (u + v);

    // FIND THE LARGEST SCALAR VALUE
    return (f);

}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::innerProduct(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB)
{
    // CREATE A VARIABLE TO HOLD THE RETURNED VALUE
    float result = NAN;

    if (fboA->width() == 2* numCols)  {
       QOpenGLFramebufferObject * fboScalA = fboScalarA;
       QOpenGLFramebufferObject * fboScalB = fboScalarB;
       QOpenGLFramebufferObject * fboScalC = fboScalarC;

       // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
       if (makeCurrent(surface)) {

           // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
           if (fboScalA->bind()) {
               if (prgrmInnerProduct.bind()) {
                   if (vertexBuffer.bind()) {
                       if (indexBuffer.bind()) {
                           // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                           glViewport(0, 0, fboScalA->width(), fboScalA->height());
                           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                           // BIND THE TEXTURES FOR THE FILTERING OPERATION
                           glActiveTexture(GL_TEXTURE0);
                           //txtScalarA->bind();
                           glBindTexture(GL_TEXTURE_2D, fboA->texture());
                           prgrmInnerProduct.setUniformValue("qt_textureA", 0);

                           glActiveTexture(GL_TEXTURE1);
                           //txtScalarB->bind();
                           glBindTexture(GL_TEXTURE_2D, fboB->texture());
                           prgrmInnerProduct.setUniformValue("qt_textureB", 1);

                           // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                           prgrmInnerProduct.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                           // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                           glVertexAttribPointer(prgrmInnerProduct.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                           prgrmInnerProduct.enableAttributeArray("qt_vertex");

                           glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                           // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                           indexBuffer.release();
                       }
                       vertexBuffer.release();
                   }
                   prgrmInnerProduct.release();
               }
               fboScalA->release();
           }

           // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
           // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
           if (fboScalB->bind()) {
               if (prgrmAccumSUM.bind()) {
                   if (vertexBuffer.bind()) {
                       if (indexBuffer.bind()) {
                           // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                           glViewport(0, 0, fboScalB->width(), fboScalB->height());
                           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                           // BIND THE TEXTURES FOR THE FILTERING OPERATION
                           glActiveTexture(GL_TEXTURE0);
                           glBindTexture(GL_TEXTURE_2D, fboScalA->texture());
                           prgrmAccumSUM.setUniformValue("qt_texture", 0);

                           // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                           prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                           // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                           glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                           prgrmAccumSUM.enableAttributeArray("qt_vertex");

                           glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                           // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                           indexBuffer.release();
                       }
                       vertexBuffer.release();
                   }
                   prgrmAccumSUM.release();
               }
               fboScalB->release();
           }

           // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
           // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
           if (fboScalC->bind()) {
               if (prgrmAccumSUM.bind()) {
                   if (vertexBuffer.bind()) {
                       if (indexBuffer.bind()) {
                           // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                           glViewport(0, 0, fboScalC->width(), fboScalC->height());
                           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                           // BIND THE TEXTURES FOR THE FILTERING OPERATION
                           glActiveTexture(GL_TEXTURE0);
                           glBindTexture(GL_TEXTURE_2D, fboScalB->texture());
                           prgrmAccumSUM.setUniformValue("qt_texture", 0);

                           // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                           prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(fboScalB->width(), fboScalB->height())));

                           // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                           glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                           prgrmAccumSUM.enableAttributeArray("qt_vertex");

                           glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                           // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                           indexBuffer.release();
                       }
                       vertexBuffer.release();
                   }
                   prgrmAccumSUM.release();
               }
               fboScalC->release();
           }

           // CREATE A VECTOR TO HOLD THE INCOMING GPU FRAME BUFFER OBJECT
           float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

           // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
           glBindTexture(GL_TEXTURE_2D, fboScalC->texture());
           glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

           // COMBINE THE VEC4 INTO A SINGLE SCALAR
           result = mse[0] + mse[1] + mse[2] + mse[3];
       }
    }
    else if (fboA->width() == numCols) {
       QOpenGLFramebufferObject * fboScalA = fboSpectralScalarA;
       QOpenGLFramebufferObject * fboScalB = fboSpectralScalarB;
       QOpenGLFramebufferObject * fboScalC = fboSpectralScalarC;
       // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
       if (makeCurrent(surface)) {

           // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
           if (fboScalA->bind()) {
               if (prgrmInnerProduct.bind()) {
                   if (vertexBuffer.bind()) {
                       if (indexBuffer.bind()) {
                           // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                           glViewport(0, 0, fboScalA->width(), fboScalA->height());
                           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                           // BIND THE TEXTURES FOR THE FILTERING OPERATION
                           glActiveTexture(GL_TEXTURE0);
                           //txtScalarA->bind();
                           glBindTexture(GL_TEXTURE_2D, fboA->texture());
                           prgrmInnerProduct.setUniformValue("qt_textureA", 0);

                           glActiveTexture(GL_TEXTURE1);
                           //txtScalarB->bind();
                           glBindTexture(GL_TEXTURE_2D, fboB->texture());
                           prgrmInnerProduct.setUniformValue("qt_textureB", 1);

                           // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                           prgrmInnerProduct.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                           // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                           glVertexAttribPointer(prgrmInnerProduct.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                           prgrmInnerProduct.enableAttributeArray("qt_vertex");

                           glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                           // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                           indexBuffer.release();
                       }
                       vertexBuffer.release();
                   }
                   prgrmInnerProduct.release();
               }
               fboScalA->release();
           }

           // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
           // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
           if (fboScalB->bind()) {
               if (prgrmAccumSUM.bind()) {
                   if (vertexBuffer.bind()) {
                       if (indexBuffer.bind()) {
                           // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                           glViewport(0, 0, fboScalB->width(), fboScalB->height());
                           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                           // BIND THE TEXTURES FOR THE FILTERING OPERATION
                           glActiveTexture(GL_TEXTURE0);
                           glBindTexture(GL_TEXTURE_2D, fboScalA->texture());
                           prgrmAccumSUM.setUniformValue("qt_texture", 0);

                           // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                           prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                           // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                           glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                           prgrmAccumSUM.enableAttributeArray("qt_vertex");

                           glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                           // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                           indexBuffer.release();
                       }
                       vertexBuffer.release();
                   }
                   prgrmAccumSUM.release();
               }
               fboScalB->release();
           }

           // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
           // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
           if (fboScalC->bind()) {
               if (prgrmAccumSUM.bind()) {
                   if (vertexBuffer.bind()) {
                       if (indexBuffer.bind()) {
                           // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                           glViewport(0, 0, fboScalC->width(), fboScalC->height());
                           glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                           // BIND THE TEXTURES FOR THE FILTERING OPERATION
                           glActiveTexture(GL_TEXTURE0);
                           glBindTexture(GL_TEXTURE_2D, fboScalB->texture());
                           prgrmAccumSUM.setUniformValue("qt_texture", 0);

                           // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                           prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(fboScalB->width(), fboScalB->height())));

                           // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                           glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                           prgrmAccumSUM.enableAttributeArray("qt_vertex");

                           glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                           // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                           indexBuffer.release();
                       }
                       vertexBuffer.release();
                   }
                   prgrmAccumSUM.release();
               }
               fboScalC->release();
           }

           // CREATE A VECTOR TO HOLD THE INCOMING GPU FRAME BUFFER OBJECT
           float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f};

           // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
           glBindTexture(GL_TEXTURE_2D, fboScalC->texture());
           glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (unsigned char *)mse);

           // COMBINE THE VEC4 INTO A SINGLE SCALAR
           result = mse[0] + mse[1] + mse[2] + mse[3];
       }

    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::innerProduct(LAUScan scanA, LAUScan scanB)
{
    // MAKE SURE WE HAVE TWO 8-CHANNEL IMAGES AND THAT THEY ARE BOTH THE SAME SIZE
    if (scanA.colors() != scanB.colors() || scanA.width() != scanB.width() || scanA.height() != scanB.height()) {
        return (NAN);
    }

    // CREATE SSE VECTOR TO HOLD THE ACCUMULATED SUM OF ERRORS
    __m128 sumVec = _mm_set1_ps(0.0f);

    // GRAB THE POINTERS TO THE TWO INPUT BUFFERS AND THE OUTPUT BUFFER
    float *bufferA = (float *)scanA.constPointer();
    float *bufferB = (float *)scanB.constPointer();

    // ITERATE THROUGH EACH PIXEL ADDING VECTORS FROM EACHOTHER
    for (unsigned int byt = 0; byt < scanA.length() / 4; byt += 4) {
        // GRAB THE VALUE BETWEEN THE NEXT SET OF FOUR FLOATS
        // AND STORE THE RESULTING VECTORS TO MEMORY
        sumVec = _mm_add_ps(sumVec, _mm_mul_ps(_mm_load_ps(bufferA + byt), _mm_load_ps(bufferB + byt)));
    }

    // EXTRACT THE FLOATS FROM THE VEC4
    float a, b, c, d;
    *(int *)&a = _mm_extract_ps(sumVec, 0);
    *(int *)&b = _mm_extract_ps(sumVec, 1);
    *(int *)&c = _mm_extract_ps(sumVec, 2);
    *(int *)&d = _mm_extract_ps(sumVec, 3);

    // FIND THE LARGEST SCALAR VALUE
    return (a + b + c + d);

}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::subtractScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject *fboout)
{
    if (fboA->width() == 2* numCols)  {

        //QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmSubtract.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmSubtract.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
                            //txtScalarB->bind();
                            glBindTexture(GL_TEXTURE_2D, fboB->texture());
                            prgrmSubtract.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmSubtract.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmSubtract.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmSubtract.release();
                }
                fboout->release();
            }
        }
   //      return (fboDataCubeA);
    }

    else if (fboA->width() == numCols) {

        //QOpenGLFramebufferObject *  fboSpectralModel = spectralMeasurementFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmSubtract.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmSubtract.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
                            //txtScalarB->bind();
                            glBindTexture(GL_TEXTURE_2D, fboB->texture());
                            prgrmSubtract.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmSubtract.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmSubtract.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmSubtract.release();
                }
                fboout->release();
            }
        }
       // return (fboout);
    }

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
    for (unsigned int byt = 0; byt < scanA.length() / 4; byt += 4) {
        // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
        // AND STORE THE RESULTING VECTORS TO MEMORY
        _mm_stream_ps(bufferR + byt, _mm_sub_ps(_mm_load_ps(bufferA + byt), _mm_load_ps(bufferB + byt)));
    }

    // RETURN THE RESULTING SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::addScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout)
{
    if (fboA->width() == 2* numCols)  {

        // QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmAdd.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmAdd.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
                            //txtScalarB->bind();
                            glBindTexture(GL_TEXTURE_2D, fboB->texture());
                            prgrmAdd.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmSubtract.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmAdd.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmAdd.release();
                }
                fboout->release();
            }
        }
        // return (fboDataCubeA);

    }

    else if (fboA->width() == numCols) {

     //   QOpenGLFramebufferObject *  fboSpectralModel = spectralMeasurementFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmAdd.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmAdd.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
                            //txtScalarB->bind();
                            glBindTexture(GL_TEXTURE_2D, fboB->texture());
                            prgrmAdd.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmSubtract.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmAdd.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmAdd.release();
                }
                fboout->release();
            }

        }
     //   return (fboSpectralModel);

    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::addScans(LAUScan scanA, LAUScan scanB)
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

    // ITERATE THROUGH EACH PIXEL ADDING VECTORS FROM EACHOTHER
    for (unsigned int byt = 0; byt < scanA.length() / 4; byt += 4) {
        // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
        // AND STORE THE RESULTING VECTORS TO MEMORY
        _mm_stream_ps(bufferR + byt, _mm_add_ps(_mm_load_ps(bufferA + byt), _mm_load_ps(bufferB + byt)));
    }

    // RETURN THE RESULTING SCAN
    return (result);
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::multiplyScans(float scalar, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout)
{
    if (fbo->width() == 2* numCols)  {

        //QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMultiply.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fbo->texture());
                            prgrmMultiply.setUniformValue("qt_texture", 0);
                            prgrmMultiply.setUniformValue("scalar", scalar);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmMultiply.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmMultiply.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmMultiply.release();
                }
                fboout->release();
            }
        }
        //return (fboDataCubeA);

    }

    else if (fbo->width() == numCols) {

      // QOpenGLFramebufferObject *  fboSpectralModel = spectralMeasurementFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMultiply.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fbo->texture());
                            prgrmMultiply.setUniformValue("qt_texture", 0);
                            prgrmMultiply.setUniformValue("scalar", scalar);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmMultiply.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmMultiply.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmMultiply.release();
                }
                fboout->release();
            }
        }
       // return (fboSpectralModel);

    }

}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::multiplyScans(float scalar, LAUScan scanA)
{
    // CREATE AN OUTPUT SCAN
    LAUScan result = LAUScan(scanA.width(), scanA.height(), scanA.color());

    // CREATE SSE VECTOR TO HOLD THE SCALARVEC
    __m128 scalarVec = _mm_set1_ps(scalar);

    // GRAB THE POINTERS TO THE TWO INPUT BUFFERS AND THE OUTPUT BUFFER
    float *bufferA = (float *)scanA.constPointer();
    float *bufferR = (float *)result.constPointer();

    // ITERATE THROUGH EACH PIXEL SUBSTRACTING VECTORS FROM EACHOTHER
    for (unsigned int byt = 0; byt < scanA.length() / 4; byt += 4) {
        // GRAB THE DIFFERENCE BETWEEN THE NEXT SET OF FOUR FLOATS
        // AND STORE THE RESULTING VECTORS TO MEMORY
        _mm_stream_ps(bufferR + byt, _mm_mul_ps(_mm_load_ps(bufferA + byt), scalarVec));
    }

    // RETURN THE RESULTING SCAN
    return (result);
}



/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::maxScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout)
{
    if (fboA->width() == 2* numCols)  {

        //QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMax.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmMax.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
                            //txtScalarB->bind();
                            glBindTexture(GL_TEXTURE_2D, fboB->texture());
                            prgrmMax.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmSubtract.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmMax.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmMax.release();
                }
                fboout->release();
            }
        }
       // return (fboDataCubeA);
    }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::maxScans(LAUScan scanA, LAUScan scanB)
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

    // ITERATE THROUGH EACH PIXEL ADDING VECTORS FROM EACHOTHER
    for (unsigned int byt = 0; byt < scanA.length() / 4; byt += 4) {
        // GRAB THE MAX VALUE BETWEEN THE NEXT SET OF FOUR FLOATS
        // AND STORE THE RESULTING VECTORS TO MEMORY
        _mm_stream_ps(bufferR + byt, _mm_max_ps(_mm_load_ps(bufferA + byt), _mm_load_ps(bufferB + byt)));
    }

    // RETURN THE RESULTING SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUCodedApertureGLFilter::minScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout)
{
    if (fboA->width() == 2* numCols)  {

      //  QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
            //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMin.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());
                            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            //txtScalarA->bind();
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmMin.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
                            //txtScalarB->bind();
                            glBindTexture(GL_TEXTURE_2D, fboB->texture());
                            prgrmMin.setUniformValue("qt_textureB", 1);

                            // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                            glVertexAttribPointer(prgrmSubtract.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                            prgrmMin.enableAttributeArray("qt_vertex");

                            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                            // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                            indexBuffer.release();
                        }
                        vertexBuffer.release();
                    }
                    prgrmMin.release();
                }
                fboout->release();
            }
        }
      //  return (fboDataCubeA);
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::minScans(LAUScan scanA, LAUScan scanB)
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

    // ITERATE THROUGH EACH PIXEL ADDING VECTORS FROM EACHOTHER
    for (unsigned int byt = 0; byt < scanA.length() / 4; byt += 4) {
        // GRAB THE MIN VALUE BETWEEN THE NEXT SET OF FOUR FLOATS
        // AND STORE THE RESULTING VECTORS TO MEMORY
        _mm_stream_ps(bufferR + byt, _mm_min_ps(_mm_load_ps(bufferA + byt), _mm_load_ps(bufferB + byt)));
    }

    // RETURN THE RESULTING SCAN
    return (result);
}


    /****************************************************************************/
    /****************************************************************************/
    /****************************************************************************/
void LAUCodedApertureGLFilter::copyfbo(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeA)
{

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeA->bind()) {
            if (prgrmCopyScan.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());
                        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        //txtScalarA->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmCopyScan.setUniformValue("qt_texture", 0);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmCopyScan.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmCopyScan.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmCopyScan.release();
            }
            fboDataCubeA->release();
        }

    }
    // return (fboDataCubeA);
  }


  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
void LAUCodedApertureGLFilter::createScan(float tau, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeA)
{
    //QOpenGLFramebufferObject *  fboSpectralModel = spectralMeasurementFBOs.takeFirst();
  //  QOpenGLFramebufferObject * fboDataCubeA  = dataCubeFBOs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        //csDataCube->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scan.constPointer());

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeA->bind()) {
            if (prgrmCreateScan.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());
                        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        //txtScalarA->bind();
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmCreateScan.setUniformValue("qt_texture", 0);
                        prgrmCreateScan.setUniformValue("tau", tau);

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmCreateScan.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmCreateScan.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmCreateScan.release();
            }
            fboDataCubeA->release();
        }

    }
    // return (fboDataCubeA);
  }

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUCodedApertureGLFilter::createScan(float tau, LAUScan scan)
{
    // CREATE AN OUTPUT SCAN
    LAUScan result = LAUScan(scan.width(), scan.height(), scan.color());

    // GRAB THE POINTERS TO THE OUTPUT BUFFER
    float *bufferR = (float *)result.constPointer();

    __m128 valueVec = _mm_set1_ps(tau);
    // ITERATE THROUGH EACH PIXEL ADDING VECTORS FROM EACHOTHER
    if (scan.color() == 1) {
        int index = 0;
        for (unsigned int row = 0; row < scan.height(); row++) {
            for (unsigned int col = 0; col < scan.width(); col++) {
                // FILL IN THE NEW LAUSCAN OBJECT
                // AND STORE THE RESULTING VECTORS TO MEMORY
                _mm_stream_ps(bufferR + index, valueVec);

                // UPDATE THE INDEX TO POINT TO THE NEXT PIXEL COMPOSED OF 8 FLOATS
                index += 4;
            }
        }
    } else if (scan.colors() == 4) {
        int index = 0;
        for (unsigned int row = 0; row < scan.height(); row++) {
            for (unsigned int col = 0; col < scan.width(); col++) {
                // FILL IN THE NEW LAUSCAN OBJECT
                // AND STORE THE RESULTING VECTORS TO MEMORY
                _mm_stream_ps(bufferR + index, valueVec);

                // UPDATE THE INDEX TO POINT TO THE NEXT PIXEL COMPOSED OF 8 FLOATS
                index += 4;
            }
        }
    } else if (scan.color() == 8) {
        int index = 0;
        for (unsigned int row = 0; row < scan.height(); row++) {
            for (unsigned int col = 0; col < scan.width(); col++) {
                // FILL IN THE NEW LAUSCAN OBJECT
                // AND STORE THE RESULTING VECTORS TO MEMORY
                _mm_stream_ps(bufferR + index + 0, valueVec);
                _mm_stream_ps(bufferR + index + 4, valueVec);

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
float LAUCodedApertureGLFilter::computeMSE(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB)
{
    // CREATE A VARIABLE TO HOLD THE RETURNED VALUE
    float result = NAN;

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

        // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
        if (fboScalarA->bind()) {
            if (prgrmAccumMSE.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboScalarA->width(), fboScalarA->height());
                        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        //txtScalarA->bind();
                        glBindTexture(GL_TEXTURE_2D, fboA->texture());
                        prgrmAccumMSE.setUniformValue("qt_textureA", 0);

                        glActiveTexture(GL_TEXTURE1);
                        //txtScalarB->bind();
                        glBindTexture(GL_TEXTURE_2D, fboB->texture());
                        prgrmAccumMSE.setUniformValue("qt_textureB", 1);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmAccumMSE.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumMSE.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumMSE.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumMSE.release();
            }
            fboScalarA->release();
        }

        // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
        // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
        if (fboScalarB->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                        glViewport(0, 0, fboScalarB->width(), fboScalarB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboScalarA->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboScalarB->release();
        }

        // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
        // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
        if (fboScalarC->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                        glViewport(0, 0, fboScalarC->width(), fboScalarC->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboScalarB->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(fboScalarB->width(), fboScalarB->height())));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboScalarC->release();
        }

        // CREATE A VECTOR TO HOLD THE INCOMING GPU FRAME BUFFER OBJECT
        float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
        glBindTexture(GL_TEXTURE_2D, fboScalarC->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

        // COMBINE THE VEC4 INTO A SINGLE SCALAR
        result = mse[0] + mse[1] + mse[2] + mse[3];

        // RELEASE THE CURRENT OPENGL CONTEXT
        //doneCurrent();
    }
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUCodedApertureGLFilter::computeMSE(LAUScan scanA, LAUScan scanB)
{
    // CREATE A VARIABLE TO HOLD THE RETURNED VALUE
    float result = NAN;

    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (scanA.colors() != 8 || scanB.colors() != 8) {
        return (result);
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
        // COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
        txtScalarA->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scanA.constPointer());
        txtScalarB->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, (const void *)scanB.constPointer());

        // THIS LOOP CALCULATES THE SUM OF SQUARED ERRORS FOR EACH PIXEL OF THE INPUT TEXTURES
        if (fboScalarA->bind()) {
            if (prgrmAccumMSE.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboScalarA->width(), fboScalarA->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        txtScalarA->bind();
                        prgrmAccumMSE.setUniformValue("qt_textureA", 0);

                        glActiveTexture(GL_TEXTURE1);
                        txtScalarB->bind();
                        prgrmAccumMSE.setUniformValue("qt_textureB", 1);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmAccumMSE.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumMSE.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumMSE.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumMSE.release();
            }
            fboScalarA->release();
        }

        // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
        // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
        if (fboScalarB->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                        glViewport(0, 0, fboScalarB->width(), fboScalarB->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboScalarA->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(8, 8)));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboScalarB->release();
        }

        // THIS LOOP ACCUMULATES PIXELS WITHIN 8X8 BLOCKS AND RETURNS THEIR SUM
        // IN PARTICULAR, THIS LOOP REDUCES THE 640X480 DOWN TO 80X60
        if (fboScalarC->bind()) {
            if (prgrmAccumSUM.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEWPORT TO MATCH THE SIZE OF THE FBO
                        glViewport(0, 0, fboScalarC->width(), fboScalarC->height());
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboScalarB->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // TELL THE SHADER HOW LARGE A BLOCK TO PROCESS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(fboScalarB->width(), fboScalarB->height())));

                        // TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
                        glVertexAttribPointer(prgrmAccumSUM.attributeLocation("qt_vertex"), 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
                        prgrmAccumSUM.enableAttributeArray("qt_vertex");

                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                        // RELEASE THE FRAME BUFFER OBJECT AND ITS ASSOCIATED GLSL PROGRAMS
                        indexBuffer.release();
                    }
                    vertexBuffer.release();
                }
                prgrmAccumSUM.release();
            }
            fboScalarC->release();
        }


        // CREATE A VECTOR TO HOLD THE INCOMING GPU FRAME BUFFER OBJECT
        float mse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

        // DOWNLOAD THE FBO FROM THE GPU TO THE CPU
        glBindTexture(GL_TEXTURE_2D, fboScalarC->texture());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, (unsigned char *)mse);

        // COMBINE THE VEC4 INTO A SINGLE SCALAR
        result = mse[0] + mse[1] + mse[2] + mse[3];

        // RELEASE THE CURRENT OPENGL CONTEXT
        //doneCurrent();
    }
    return (result);
}
