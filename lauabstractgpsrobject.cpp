#include "lauabstractgpsrobject.h"
#include "locale.h"

float LAUGPSRObject::LoD[16] = {  -0.00338242,  -0.00054213,   0.03169509,   0.00760749,  -0.14329424,  -0.06127336,   0.48135965,   0.77718575,   0.36444189,  -0.05194584,  -0.02721903,   0.04913718,   0.00380875,  -0.01495226,  -0.00030292,   0.00188995 };
float LAUGPSRObject::HiD[16] = {  -0.00188995,  -0.00030292,   0.01495226,   0.00380875,  -0.04913718,  -0.02721903,   0.05194584,   0.36444189,  -0.77718575,   0.48135965,   0.06127336,  -0.14329424,  -0.00760749,   0.03169509,   0.00054213,  -0.00338242 };
float LAUGPSRObject::LoR[16] = {   0.00188995,  -0.00030292,  -0.01495226,   0.00380875,   0.04913718,  -0.02721903,  -0.05194584,   0.36444189,   0.77718575,   0.48135965,  -0.06127336,  -0.14329424,   0.00760749,   0.03169509,  -0.00054213,  -0.00338242 };
float LAUGPSRObject::HiR[16] = {  -0.00338242,   0.00054213,   0.03169509,  -0.00760749,  -0.14329424,   0.06127336,   0.48135965,  -0.77718575,   0.36444189,   0.05194584,  -0.02721903,  -0.04913718,   0.00380875,   0.01495226,  -0.00030292,  -0.00188995 };


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUAbstractGPSRObject::LAUAbstractGPSRObject(unsigned int xCols, unsigned int xRows, QOpenGLFramebufferObjectFormat xFormat, unsigned int yCols, unsigned int yRows, QOpenGLFramebufferObjectFormat yFormat, QObject *parent) : QOpenGLContext(parent), numColsX(xCols), numRowsX(xRows), numColsY(yCols), numRowsY(yRows), formatX(xFormat), formatY(yFormat)
{
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
LAUAbstractGPSRObject::~LAUAbstractGPSRObject()
{
    if (surface && makeCurrent(surface)) {
        while (fboXs.isEmpty() == false) {
            delete fboXs.takeFirst();
        }
        while (fboYs.isEmpty() == false) {
            delete fboYs.takeFirst();
        }
        doneCurrent();
        delete surface;
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUAbstractGPSRObject::initialize()
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

    // CREATE A LIST OF FBOS FOR USE AS NEEDED SO THAT ANYTHING IN THE LIST
    // IS AVAILABLE FOR USE WHILE ANYTHING NOT IN THE LIST IS BEING USED
    for (int n = 0; n < 15; n++) {
        QOpenGLFramebufferObject *fboXX = new QOpenGLFramebufferObject(numColsX, numRowsX, formatX);
        fboXX->release();
        fboXs << fboXX;

        QOpenGLFramebufferObject *fboYY = new QOpenGLFramebufferObject(numColsY, numRowsY, formatY);
        fboYY->release();
        fboYs << fboYY;
    }

    // CREATE GLSL PROGRAM FOR PROCESSING THE INCOMING VIDEO
    setlocale(LC_NUMERIC, "C");
    program.addShaderFromSourceFile(QOpenGLShader::Vertex,   "PUT_YOUR_SHADER_HERE.vert");
    program.addShaderFromSourceFile(QOpenGLShader::Fragment, "PUT_YOUR_SHADER_HERE.frag");
    program.link();
    setlocale(LC_ALL, "");

    // NOW GIVE THE SUBCLASS A CHANCE TO INITIALIZE ITSELF
    initializeGL();
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUGPSRObject::LAUGPSRObject(unsigned int xCols, unsigned int xRows, QOpenGLFramebufferObjectFormat xFormat, unsigned int yCols, unsigned int yRows, QOpenGLFramebufferObjectFormat yFormat, QObject *parent) : QOpenGLContext(parent), numColsX(xCols), numRowsX(xRows), numColsY(yCols), numRowsY(yRows), formatX(xFormat), formatY(yFormat)
{
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
LAUGPSRObject::~LAUGPSRObject()
{
    if (surface && makeCurrent(surface)) {
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
        if (txtCodeAper) {
            delete txtCodeAper;
        }
        if (fboCodeAperLeft) {
            delete fboCodeAperLeft;
        }
        if (fboCodeAperRight) {
            delete fboCodeAperRight;
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
void LAUGPSRObject::initializeGL()
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
	
	setCodedAperture(QImage(":/Images/Images/CASSIMask.bmp"));
	}	
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::initializeVertices()
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
void LAUGPSRObject::initializeShaders()
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
void LAUGPSRObject::initializeTextures()
{
    // CREATE THE GPU SIDE TEXTURE TO HOLD THE 3D DATA CUBE
    dataCube = new QOpenGLTexture(QOpenGLTexture::Target2D);
    dataCube->setSize(numColsX, numRowsX);
    dataCube->setFormat(QOpenGLTexture::RGBA32F);
    dataCube->setWrapMode(QOpenGLTexture::ClampToBorder);
    dataCube->setMinificationFilter(QOpenGLTexture::Nearest);
    dataCube->setMagnificationFilter(QOpenGLTexture::Nearest);
    dataCube->allocateStorage();

    // CREATE GPU SIDE TEXTURES FOR SCALAR FILTERING OPERATIONS
    txtScalarA = new QOpenGLTexture(QOpenGLTexture::Target2D);
    txtScalarA->setSize(numColsX, numRowsX);
    txtScalarA->setFormat(QOpenGLTexture::RGBA32F);
    txtScalarA->allocateStorage();

    txtScalarB = new QOpenGLTexture(QOpenGLTexture::Target2D);
    txtScalarB->setSize(numColsX, numRowsX);
    txtScalarB->setFormat(QOpenGLTexture::RGBA32F);
    txtScalarB->allocateStorage();

    // CREATE THE INTERMEDIATE FBOS FOR THE FORWARD AND INVERSE TRANSFORMS
    fboCodeAperLeft = new QOpenGLFramebufferObject(numColsX, numRowsX, formatX);
    fboCodeAperLeft->release();

    fboCodeAperRight = new QOpenGLFramebufferObject(numColsX, numRowsX, formatX);
    fboCodeAperRight->release();

    // CREATE FRAME BUFFER OBJECTS FOR ACCUMULATIVE SUM OPERATIONS BY GENERATING
    // TWO LAYERS OF BUFFERS THAT REDUCE THE SIZE OF THE TARGET BUFFER BY FACTORS
    // OF EIGTH IN THE HEIGHT AND WIDTH DIMENSIONS
    int sWidth = (int)qCeil((float)(numColsX / 8.0f));
    int sHeight = (int)qCeil((float)(numRowsX / 8.0f));

    fboScalarA = new QOpenGLFramebufferObject(sWidth, sHeight, formatX);
    fboScalarA->release();

    sWidth = (int)qCeil((float)(sWidth / 8.0f));
    sHeight = (int)qCeil((float)(sHeight / 8.0f));

    fboScalarB = new QOpenGLFramebufferObject(sWidth, sHeight, formatX);
    fboScalarB->release();

    fboScalarC = new QOpenGLFramebufferObject(1, 1, formatX);
    fboScalarC->release();

    // CREATE THE FINAL FBO FOR HOLDING THE MONOCHROME OUTPUT
	QOpenGLFramebufferObject * fboYY = fboYs.takeFirst();

    sWidth = (int)qCeil((float)(fboYY->width() / 8.0f));
    sHeight = (int)qCeil((float)(fboYY->height() / 8.0f));
	
	fboYs.append(fboYY);

    fboSpectralScalarA = new QOpenGLFramebufferObject(sWidth, sHeight, formatY);
    fboSpectralScalarA->release();

    sWidth = (int)qCeil((float)(sWidth / 8.0f));
    sHeight = (int)qCeil((float)(sHeight / 8.0f));

    fboSpectralScalarB = new QOpenGLFramebufferObject(sWidth, sHeight, formatY);
    fboSpectralScalarB->release();

    fboSpectralScalarC = new QOpenGLFramebufferObject(1, 1, formatY);
    fboSpectralScalarC->release();

}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::initializeParameters()
{
    dataCube = NULL;
    txtScalarA = NULL;
    txtScalarB = NULL;
    fboScalarA = NULL;
    fboScalarB = NULL;
    fboScalarC = NULL;
    txtCodeAper = NULL;
	fboSpectralScalarA = NULL;
	fboSpectralScalarB = NULL;
	fboSpectralScalarC = NULL;
    fboCodeAperLeft = NULL;
    fboCodeAperRight = NULL;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::setCodedAperture(QImage image)
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
void LAUGPSRObject:: Ax(QOpenGLFramebufferObject *fboXX, QOpenGLFramebufferObject *fboYY)
{
	QOpenGLFramebufferObject * fbooutdwt  = fboXs.takeFirst();
    reverseDWCTransform(fboXX, fbooutdwt);
    reverseCodedAperture(fbooutdwt, fboYY);
    fboXs.append(fbooutdwt);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject:: ATy(QOpenGLFramebufferObject *fboYY, QOpenGLFramebufferObject *fboXX)
{
	QOpenGLFramebufferObject *  fbooutcodedapt = fboXs.takeFirst();
    forwardCodedAperture(fboYY, fbooutcodedapt);
    forwardDWCTransform(fbooutcodedapt, fboXX);
	fboXs.append(fbooutcodedapt);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUGPSRObject::reconstructDataCubeGPU(LAUScan ideal)
{
    // THE INCOMING SCAN IS THE COMPLETE, IDEAL, PERFECT 3D DATA CUBE
    // WE WANT TO GENERATE A CODED APERTURE ENCODING AND THEN RECONSTRUCT THIS SCAN


    // SO LET'S START BY GENERATING OUR CODED APERTURE ENCODING
    QOpenGLFramebufferObject * fboY = fboYs.takeFirst();
    firstreverseCodedAperture(ideal, fboY);

    QOpenGLFramebufferObject * fboW = fboXs.takeFirst();
    forwardCodedAperture(fboY, fboW);

    QOpenGLFramebufferObject * fboZ = fboYs.takeFirst();
    reverseCodedAperture(fboW, fboZ);

    // GET THE GROUND TRUTH X
    QOpenGLFramebufferObject *  fbogrtruth = fboXs.takeFirst();
    firstforwardDWCTransform(ideal, fbogrtruth);


    LAUScan result = GPSR_GPUsolver(fboY, fbogrtruth, ideal);


    fboYs.append(fboY);
    fboYs.append(fboZ);
    fboXs.append(fboW);
    fboXs.append(fbogrtruth);

    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUGPSRObject::GPSR_GPUsolver(QOpenGLFramebufferObject *fboY, QOpenGLFramebufferObject * fbogrtruth, LAUScan ideal)
//LAUScan LAUoptimization::GPSRGPU(QOpenGLFramebufferObject *fboY, QOpenGLFramebufferObject * fbogrtruth)
{

    LAUScan result;
    // INITIALIZE VARIABLES FOR MANAGING GPSR ALGORITHM
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

    // NOW CALCULATE THE INITIAL ESTIMATE (LINE 290 OF GPSR_BB SCRIPT)
    QOpenGLFramebufferObject *  fboXi = fboXs.takeFirst();
	ATy(fboY, fboXi);

    QOpenGLFramebufferObject *  fboX = fboXs.takeFirst();
	ATy(fboY, fboX);

    // CALL METHOD FOR CALCULATING THE INITIAL TAU PARAMETER ACCORDING TO  0.5 * max(abs(AT(y)))
    firstTau = 0.1415;

    // INITIALIZE U AND V VECTORS (LINES 345 AND 346 OF GPSR_BB SCRIPT)
    QOpenGLFramebufferObject *  fboU = fboXs.takeFirst();
    computeVectorU(fboXi, fboU);

    QOpenGLFramebufferObject *  fboV = fboXs.takeFirst();
    computeVectorV(fboXi, fboV);

    // GET THE NUMBER OF NON-ZERO ELEMENTS IN X (LINE 350 OF GPSR_BB SCRIPT)
    int nonZeroCount = nonZeroElements(fboXi);

    // CALCULATE RESIDUE (LINE 402 OF GPSR_BB SCRIPT)
    QOpenGLFramebufferObject *  fboAofX = fboYs.takeFirst();
	Ax(fboXi, fboAofX);

    QOpenGLFramebufferObject *  fboResidue = fboYs.takeFirst();
    subtractScans(fboY, fboAofX, fboResidue);
    fboYs.append(fboAofX);

    iter = 1;
    alpha = 1;

    //COMPUTE INITIAL VALUE OF THE OBJECTIVE FUNCTION (LINE 438 OF GPSR_BB SCRIPT)
    float f = objectiveFun(fboResidue, fboU, fboV, firstTau);
    float mse = computeMSE(fbogrtruth, fboXi);

    if (verbose) {
        qDebug() << "Setting firstTau =" << firstTau;
        qDebug() << "Initial MSE =" << mse;
        qDebug() << "Initial obj =" << f << ", nonzeros =" << nonZeroCount;
    }

    // COMPUTE THE INITIAL GRADIENT AND THE USEFUL QUANTITY RESID_BASE (LINE 452 OF GPSR_BB SCRIPT)
    QOpenGLFramebufferObject *  fboResidueBase = fboYs.takeFirst();
    subtractScans(fboY, fboResidue, fboResidueBase);

    //CONTROL VARIABLE FOR THE OUTER LOOP AND ITER COUNTER
    int keep_going = 1;

    //(LINE 461 OF GPSR_BB SCRIPT)
    while (keep_going) {
        // CALCULATE THE GRADIENT BASED ON THE FORWARD TRANSFORM OF THE RESIDUE_BASE(LINE 464 OF GPSR_BB SCRIPT)
        QOpenGLFramebufferObject *  fboGradient = fboXs.takeFirst();
	    ATy(fboResidueBase, fboGradient);

        QOpenGLFramebufferObject *  fbotau = fboXs.takeFirst();
        createScan(firstTau, fboGradient, fbotau);
        QOpenGLFramebufferObject *  fboterm = fboXs.takeFirst();
        subtractScans(fboGradient, fboX, fboterm);
        fboXs.append(fboGradient);

        QOpenGLFramebufferObject *  fbogradu = fboXs.takeFirst();
        addScans(fboterm, fbotau, fbogradu);
        QOpenGLFramebufferObject *  fbogradv = fboXs.takeFirst();
        subtractScans(fbotau, fboterm, fbogradv);

        fboXs.append(fbotau);
        fboXs.append(fboterm);

        //PROJECTION AND COMPUTTATION OF SEARCH DIRECTION VECTOR(LINE 471 OF GPSR_BB SCRIPT)
        QOpenGLFramebufferObject *  fbodu = fboXs.takeFirst();
        QOpenGLFramebufferObject *  fbomul = fboXs.takeFirst();
        QOpenGLFramebufferObject *  fbosub = fboXs.takeFirst();
        QOpenGLFramebufferObject *  fbocrt = fboXs.takeFirst();
        QOpenGLFramebufferObject *  fbomax = fboXs.takeFirst();
        multiplyScans(alpha, fbogradu, fbomul);
        subtractScans(fboU, fbomul, fbosub);
        createScan(0, fbogradu, fbocrt);
        maxScans(fbosub, fbocrt, fbomax);
        subtractScans(fbomax, fboU, fbodu);

        QOpenGLFramebufferObject *  fbodv = fboXs.takeFirst();
        multiplyScans(alpha, fbogradv, fbomul);
        subtractScans(fboV, fbomul, fbosub);
        createScan(0, fbogradv, fbocrt);
        maxScans(fbosub, fbocrt, fbomax);
        subtractScans(fbomax, fboV, fbodv);

        fboXs.append(fbomul);
        fboXs.append(fbosub);
        fboXs.append(fbocrt);
        fboXs.append(fbomax);

        QOpenGLFramebufferObject *  fbodx = fboXs.takeFirst();
        subtractScans(fbodu, fbodv, fbodx);

        QOpenGLFramebufferObject *  fboold_u = fboXs.takeFirst();
        copyfbo(fboU, fboold_u);

        QOpenGLFramebufferObject *  fboold_v = fboXs.takeFirst();
        copyfbo(fboV, fboold_v);

        //CALCULATE USEFUL MATRIX-VECTOR PRODUCT INVOLVING dx (LINE 478 OF GPSR_BB SCRIPT)
        QOpenGLFramebufferObject *  fboauv = fboYs.takeFirst();
		Ax(fbodx, fboauv);

        float dGd = innerProduct(fboauv, fboauv);

        if (monotone == true) {
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
        QOpenGLFramebufferObject * fbomul1  = fboXs.takeFirst();
        multiplyScans(lambda, fbodu, fbomul1);
        addScans(fboold_u, fbomul1, fboU);

        multiplyScans(lambda, fbodv, fbomul1);
        addScans(fboold_v, fbomul1, fboV);
        fboXs.append(fbomul1);

        QOpenGLFramebufferObject *  fboUVmin = fboXs.takeFirst();
        minScans(fboU, fboV, fboUVmin);
        subtractScans(fboU, fboUVmin, fboU);
        subtractScans(fboV, fboUVmin, fboV);

        fboXs.append(fboUVmin);
        subtractScans(fboU, fboV, fboXi);

        //CALCULATE NONZERO PATTERN AND NUMBER OF NONZEROS(LINE 502 OF GPSR_BB SCRIPT)
        int prev_nonZeroCount = nonZeroCount;
        int nonZeroCount = nonZeroElements(fboXi);

        //UPDATE RESIDUAL AND FUNCTION(LINE 507 OF GPSR_BB SCRIPT)
        QOpenGLFramebufferObject *  fbosub1 = fboYs.takeFirst();
        QOpenGLFramebufferObject *  fbomul2 = fboYs.takeFirst();
        subtractScans(fboY, fboResidueBase, fbosub1);
        multiplyScans(lambda, fboauv, fbomul2);
        subtractScans(fbosub1, fbomul2, fboResidue);
        fboYs.append(fbosub1);
        fboYs.append(fbomul2);

        float prev_f = f;
        f = objectiveFun(fboResidue, fboU, fboV, firstTau);

        // COMPUTER NEW ALPHA(LINE 513 OF GPSR_BB SCRIPT)
        float dd = innerProduct(fbodu, fbodu) + innerProduct(fbodv, fbodv);

        fboXs.append(fbodu);
        fboXs.append(fbodv);

        if (dGd <= 0) {
            qDebug() << "nonpositive curvature detected dGd = " << dGd;
            alpha = alphaMax;
        } else {
            alpha = qMin(alphaMax, qMax(alphaMin, dd / dGd));
        }

        QOpenGLFramebufferObject *  fbomul3 = fboYs.takeFirst();
        multiplyScans(lambda, fboauv, fbomul3);
        addScans(fboResidueBase, fbomul3, fboResidueBase);

        fboYs.append(fbomul3);
        fboYs.append(fboauv);

        if (verbose) {
            qDebug() << "Iter = " << iter << ", obj = " << f << ", lambda = " << lambda << ", alpha = " << alpha << ", nonezeros = " << nonZeroCount << ", MSE= " << mse;
        }

        // UPDATE ITERATION COUNTS (LINE 530 OF GPSR_BB SCRIPT)
        iter = iter + 1;
        mse = computeMSE(fbogrtruth, fboXi);

        // FINAL RECONSTRUCTED SNAPSHOT ON CASSI BY SOLVED X
        result = firstreverseDWCTransform(fboXi);

        float psnr = 10 * log10(1 * 1 * numColsY * numRowsY * 8/computeMSE(result, ideal));
        qDebug() << "psnr = " << psnr ;

        if (stopCriterion != 2){
            fboXs.append(fbodx);
        }

        if (stopCriterion != 3){
            fboXs.append(fboold_u);
            fboXs.append(fboold_v);
            fboXs.append(fbogradu);
            fboXs.append(fbogradv);
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
                fboXs.append(fbodx);
                break;
            }
            // CRITERION BASED ON "LCP" - AGAIN BASED ON THE PREVIOUS ITERATE. MAKE IT RELATIVE TO THE NORM OF X
            case 3: {
               // float CriterionLCP = qMax(maxAbsValue(minScans(fbogradu, fboold_u)), maxAbsValue(minScans(fbogradv, fboold_v)));
                QOpenGLFramebufferObject *  fbominu = fboXs.takeFirst();
                QOpenGLFramebufferObject *  fbominv = fboXs.takeFirst();
                minScans(fbogradu, fboold_u, fbominu);
                minScans(fbogradv, fboold_v, fbominv);
                float CriterionLCP = qMax(maxAbsValue(fbominu), maxAbsValue(fbominv));

                CriterionLCP = CriterionLCP / qMax(1e-6f, qMax(maxAbsValue(fboold_u), maxAbsValue(fboold_v)));
                keep_going = (CriterionLCP > tolA);
                if (verbose) {
                    qDebug() << "LCP = " << CriterionLCP << "target = " << tolA;
                }
                fboXs.append(fboold_u);
                fboXs.append(fboold_v);
                fboXs.append(fbogradu);
                fboXs.append(fbogradv);
                fboXs.append(fbominu);
                fboXs.append(fbominv);

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

    fboYs.append(fboResidue);
    fboYs.append(fboResidueBase);
    fboXs.append(fboU);
    fboXs.append(fboV);
    fboXs.append(fboXi);
    fboXs.append(fboX);

    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void  LAUGPSRObject::firstforwardDWCTransform(LAUScan scan, QOpenGLFramebufferObject *fboDataCubeB, int levels)
{
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

    QOpenGLFramebufferObject * fboDataCubeA  = fboXs.takeFirst();
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
   fboXs.append(fboDataCubeA);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void  LAUGPSRObject::forwardDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeB, int levels)
{

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

    QOpenGLFramebufferObject * fboDataCubeA  = fboXs.takeFirst();
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
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
    fboXs.append(fboDataCubeA);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
LAUScan LAUGPSRObject::firstreverseDWCTransform(QOpenGLFramebufferObject *fbo, int levels)
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

    QOpenGLFramebufferObject * fboDataCubeA  = fboXs.takeFirst();
    QOpenGLFramebufferObject * fboDataCubeB  = fboXs.takeFirst();

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

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
    }

    fboXs.append(fboDataCubeA);
    fboXs.append(fboDataCubeB);
    // RETURN THE UPDATED SCAN
    return (result);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::reverseDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeB, int levels)
{
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

    QOpenGLFramebufferObject * fboDataCubeA  = fboXs.takeFirst();
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
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
    }
    fboXs.append(fboDataCubeA);
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::forwardCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboDataCubeA)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
     if (makeCurrent(surface)) {
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
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::firstreverseCodedAperture(LAUScan scan,  QOpenGLFramebufferObject * fboSpectralModel)
{
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
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::reverseCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboSpectralModel)
  {
      // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
      if (fbo->width() == numColsX) {
          if (makeCurrent(surface)) {
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
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::computeVectorU(QOpenGLFramebufferObject *fbo,  QOpenGLFramebufferObject * fboDataCubeA)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (fbo->width() != numColsX)  {
        qDebug()<<"someting wrong with U";
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
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
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::computeVectorV(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboDataCubeB)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (fbo->width() != numColsX)  {
        qDebug()<<"someting wrong with V";
    }

    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {
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
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUGPSRObject::maxAbsValue(QOpenGLFramebufferObject *fbo)
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

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmAbsMax.setUniformValue("qt_texture", 0);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmAbsMax.setUniformValue("qt_blockSize", QPointF(QPoint(numColsX, numRowsX)));

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
float LAUGPSRObject::sumAbsValue(QOpenGLFramebufferObject *fbo)
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

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fbo->texture());
                        prgrmAccumSUM.setUniformValue("qt_texture", 0);

                        // SET THE BLOCK SIZE FOR ACCUMULATED SUMS
                        prgrmAccumSUM.setUniformValue("qt_blockSize", QPointF(QPoint(numColsX, numRowsX)));

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
 int LAUGPSRObject::nonZeroElements(QOpenGLFramebufferObject *fbo)
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

                         // BIND THE TEXTURES FOR THE FILTERING OPERATION
                         glActiveTexture(GL_TEXTURE0);
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
     }
     return (result);
 }


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUGPSRObject::objectiveFun(QOpenGLFramebufferObject *fboResidue, QOpenGLFramebufferObject *fboU, QOpenGLFramebufferObject *fboV, float tau)
{
    // MAKE SURE WE HAVE AN 8-CHANNEL IMAGE
    if (fboU->width() != numColsX || fboV->width() != numColsX) {
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
float LAUGPSRObject::sumScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB)
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
    }
    return (result);
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUGPSRObject::innerProduct(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB)
{
    // CREATE A VARIABLE TO HOLD THE RETURNED VALUE
    float result = NAN;

    if (fboA->width() == numColsX)  {
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
    else if (fboA->width() == numColsY) {
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
                           glBindTexture(GL_TEXTURE_2D, fboA->texture());
                           prgrmInnerProduct.setUniformValue("qt_textureA", 0);

                           glActiveTexture(GL_TEXTURE1);
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
void LAUGPSRObject::subtractScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject *fboout)
{
    if (fboA->width() == numColsX)  {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmSubtract.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmSubtract.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
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
    }

    else if (fboA->width() == numColsY) {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmSubtract.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmSubtract.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
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
    }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::addScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout)
{
    if (fboA->width() == numColsX)  {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmAdd.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmAdd.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
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
    }

    else if (fboA->width() == numColsY) {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmAdd.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmAdd.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
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
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::multiplyScans(float scalar, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout)
{
    if (fbo->width() == numColsX)  {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMultiply.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
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
    }

    else if (fbo->width() == numColsY) {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {
            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMultiply.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
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
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::maxScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout)
{
    if (fboA->width() == numColsX)  {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMax.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmMax.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
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
    }
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::minScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout)
{
    if (fboA->width() == numColsX)  {

        // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
        if (makeCurrent(surface)) {

            // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
            // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
            if (fboout->bind()) {
                if (prgrmMin.bind()) {
                    if (vertexBuffer.bind()) {
                        if (indexBuffer.bind()) {
                            // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                            glViewport(0, 0, fboout->width(), fboout->height());

                            // BIND THE TEXTURES FOR THE FILTERING OPERATION
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, fboA->texture());
                            prgrmMin.setUniformValue("qt_textureA", 0);

                            glActiveTexture(GL_TEXTURE1);
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
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::copyfbo(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeA)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeA->bind()) {
            if (prgrmCopyScan.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
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
  }


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
void LAUGPSRObject::createScan(float tau, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboDataCubeA)
{
    // MAKE SURE WE HAVE AN INPUT SCAN WITH EIGHT CHANNELS
    if (makeCurrent(surface)) {

        // BIND THE FRAME BUFFER OBJECT FOR PROCESSING ALONG WITH
        // THE SHADER AND VBOS FOR DRAWING TRIANGLES ON SCREEN
        if (fboDataCubeA->bind()) {
            if (prgrmCreateScan.bind()) {
                if (vertexBuffer.bind()) {
                    if (indexBuffer.bind()) {
                        // SET THE VIEW PORT TO THE LEFT-HALF OF THE IMAGE FOR LOW-PASS FILTERING
                        glViewport(0, 0, fboDataCubeA->width(), fboDataCubeA->height());

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
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
}


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
float LAUGPSRObject::computeMSE(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB)
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

                        // BIND THE TEXTURES FOR THE FILTERING OPERATION
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, fboA->texture());
                        prgrmAccumMSE.setUniformValue("qt_textureA", 0);

                        glActiveTexture(GL_TEXTURE1);
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
    }
    return (result);
}





