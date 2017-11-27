#ifndef LAUABSTRACTGPSROBJECT_H
#define LAUABSTRACTGPSROBJECT_H

#include <QObject>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObjectFormat>

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUAbstractGPSRObject : public QOpenGLContext, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum StopCriterion { SCSmallStepsInX, SCSmallStepsInObjectiveFunction, SCSmallStepsInNormOfDifference, SCSmallStepsInLCPEstimate, SCSmallObjectiveFunction, SCSmallStepsInSquareOfDifference };
    enum Initialization { InitAllZeros, InitRandom, InitInverseY };

    explicit LAUAbstractGPSRObject(unsigned int xCols, unsigned int xRows, QOpenGLFramebufferObjectFormat xFormat, unsigned int yCols, unsigned int yRows, QOpenGLFramebufferObjectFormat yFormat, QObject *parent = NULL);
    ~LAUAbstractGPSRObject();

    bool isValid() const
    {
        return (wasInitialized());
    }

    bool wasInitialized() const
    {
        return (vertexArrayObject.isCreated());
    }

    void setSurface(QSurface *srfc)
    {
        surface = srfc;
    }

    void setStopCriterion(StopCriterion criterion)
    {
        stopCriterion = criterion;
    }

    void setInitialization(Initialization init)
    {
        initialization = init;
    }

    void setDebias(bool state)
    {
        debias = state;
    }

    void setVerbose(bool state)
    {
        verbose = state;
    }

    void setMonotone(bool state)
    {
        monotone = state;
    }

    void setContinuation(bool state)
    {
        continuation = state;
    }

    void setToleranceA(float val)
    {
        tolA = val;
    }

    void setToleranceD(float val)
    {
        tolD = val;
    }

    void setFirstTau(float val)
    {
        firstTau = val;
    }

    void setAlpha(float mn, float mx)
    {
        alphaMin = mn;
        alphaMax = mx;
    }

    void setIterationsA(unsigned int mn, unsigned int mx)
    {
        minIterA = mn;
        maxIterA = mx;
    }

    void setIterationsD(unsigned int mn, unsigned int mx)
    {
        minIterD = mn;
        maxIterD = mx;
    }

    void setContinuationSteps(unsigned int val)
    {
        continuationSteps = val;
    }

    virtual void initializeGL() = 0;
    virtual void Ax(QOpenGLFramebufferObject *fboYY, QOpenGLFramebufferObject *fboXX) = 0;
    virtual void ATy(QOpenGLFramebufferObject *fboXX, QOpenGLFramebufferObject *fboYY) = 0;

protected:
    unsigned int numColsX, numRowsX, numColsY, numRowsY;
    QOpenGLFramebufferObjectFormat formatX, formatY;

    QSurface *surface;
    QOpenGLShaderProgram program;
    QOpenGLBuffer vertexBuffer, indexBuffer;
    QOpenGLVertexArrayObject vertexArrayObject;

    QList<QOpenGLFramebufferObject *> fboYs;
    QList<QOpenGLFramebufferObject *> fboXs;

    StopCriterion stopCriterion;    // type of stopping criterion to use, Default = SCSmallStepsInNormOfDifference
    Initialization initialization;  // initialization provided by the user. Default = 0;
    bool debias;                    // debiasing option: 1 = yes, 0 = no. Default = 0.
    bool verbose;                   // work silently (false) or verbosely (true)
    bool monotone;                  // enforce monotonic decrease in f, or not ?, Default = true.
    bool continuation;              // Continuation or not, Default = false;
    float tolA;                     // stopping threshold; Default = 0.01
    float tolD;                     // stopping threshold for the debiasing phase, Default = 0.0001.
    float firstTau;                 // Initial tau value, if using continuation. Default = 0.5 * max(abs(AT(y)))
    float alphaMin;                 // the alphamin parameter of the BB method. Default = 1e-30
    float alphaMax;                 // the alphamax parameter of the BB method. Default = 1e+30
    unsigned int maxIterA;          // maximum number of iterations allowed in the main phase of the algorithm. Default = 10000
    unsigned int minIterA;          // minimum number of iterations allowed in the main phase of the algorithm. Default = 5
    unsigned int maxIterD;          // maximum number of iterations allowed in the debising phase of the algorithm. Default = 200
    unsigned int minIterD;          // minimum number of iterations allowed in the debiasing phase of the algorithm. Default = 5
    unsigned int continuationSteps; // Number of steps in the continuation procedure, Default = 0
    unsigned int iter;
    float alpha;
    float lambda;
    float f;                        // object function;

    void initialize();
};

