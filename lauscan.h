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

#ifndef LAUSCAN_H
#define LAUSCAN_H

#include <QDir>
#include <QFile>
#include <QList>
#include <QTime>
#include <QSize>
#include <QtCore>
#include <QImage>
#include <QDebug>
#include <QLabel>
#include <QtCore>
#include <QDialog>
#include <QObject>
#include <QtEndian>
#include <QVector3D>
#include <QCheckBox>
#include <QGLWidget>
#include <QMatrix4x4>
#include <QByteArray>
#include <QStringList>
#include <QSharedData>
#include <QFileDialog>
#include <QScrollArea>
#include <QDataStream>
#include <QPushButton>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <QProgressDialog>
#include <QDialogButtonBox>

#include "Eigen/Eigen"
#include "xmmintrin.h"
#include "smmintrin.h"
#include "tmmintrin.h"

#include "laumemoryobject.h"

using namespace LAU3DVideoParameters;

class LAUScan : public LAUMemoryObject
{
public:
    explicit LAUScan(unsigned int cols = 0, unsigned int rows = 0, LAUVideoPlaybackColor clr = ColorUndefined);
    explicit LAUScan(QString fileName);
    ~LAUScan() { ; }

    LAUScan(libtiff::TIFF *inTiff);
    LAUScan(const LAUMemoryObject &other, LAUVideoPlaybackColor clr);
    LAUScan(const LAUScan &other);
    LAUScan &operator = (const LAUScan &other);
    LAUScan &operator + (const LAUScan &other);

    void updateLimits();

    bool save(QString filename = QString());
    bool save(libtiff::TIFF *otTiff, int index = 0);
    bool load(libtiff::TIFF *inTiff);

    QVector<float> pixel(int col, int row) const;
    QVector<float> pixel(QPoint point) const;
    QVector3D centroid() const
    {
        return (com);
    }

    void setFilename(const QString string)
    {
        fileString = string;
    }

    QString filename() const
    {
        return (fileString);
    }

    void setMake(const QString string)
    {
        makeString = string;
    }

    QString make() const
    {
        return (makeString);
    }

    void setModel(const QString string)
    {
        modelString = string;
    }

    QString model() const
    {
        return (modelString);
    }

    void setSoftware(const QString string)
    {
        softwareString = string;
    }

    QString software() const
    {
        return (softwareString);
    }

    float minX() const
    {
        return (xMin);
    }

    float maxX() const
    {
        return (xMax);
    }

    float minY() const
    {
        return (yMin);
    }

    float maxY() const
    {
        return (yMax);
    }

    float minZ() const
    {
        return (zMin);
    }

    float maxZ() const
    {
        return (zMax);
    }

    QPointF zLimits() const
    {
        return (QPointF(zMin, zMax));
    }

    void setZLimits(QPointF point)
    {
        zMin = point.x();
        zMax = point.y();
    }

    void setZLimits(float zmn, float zmx)
    {
        zMin = zmn;
        zMax = zmx;
    }

    QPointF fieldOfView() const
    {
        return (fov);
    }

    void setFov(QPointF fov)
    {
        fov = fov;
    }

    QTime timeStamp() const
    {
        return (time);
    }

    void setTimeStamp(QTime tm)
    {
        time = tm;
    }

    QString parentName() const
    {
        return (QString(parentByteArray));
    }

    void setParentName(QString string)
    {
        parentByteArray = string.toLatin1();
    }

    LAUScan resize(unsigned int cols, unsigned int rows);
    LAUScan crop(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    LAUScan extractChannel(unsigned int channel) const;
    LAUScan transformScan(QMatrix4x4 mat);

    QVector<float> const boundingBox();
    QMatrix4x4 const lookAt();
    QVector3D const center();

    QList<QImage> nearestNeighborMap();

    LAUVideoPlaybackColor color() const
    {
        return (playbackColor);
    }
    LAUScan convertToColor(LAUVideoPlaybackColor col) const;
    LAUScan maskChannel(unsigned int chn, float threshold) const;
    int extractXYZWVertices(float *toBuffer, int downSampleFactor = 1) const;
    int pointCount() const;

    QImage preview(QSize size, Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);
    bool approveImage(bool *doNotShowAgainCheckBoxEnabled = NULL, QWidget *parent = NULL);
    int inspectImage();

    static QString lastUsedDirectory;
    static LAUScan mergeScans(QList<LAUScan> scans);
    static LAUScan loadFromSKW(QString filename);
    static LAUScan loadFromCSV(QString filename);
    static float fitCircle(float *points, int length, int step, int offset, float *weights);

private:
    QTime time;
    QString fileString;
    QString makeString;
    QString modelString;
    QString softwareString;
    QByteArray parentByteArray;
    LAUVideoPlaybackColor playbackColor;

    float xMin, xMax, yMin, yMax, zMin, zMax;
    QVector3D com;
    QPointF fov;
};
#endif // LAUSCAN_H
