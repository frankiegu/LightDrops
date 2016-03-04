#include "RawImage.h"

// libraw
#include <libraw.h>

// opencv
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// Qt
#include <iostream>
#include <QString>
#include <QDebug>
#include <QColor>
#include <QImage>
#include <QElapsedTimer>

using namespace cv;

RawImage::RawImage()
{

}

RawImage::RawImage(QString filePath):imageProcessed(NULL)
{
    QElapsedTimer timer1, timer2;

    int error = 0;
    LibRaw rawProcess = LibRaw();

    std::string filePathStr(filePath.toStdString());

    qDebug() << "Raw file path:" + filePath.toLatin1();

    error = rawProcess.open_file(filePathStr.c_str());

    if (error !=0)
    {
        qDebug() << "error on open_file = " << error;
    }

    //rawProcess.imgdata.params.use_camera_matrix = 0;
    rawProcess.imgdata.params.use_camera_wb = 0;
    rawProcess.imgdata.params.no_auto_bright = 1;
    //rawProcess.imgdata.params.user_qual = -1;


    timer1.start();

    error = rawProcess.unpack();
    if (error!=0)
    {
        qDebug() << "unpack() error =" << error;
    }
    qDebug()<<"Time for rawProcess.unpack()=" <<  timer1.elapsed() << "ms";
    qDebug() << "Bayer Pattern: rawProcess.imgdata.idata.cdesc=" << rawProcess.imgdata.idata.cdesc;

    naxis1 = rawProcess.imgdata.sizes.iwidth;
    naxis2 = rawProcess.imgdata.sizes.iheight;
    nPixels = naxis1 * naxis2;

    //    rawProcess.subtract_black();

    // Advised by lexa (Author of Libraw):
    //    Each row contains non-visible pixels on both ends:
    //    - on left (0.... left_margin-1)
    //    - and on right (from visible area end to row end)
    //    Also, rows may be aligned for efficient SSE/AVX access. LibRaw internal code do not align rows, but if you use LibRaw+RawSpeed, RawSpeed will align rows on 16 bytes.
    //    So, it is better to use imgdata.sizes.raw_pitch (it is in bytes, so divide /2 for bayer data) instead of raw_width.

    //    int raw_width = (int) rawProcess.imgdata.sizes.raw_width;
    int raw_width = (int) rawProcess.imgdata.sizes.raw_pitch/2;
    int top_margin = (int) rawProcess.imgdata.sizes.top_margin;
    int left_margin = (int) rawProcess.imgdata.sizes.left_margin;
    int first_visible_pixel = (int) (raw_width * top_margin + left_margin);

    qDebug() << "first_visible_pixel =" << first_visible_pixel;


    matCFA.create(naxis2, naxis1, CV_16UC1);
    ushort *rawP = matCFA.ptr<ushort>(0);

    // The following block re-assign the data in a more contiguous form,
    // where the "dark" margins (top and left) have been removed from the raw buffer (rawdata.raw_image).
    // Hence the dimension of this new buffer (rawP) match the final image size of the camera.
    // We convert them afterwards to float.
    // These steps cost about 150 ms at most on a 22 MP DSLR, core i5 macbook Pro Retina 2013.
    // Without parallelization (QThread or alike), this is about 5-10% of the total time to load and display the image.
    long j=0;
    for(int r = 0; r < naxis2; r++)
    {
        for(int c=0; c < naxis1; c++)
        {
            rawP[j] = rawProcess.imgdata.rawdata.raw_image[(r + top_margin)*raw_width + left_margin + c];
            j++;
        }
    }
   matCFA.convertTo(matCFA, CV_32F);


    // White balance
    wbRed = rawProcess.imgdata.color.cam_mul[0] /1000.0f;
    wbGreen = rawProcess.imgdata.color.cam_mul[1]/1000.0f;
    wbBlue = rawProcess.imgdata.color.cam_mul[2]/1000.0f;

    qDebug("white balance cam_mul =%f , %f , %f" , wbRed, wbGreen, wbBlue);

    //timer2.restart();
    //std::vector<Mat> rgbChannels(3);
    //split(bayerMat32, rgbChannels);
    //imRed = rgbChannels.at(0);
    //imGreen = rgbChannels.at(1);
    //imBlue = rgbChannels.at(2);

    //imRed = imRed * redBalance;
    //imGreen = imGreen * greenBalance;
    //imBlue = imBlue * blueBalance;

    //std::vector<cv::Mat> matVector;
    //matVector.push_back(imRed);
    //matVector.push_back(imGreen);
    //matVector.push_back(imBlue);
    //cv::merge(matVector, bayerMat32);


    //qDebug()<<"Time for openCV white balancing:" <<  timer2.elapsed() << "ms";


    // raw2image()
    //    timer2.start();
    //    error = rawProcess.raw2image();
    //    if (error !=0)
    //    {
    //        qDebug() << "error on raw2image()";
    //    }
    //    qDebug()<<"Time for rawProcess.raw2image()" <<  timer2.elapsed() << "ms";


    //    for(int i = 0;i < nPixels; i++)
    //    {
    //        qDebug("i=%d R=%d G=%d B=%d G2=%d\n", i,
    //                     rawProcess.imgdata.image[i][0],
    //                     rawProcess.imgdata.image[i][1],
    //                     rawProcess.imgdata.image[i][2],
    //                     rawProcess.imgdata.image[i][3]
    //             );
    //    }

    //    timer2.restart();
    //    error = rawProcess.dcraw_process();
    //    if (error !=0)
    //    {
    //        qDebug() << "error on dcraw_process()";
    //    }
    ////    qDebug()<<"Time for rawProcess.dcraw_process()" <<  timer2.elapsed() << "ms";

    //    ushort *redP = imRed.ptr<ushort>(0);
    //    ushort *greenP = imGreen.ptr<ushort>(0);
    //    ushort *blueP = imBlue.ptr<ushort>(0);

    //    for (int i = 0; i < nPixels; i++)
    //    {

    //        redP[i] = rawProcess.imgdata.image[i][0];
    //        greenP[i] = rawProcess.imgdata.image[i][1];
    //        blueP[i] = rawProcess.imgdata.image[i][2];
    //    }


    //    std::vector<cv::Mat> matVector;
    //    matVector.push_back(imRed);
    //    matVector.push_back(imGreen);
    //    matVector.push_back(imBlue);
    ////    //matVector.push_back(imGreen2);

    //    cv::Mat bayerMat32b;
    //    cv::merge(matVector, bayerMat32b);
    //    bayerMat32b.convertTo(bayerMat32b, CV_32FC3);

    //    qDebug()<<"Time for dcraw_process() and channel assignment: " <<  timer2.elapsed() << "ms";

    //    Mat bayerMat16, rgbMat16;
    //    bayerMat32.convertTo(bayerMat16, CV_16UC1);
    //    cvtColor(bayerMat16, rgbMat16, CV_BayerBG2RGB);
    //    rgbMat16.convertTo(bayerMat32, CV_32FC3);


}

