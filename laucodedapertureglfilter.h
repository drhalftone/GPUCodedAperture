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

#ifndef LAUCODEDAPERTUREGLFILTER_H
#define LAUCODEDAPERTUREGLFILTER_H

#include <QtCore>
#include <QScreen>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLVertexArrayObject>

#include "lauscan.h"

#ifdef Q_OS_WIN
#define save_dir "C:/Users/yuzhang/Documents/MATLAB/"
#define msDataSet_dir "C:/Users/yuzhang/Documents/GPUCodedAperture/Images/msDataSet.tif"
#elif defined(Q_OS_MAC)
#define save_dir "/Users/dllau/SourceTree/LAUCodedAperture/Matlab/"
#define msDataSet_dir "/Users/dllau/SourceTree/LAUCodedAperture/Matlab/msDataSet.tif"
#endif

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCodedApertureGLFilter : public QOpenGLContext, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum StopCriterion { SCSmallStepsInX, SCSmallStepsInObjectiveFunction, SCSmallStepsInNormOfDifference, SCSmallStepsInLCPEstimate, SCSmallObjectiveFunction, SCSmallStepsInSquareOfDifference };
    enum Initialization { InitAllZeros, InitRandom, InitInverseY };

    explicit LAUCodedApertureGLFilter(unsigned int cols, unsigned int rows, LAUVideoPlaybackColor color, QWidget *parent = NULL);
    explicit LAUCodedApertureGLFilter(LAUScan scan, QWidget *parent = NULL);
    ~LAUCodedApertureGLFilter();

    bool isValid() const
    {
        return (wasInitialized());
    }

    bool wasInitialized() const
    {
        return (vertexArrayObject.isCreated());
    }

    int width() const
    {
        return (numCols);
    }

    int height() const
    {
        return (numRows);
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

    void initialize();
    void setCodedAperture(QImage image);

    LAUScan reconstructDataCube(LAUScan ideal);                 // DERIVE THE 3D DATACUBE FROM THE MONOCHROME IMAGE
    LAUScan forwardDWCTransform(LAUScan scan, int levels = -1); // DERIVE THE FORWARD DWT+DCT TRANSFORM OF THE 3D DATACUBE
    LAUScan reverseDWCTransform(LAUScan scan, int levels = -1); // DERIVE THE INVERSE DWT+DCT TRANSFORM OF THE 3D DATACUBE
    LAUScan forwardCodedAperture(LAUScan scan);                 // GENERATE THE MONOCHROME IMAGE FROM THE 3D DATACUBE USING THE CODED APERTURE
    LAUScan reverseCodedAperture(LAUScan scan);                 // GENERATE THE 3D DATACUBE FROM THE MONOCHROME IMAGE USING THE CODED APERTURE

    LAUScan forwardTransform(LAUScan scan)
    {
        return (forwardDWCTransform(forwardCodedAperture(scan)));
    }

    LAUScan reverseTransform(LAUScan scan)
    {
        return (reverseCodedAperture(reverseDWCTransform(scan)));
    }

    //GPU VERSION
    LAUScan reconstructDataCubeGPU(LAUScan ideal);
    LAUScan firstreverseDWCTransform(QOpenGLFramebufferObject *fbo, int levels = -1);
    void firstreverseCodedAperture(LAUScan scan, QOpenGLFramebufferObject * fboout);
    void firstforwardDWCTransform(LAUScan scan, QOpenGLFramebufferObject * fboDataCubeB, int levels = -1);
    void forwardDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout, int levels = -1);
    void reverseDWCTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout, int levels = -1);
    void forwardCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);
    void reverseCodedAperture(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);

//    void forwardTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fbooutdwt)
//    {
//        forwardCodedAperture( fbo, fbooutcodedapt);
//        forwardDWCTransform(fbooutcodedapt, fbooutdwt);
//    }

//    void reverseTransform(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout)
//    {
//        return (reverseCodedAperture(reverseDWCTransform( fbo, fboout)));
//    }