class LAUGPSRObject : public LAUAbstractGPSRObject
{
public:
    LAUGPSRObject(unsigned int xCols, unsigned int xRows, QOpenGLFramebufferObjectFormat xFormat, unsigned int yCols, unsigned int yRows, QOpenGLFramebufferObjectFormat yFormat, QObject *parent = NULL);
    ~LAUGPSRObject();

    void initializeGL();
    void Ax(QOpenGLFramebufferObject *fboXX, QOpenGLFramebufferObject *fboYY);
    void ATy(QOpenGLFramebufferObject *fboYY, QOpenGLFramebufferObject *fboXX);

    LAUScan reconstructDataCubeGPU(LAUScan ideal);
    LAUScan GPSR_GPUsolver(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fbo, LAUScan ideal);
    LAUScan firstreverseDWCTransform(QOpenGLFramebufferObject *fbo, int levels = -1);
    void firstreverseCodedAperture(LAUScan scan, QOpenGLFramebufferObject *fboout);
    void firstforwardDWCTransform(LAUScan scan, QOpenGLFramebufferObject *fboDataCubeB, int levels = -1);
    void forwardDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout, int levels = -1);
    void reverseDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout, int levels = -1);
    void forwardCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);
    void reverseCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);

protected:
    QOpenGLTexture *dataCube;
    QOpenGLTexture *txtScalarA, *txtScalarB, *txtCodeAper;

    QOpenGLFramebufferObject *fboScalarA, *fboScalarB, *fboScalarC, *fboSpectralScalarA, *fboSpectralScalarB, *fboSpectralScalarC;
    QOpenGLFramebufferObject *fboCodeAperLeft, *fboCodeAperRight;

    QOpenGLShaderProgram prgrmForwardDWTx, prgrmForwardDWTy;
    QOpenGLShaderProgram prgrmForwardDCT, prgrmReverseDCT;
    QOpenGLShaderProgram prgrmReverseDWTx, prgrmReverseDWTy;
    QOpenGLShaderProgram prgrmForwardCodedAperture, prgrmReverseCodedAperture;
    QOpenGLShaderProgram prgrmU, prgrmV, prgrmAccumMSE, prgrmAccumNZE, prgrmAccumSUM, prgrmAccumMAX, prgrmAccumMIN;
    QOpenGLShaderProgram prgrmSubtract, prgrmAdd, prgrmInnerProduct, prgrmSumScans, prgrmCreateScan, prgrmMultiply, prgrmMax, prgrmMin, prgrmAbsMax, prgrmCopyScan;

    static float LoD[16], HiD[16], LoR[16], HiR[16];

    void initializeParameters();
    void initializeShaders();
    void initializeTextures();
    void initializeVertices();

    void setCodedAperture(QImage image);
    void computeVectorU(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);
    void computeVectorV(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);
    void subtractScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject *fboout);
    void addScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject *fboout);
    void multiplyScans(float scalar, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);
    void createScan(float tau, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);
    void maxScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject *fboout);
    void minScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject *fboout);
    void copyfbo(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject *fboout);
    float computeMSE(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB);
    float innerProduct(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB);
    float sumScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB);
    float objectiveFun(QOpenGLFramebufferObject *fboResidue, QOpenGLFramebufferObject *fboU, QOpenGLFramebufferObject *fboV, float tau);
    float maxAbsValue(QOpenGLFramebufferObject *fbo);
    float sumAbsValue(QOpenGLFramebufferObject *fbo);
    int nonZeroElements(QOpenGLFramebufferObject *fbo);
};

#endif // LAUABSTRACTGPSROBJECT_H