RawImage::~RawImage()
{
    imRed.release();
    imGreen.release();
    imBlue.release();
    delete[] imageProcessed;
}

LibRaw RawImage::getRawProcess() const
{
    return rawProcess;
}


Mat RawImage::getImRed() const
{
    return imRed;
}

Mat RawImage::getImGreen() const
{
    return imGreen;
}

Mat RawImage::getImBlue() const
{
    return imBlue;
}


qint32 RawImage::getNaxis1() const
{
    return naxis1;
}

qint32 RawImage::getNaxis2() const
{
    return naxis2;
}

int RawImage::getNPixels() const
{
    return nPixels;
}


float RawImage::getDataMaxRed() const
{
    return dataMaxRed;
}

float RawImage::getDataMaxGreen() const
{
    return dataMaxGreen;
}

float RawImage::getDataMaxBlue() const
{
    return dataMaxBlue;
}

float RawImage::getDataMinRed() const
{
    return dataMinRed;
}

float RawImage::getDataMinGreen() const
{
    return dataMinGreen;
}

float RawImage::getDataMinBlue() const
{
    return dataMinBlue;
}

float RawImage::getWbRed() const
{
    return wbRed;
}

float RawImage::getWbGreen() const
{
    return wbGreen;
}

float RawImage::getWbBlue() const
{
    return wbBlue;
}