private:
    unsigned int numCols, numRows;

    LAUVideoPlaybackColor playbackColor;

    QSurface *surface;
    QOpenGLBuffer vertexBuffer, indexBuffer;
    QOpenGLVertexArrayObject vertexArrayObject;

    QOpenGLTexture *dataCube, *csDataCube, *spectralMeasurement;
    QOpenGLTexture *txtScalarA, *txtScalarB, *txtCodeAper;

    QOpenGLFramebufferObject *fboScalarA, *fboScalarB, *fboScalarC, *fboSpectralScalarA, *fboSpectralScalarB, *fboSpectralScalarC;
    QOpenGLFramebufferObject *fboDataCubeA, *fboDataCubeB;
    QOpenGLFramebufferObject *fboCodeAperLeft, *fboCodeAperRight, *fboSpectralModel;

    QOpenGLShaderProgram prgrmForwardDWTx, prgrmForwardDWTy;
    QOpenGLShaderProgram prgrmForwardDCT, prgrmReverseDCT;
    QOpenGLShaderProgram prgrmReverseDWTx, prgrmReverseDWTy;
    QOpenGLShaderProgram prgrmForwardCodedAperture, prgrmReverseCodedAperture;
    QOpenGLShaderProgram prgrmU, prgrmV, prgrmAccumMSE, prgrmAccumNZE, prgrmAccumSUM, prgrmAccumMAX, prgrmAccumMIN;
    QOpenGLShaderProgram prgrmSubtract, prgrmAdd, prgrmInnerProduct, prgrmSumScans, prgrmCreateScan, prgrmMultiply, prgrmMax, prgrmMin, prgrmAbsMax, prgrmCopyScan;

    QList<QOpenGLFramebufferObject *> spectralMeasurementFBOs;
    QList<QOpenGLFramebufferObject *> dataCubeFBOs;

    static float LoD[16], HiD[16], LoR[16], HiR[16];

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

    void initializeParameters();
    void initializeShaders();
    void initializeTextures();
    void initializeVertices();

    LAUScan computeVectorU(LAUScan scan);
    LAUScan computeVectorV(LAUScan scan);

    LAUScan subtractScans(LAUScan scanA, LAUScan scanB);
    LAUScan addScans(LAUScan scanA, LAUScan scanB);
    LAUScan multiplyScans(float scalar, LAUScan scanB);
    LAUScan createScan(float tau, LAUScan scan);
    LAUScan maxScans(LAUScan scanA, LAUScan scanB);
    LAUScan minScans(LAUScan scanA, LAUScan scanB);
    float computeMSE(LAUScan scanA, LAUScan scanB);
    float innerProduct(LAUScan scanA, LAUScan scanB);
    float objectiveFun(LAUScan vectorResidue, LAUScan vectorU, LAUScan vectorV, float tau);
    float maxAbsValue(LAUScan scan);
    float sumAbsValue(LAUScan scan);
    int nonZeroElements(LAUScan scan);

    //GPU VERSION
    void computeVectorU(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);
    void computeVectorV(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);

    void subtractScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout);
    void addScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout);
    void multiplyScans(float scalar, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);
    void createScan(float tau, QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);
    void maxScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout);
    void minScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB, QOpenGLFramebufferObject * fboout);
    void copyfbo(QOpenGLFramebufferObject *fbo, QOpenGLFramebufferObject * fboout);
    float computeMSE(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB);
    float innerProduct(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB);
    float sumScans(QOpenGLFramebufferObject *fboA, QOpenGLFramebufferObject *fboB);
    float objectiveFun(QOpenGLFramebufferObject *fboResidue, QOpenGLFramebufferObject *fboU, QOpenGLFramebufferObject *fboV, float tau);
    float maxAbsValue(QOpenGLFramebufferObject *fbo);
    float sumAbsValue(QOpenGLFramebufferObject *fbo);
    int nonZeroElements(QOpenGLFramebufferObject *fbo);


};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCodedApertureGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit LAUCodedApertureGLWidget(unsigned int cols = 0, unsigned int rows = 0, QWidget *parent = 0);
    explicit LAUCodedApertureGLWidget(LAUScan scn, QWidget *parent = 0);
    ~LAUCodedApertureGLWidget();

    virtual bool isValid() const
    {
        return (wasInitialized());
    }

    bool wasInitialized() const
    {
        return (vertexArrayObject.isCreated());
    }

protected:
    void keyPressEvent(QKeyEvent *event)
    {
        if (event->key() == Qt::Key_Up) {
            onIncrementChannel();
        } else if (event->key() == Qt::Key_Down) {
            onDecrementChannel();
        }
        QOpenGLWidget::keyPressEvent(event);
    }
    void wheelEvent(QWheelEvent *event);
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

public slots:
    void onUpdateScan(LAUScan scn);
    void onIncrementChannel()
    {
        channel++;
        update();
    }
    void onDecrementChannel()
    {
        channel--;
        update();
    }

private:
    unsigned int localHeight, localWidth, channel;

    LAUScan scan;
    QOpenGLTexture *dataCube;
    QOpenGLShaderProgram prgrm;
    QOpenGLBuffer vertexBuffer, indexBuffer;
    QOpenGLVertexArrayObject vertexArrayObject;
};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCodedApertureWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LAUCodedApertureWidget(LAUScan scn, QWidget *parent = 0);
    ~LAUCodedApertureWidget();

    LAUScan smoothedScan();
    static bool inspectScan(LAUScan scan, QWidget *parent = NULL);

public slots:
    void onSetCodedAperture();

private:
    LAUScan scan;
    LAUCodedApertureGLWidget *glWidget;
    LAUCodedApertureGLFilter *codedApertureFilter;

signals:

};

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
class LAUCodedApertureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LAUCodedApertureDialog(LAUScan scan, QWidget *parent = 0) : QDialog(parent)
    {
        widget = new LAUCodedApertureWidget(scan);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
        connect(buttonBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

        QPushButton *button = new QPushButton(QString("Set Aperture"));
        connect(button, SIGNAL(clicked()), widget, SLOT(onSetCodedAperture()));
        buttonBox->addButton(button, QDialogButtonBox::ActionRole);

        this->setLayout(new QVBoxLayout());
        this->layout()->setContentsMargins(6, 6, 6, 6);
        this->layout()->addWidget(widget);
        this->layout()->addWidget(buttonBox);
    }

protected:
    void accept()
    {
        // GIVE THE USER THE CHANCE TO SAVE THE RESULTING SCAN TO DISK
        LAUScan result = widget->smoothedScan();
        if (result.isValid()) {
            if (result.save(QString())) {
                QDialog::accept();
            }
        }
    }

private:
    LAUCodedApertureWidget *widget;
};

#endif // LAUCODEDAPERTUREGLFILTER_H
