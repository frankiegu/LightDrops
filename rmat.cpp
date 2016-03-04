#include "rmat.h"
#include <qdebug.h>

RMat::RMat()
{
    cv::Mat emptyMat;
    this->matImage = emptyMat;
    this->bayer = false;
    this->bscale = 1;
    this->bzero = 0;

}

RMat::RMat(cv::Mat &matImage) : dataMin(0), dataMax(0), bscale(1), bzero(0)
{
    this->matImage = matImage;
    this->bayer = false;
    this->imageTitle = QString("Image #");
    this->instrument = instruments::generic;

    if (matImage.channels() > 1)
    {
        cv::cvtColor(matImage, matImageGray, CV_RGB2GRAY);
        cv::minMaxLoc(matImageGray, &dataMin, &dataMax);
    }
    else
    {
        matImageGray = matImage;
        cv::minMaxLoc(matImage, &dataMin, &dataMax);
    }

    calcStats();
}

RMat::RMat(cv::Mat &matImage, bool bayer) : dataMin(0), dataMax(0), bscale(1), bzero(0)
{
    this->matImage = matImage;
    this->bayer = bayer;
    this->imageTitle = QString("Image #");
    this->instrument = instruments::generic;

    if (matImage.channels() > 1)
    {
        cv::cvtColor(matImage, matImageGray, CV_RGB2GRAY);
        cv::minMaxLoc(matImageGray, &dataMin, &dataMax);
    }
    else
    {
        matImageGray = matImage;
        cv::minMaxLoc(matImage, &dataMin, &dataMax);
    }

    calcStats();
}

RMat::RMat(cv::Mat &matImage, bool bayer, instruments instrument) : dataMin(0), dataMax(0), bscale(1), bzero(0)
{
    this->matImage = matImage;
    this->bayer = bayer;
    this->imageTitle = QString("Image #");
    this->instrument = instrument;

    if (matImage.channels() > 1)
    {
        cv::cvtColor(matImage, matImageGray, CV_RGB2GRAY);
        cv::minMaxLoc(matImageGray, &dataMin, &dataMax);
    }
    else
    {
        matImageGray = matImage;
        cv::minMaxLoc(matImage, &dataMin, &dataMax);
    }

    calcStats();
}



RMat::~RMat()
{
    matImage.release();
}

bool RMat::empty()
{
    return matImage.empty();
}

void RMat::computeHist(int nBins, float minRange, float maxRange)
{
    float range[2];
    range[0] = minRange;
    range[1] = maxRange;
    const float* histRange = { range };
    bool uniform = true;
    bool accumulate = false;

    /// Compute the histograms:
    cv::Mat matImageGrayShifted;
//    matImageGray.copyTo(matImageGrayShifted);
//    matImageGrayShifted = matImageGrayShifted + 350;
    cv::calcHist( &matImageGray, 1, 0, cv::Mat(), matHist, 1, &nBins, &histRange, uniform, accumulate);
}

void RMat::calcStats()
{
    float minRange = 0;
    float maxRange = 0;

    cv::Scalar meanScalar;
    cv::Scalar stdDevScalar;
    cv::meanStdDev(matImageGray, meanScalar, stdDevScalar);
    mean = (float) meanScalar.val[0];
    stdDev = (float) stdDevScalar.val[0];
    nPixels = (uint) matImage.cols * matImage.rows;

    /// Set the range and bins of the histogram
    /// These params depend on the instrument's sensor (different bit depths).

    int nBins = 65536;

    if (instrument == instruments::USET)
    {
        nBins = 4096;
    }

    if (dataMin >= 0)
    {
        minRange = 0.0f;
        maxRange = (float) nBins-1;
    }
    else
    {
        minRange = (float) dataMin;
        maxRange = (float) dataMax;
    }

    // Get histogram
    computeHist(nBins, minRange, maxRange);
   // Calculate median
    median = calcMedian();

    histWidth = (maxRange - minRange)/(double) (nBins -1);

//    percentileLow = calcPercentile(0.05, histWidth, minRange);
//    percentileHigh = calcPercentile( 0.9985, histWidth, minRange);
    // The percentileLow and percentileHigh are the intensities calculated
    // with the percentiles set such that the CDF of the histogram is 1000 pixels less than
    // the total pixel count (for the High) and 1 pixel above 0.
    float cutOffLow;
    float cutOffHigh;

    if (instrument == instruments::USET)
    {
        cutOffLow = 0;
        cutOffHigh = 99.97f;
    }
    else
    {
        cutOffLow = 5.0f;
        cutOffHigh = 99.85f;
    }

    intensityLow = calcThreshold(cutOffLow, histWidth, minRange);
    intensityHigh = calcThreshold(cutOffHigh, histWidth, minRange);
    qDebug("RMat::calcStats():: median = %f", median);
    qDebug("RMat::calcStats():: percentile Low = %f", intensityLow);
    qDebug("RMat::calcStats():: percentile High = %f", intensityHigh);
}

float RMat::calcMedian()
{
    int cdf = 0;
    int nBins = matHist.rows;
    int medianLimit = std::round(nBins/2);

    float medianVal = -1;
    for (int i = 1; i < nBins && medianVal < 0.0; i++){
        cdf += matHist.at<float>(i);
        if (cdf > medianLimit) { medianVal = i;}
    }

    return medianVal;
}

float RMat::calcThreshold(float cutOff, double histWidth, float minRange)
{
    float cdf = 0.0f;
    bool checkThreshold = false;
    int nBins = matHist.rows;

    int index = 0;
    int i =0;
    while (i < nBins && checkThreshold == false){
        cdf += matHist.at<float>(i);
        checkThreshold = (100.0f * cdf/nPixels > cutOff);
        if (checkThreshold) { index = i;}
        i++;
    }

   float intensity = minRange + (float) histWidth * (float) index;

   return intensity;
}

// Getters
bool RMat::isBayer() const
{
    return bayer;
}

int RMat::getBscale() const
{
    return bscale;
}

int RMat::getBzero() const
{
    return bzero;
}

double RMat::getDataMin() const
{
    return dataMin;
}

double RMat::getDataMax() const
{
    return dataMax;
}

float RMat::getWbRed() const
{
    return wbRed;
}

float RMat::getWbGreen() const
{
    return wbGreen;
}

float RMat::getWbBlue() const
{
    return wbBlue;
}

instruments RMat::getInstrument() const
{
    return instrument;
}

QString RMat::getImageTitle() const
{
    return imageTitle;
}

cv::Mat RMat::getMatHist() const
{
    return matHist;
}

float RMat::getMean() const
{
    return mean;
}

float RMat::getStdDev() const
{
    return stdDev;
}

float RMat::getMedian() const
{
    return median;
}

float RMat::getIntensityLow() const
{
    return intensityLow;
}

float RMat::getIntensityHigh() const
{
    return intensityHigh;
}

double RMat::getHistWidth() const
{
    return histWidth;
}

uint RMat::getNPixels() const
{
    return nPixels;
}


// Setters
void RMat::setBayer(bool bayer)
{
    this->bayer = bayer;
}

void RMat::setBscale(int bscale)
{
    this->bscale = bscale;
}

void RMat::setBzero(int bzero)
{
    this->bzero = bzero;
}

void RMat::setDataMin(float dataMin)
{
    this->dataMin = dataMin;
}

void RMat::setDataMax(float dataMax)
{
    this->dataMax = dataMax;
}

void RMat::setWbRed(float wbRed)
{
    this->wbRed = wbRed;
}

void RMat::setWbGreen(float wbGreen)
{
    this->wbGreen = wbGreen;
}

void RMat::setWbBlue(float wbBlue)
{
    this->wbBlue = wbBlue;
}

void RMat::setImageTitle(QString title)
{
    this->imageTitle = title;
}

void RMat::setInstrument(instruments instrument)
{
    this->instrument = instrument;
}

