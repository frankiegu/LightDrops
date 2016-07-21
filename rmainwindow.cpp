#include <QMdiSubWindow>
#include <QDropEvent>
#include <QFileDialog>

#include "rmainwindow.h"
#include "ui_rmainwindow.h"
#include "rsubwindow.h"

//QCustomPlot
#include <qcustomplot/qcustomplot.h>


using namespace std;

RMainWindow::RMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RMainWindow),
    currentROpenGLWidget(NULL),
    resultROpenGLWidget(NULL),
    cannyContoursROpenGLWidget(NULL),
    limbFittingROpenGLWidget(NULL),
    graphicsView(NULL),
    cannySubWindow(NULL), limbRegisterSubWindow(NULL),
    currentSubWindow(NULL), plotSubWindow(NULL), tempSubWindow(NULL),
    toneMappingGraph(NULL)
{
    ui->setupUi(this);

    //QRect rec = QApplication::desktop()->screenGeometry();

    this->showMaximized();
    setCentralWidget(ui->mdiArea);
    processing = new RProcessing(this);
    processing->setTreeWidget(ui->treeWidget);

    vertLineHigh = NULL;

    defaultWindowSize = QSize(512, 512);
    sliderScale = 1.0;
    gammaScale = 0.1;
    gammaMin = ui->doubleSpinBoxGamma->minimum();
    gamma = 1.0f;

    sliderScaleWB = 0.01;
    lastFrameIndex = 0;

    setupSubImage();

//    tabifyDockWidget(ui->currentFrameDock, ui->settingsDock);
//    this->setTabPosition(Qt::DockWidgetArea::BottomDockWidgetArea, QTabWidget::TabPosition::South);

    //this->setTabShape(QTabWidget::Triangular);

    connect(this, SIGNAL(tempMessageSignal(QString,int)), this->statusBar(), SLOT(showMessage(QString,int)));

    /// Connect the change of slider value to the number displayed in the spinBox
    connect(ui->sliderHigh, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    connect(ui->sliderLow, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));
    connect(ui->sliderGamma, SIGNAL(valueChanged(int)), this, SLOT(updateDoubleSpinBox(int)));

    /// Connect the change of the number in the spinBox to the change of slider value
    ///Use editingFinished() instead of valueChanged(double()) to avoid unwanted feedback on the slider.
    connect(ui->doubleSpinBoxHigh, SIGNAL(editingFinished()), this, SLOT(updateSliderValueSlot()));
    connect(ui->doubleSpinBoxLow, SIGNAL(editingFinished()), this, SLOT(updateSliderValueSlot()));
    connect(ui->doubleSpinBoxGamma, SIGNAL(editingFinished()), this, SLOT(updateSliderValueSlot()));

    /// Connect the change of slider value to the image linear scaling and gamma scaling.
    connect(ui->sliderHigh, SIGNAL(valueChanged(int)), this, SLOT(scaleImageSlot(int)));
    connect(ui->sliderLow, SIGNAL(valueChanged(int)), this, SLOT(scaleImageSlot(int)));
    connect(ui->sliderGamma, SIGNAL(valueChanged(int)), this, SLOT(gammaScaleImageSlot(int)));

    /// Connect the white balance sliders
    connect(ui->redSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWB(int)));
    connect(ui->blueSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWB(int)));
    connect(ui->greenSlider, SIGNAL(valueChanged(int)), this, SLOT(updateWB(int)));

    /// Connect the pushButton and lineEdit of the ui for creating the masters in the processing class
    connect(ui->makeMasterButton, SIGNAL(pressed()), processing, SLOT(createMasters()));

    connect(ui->calibrateDirLineEdit, SIGNAL(textChanged(QString)), processing, SLOT(setExportCalibrateDir(QString)));

    /// Connect the sliderFrame and playback buttons
    connect(ui->sliderFrame, SIGNAL(valueChanged(int)), this, SLOT(updateFrameInSeries(int)));
    connect(ui->forwardButton, SIGNAL(released()), this, SLOT(stepForward()));
    connect(ui->backwardButton, SIGNAL(released()), this, SLOT(stepBackward()));
    connect(ui->playButton, SIGNAL(released()), this, SLOT(playForward()));
    connect(ui->stopButton, SIGNAL(released()), this, SLOT(stopButtonPressed()));

    /// Connect processing outputs to the main ui.
    connect(processing, SIGNAL(tempMessageSignal(QString,int)), this->statusBar(), SLOT(showMessage(QString,int)));
    connect(processing, SIGNAL(resultSignal(RMat*)), this, SLOT(createNewImage(RMat*)));
    connect(processing, SIGNAL(resultSignal(cv::Mat, bool, instruments)), this, SLOT(createNewImage(cv::Mat, bool, instruments)));
    connect(processing, SIGNAL(resultSignal(cv::Mat, bool, instruments, QString)), this, SLOT(createNewImage(cv::Mat, bool, instruments, QString)));
    connect(processing, SIGNAL(listResultSignal(QList<RMat*>)), this, SLOT(createNewImage(QList<RMat*>)));

    /// Connect ui radio button to tree widget to send out window titles.
    connect(ui->lightRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));
    connect(ui->biasRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));
    connect(ui->darkRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));
    connect(ui->flatRButton, SIGNAL(clicked(bool)), this, SLOT(radioRMatSlot()));

    // Connect the radioLight, radioBias,... signals to the treeWidget
    connect(this, SIGNAL(radioLightImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromLightRButton(QList<RMat*>)));
    connect(this, SIGNAL(radioBiasImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromBiasRButton(QList<RMat*>)));
    connect(this, SIGNAL(radioDarkImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromDarkRButton(QList<RMat*>)));
    connect(this, SIGNAL(radioFlatImages(QList<RMat*>)), ui->treeWidget, SLOT(rMatFromFlatRButton(QList<RMat*>)));

    // Connect Canny detection
    connect(ui->cannySlider, SIGNAL(sliderReleased()), this, SLOT(cannyEdgeDetection()));

    // Connect QImage scenes
    connect(processing, SIGNAL(resultQImageSignal(QImage&)), this, SLOT(createNewImage(QImage&)));

    //connect(processing, SIGNAL(ellipseSignal(cv::RotatedRect)), this, SLOT(addEllipseToScene(cv::RotatedRect)));

    // Connect Fit stats button
    connect(ui->plotFitStatsButton, SIGNAL(released()), this, SLOT(showLimbFitStats()));
    // Connect Tone mapping button
    connect(ui->toneMappingButton, SIGNAL(released()), this, SLOT(setupToneMappingCurve()));
    connect(ui->toneMappingSlider1, SIGNAL(valueChanged(int)), this, SLOT(updateToneMappingSlot()));
    connect(ui->toneMappingSlider2, SIGNAL(valueChanged(int)), this, SLOT(updateToneMappingSlot()));
    connect(ui->toneMappingSlider3, SIGNAL(valueChanged(int)), this, SLOT(updateToneMappingSlot()));
    connect(ui->applyToneMappingCheckBox, SIGNAL(toggled(bool)), this, SLOT(applyToneMappingCurve()));

    // Fourier Filters
   qDebug() << "mdiArea->size =" << ui->mdiArea->size();

}

RMainWindow::~RMainWindow()
{
    delete ui;
    delete processing;
}

void RMainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (!mimeData->hasUrls())
        return;

    event->acceptProposedAction();

    RListImageManager *newRListImageManager = new RListImageManager(mimeData->urls());
    if (newRListImageManager->getRMatImageList().empty())
    {
        qDebug("Nothing to load: wrong file extention?");
        return;
    }

    createNewImage(newRListImageManager);

}

void RMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void RMainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void RMainWindow::createNewImage(RListImageManager *newRListImageManager)
{
    currentROpenGLWidget = new ROpenGLWidget(newRListImageManager, this);


    addImage(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
    currentSubWindow->show();
    setupSliders(currentROpenGLWidget);
    autoScale(currentROpenGLWidget);

    //dispatchRMatImages(currentROpenGLWidget->getRMatImageList());
    ui->treeWidget->rMatLightList = newRListImageManager->getRMatImageList();
    processing->rMatLightList = newRListImageManager->getRMatImageList();

    ui->treeWidget->rMatFromLightRButton(newRListImageManager->getRMatImageList());

    ///--- Test of Fourier Filtering ---///
//    cv::Mat matImage = currentROpenGLWidget->getRMatImageList().at(0)->matImage.clone();
//    cv::Mat matImageHPF = processing->makeImageHPF(matImage, 30);
//    createNewImage(matImageHPF, false, instruments::generic, QString("Filtered"));
    currentROpenGLWidget->setAttribute(Qt::WA_DeleteOnClose, true);
}



void RMainWindow::createNewImage(QList<RMat*> newRMatImageList)
{
    currentROpenGLWidget = new ROpenGLWidget(newRMatImageList, this);
    addImage(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
    currentSubWindow->show();
    setupSliders(currentROpenGLWidget);
    currentROpenGLWidget->setAttribute(Qt::WA_DeleteOnClose, true);
}

void RMainWindow::createNewImage(RMat *rMatImage)
{
    currentROpenGLWidget = new ROpenGLWidget(rMatImage, this);
    addImage(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
    currentSubWindow->show();
    setupSliders(currentROpenGLWidget);
    autoScale(currentROpenGLWidget);
}


void RMainWindow::createNewImage(cv::Mat cvImage, bool bayer, instruments instrument, QString imageTitle)
{
    RMat *rMatImage = new RMat(cvImage, bayer, instrument);
    rMatImage->setImageTitle(imageTitle);
    createNewImage(rMatImage);
}

void RMainWindow::createNewImage(QImage &image)
{
    //QSize currentSize = ui->mdiArea->currentSubWindow()->size();
    QPixmap pixMap = QPixmap::fromImage(image);

    QGraphicsScene *newScene = new QGraphicsScene;
    newScene->addPixmap(pixMap);

    graphicsView= new QGraphicsView(newScene);

    QMdiSubWindow *newSubWindow = new QMdiSubWindow;
    newSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    newSubWindow->setWidget(graphicsView);
    newSubWindow->setWindowTitle(QString("QImage"));
    ui->mdiArea->addSubWindow(newSubWindow);

    newSubWindow->show();
    //newSubWindow->resize(currentSize);

}

void RMainWindow::displayQImage(QImage &image, RGraphicsScene *scene, QMdiSubWindow *subWindow, QString windowTitle)
{
    QPixmap pixMap = QPixmap::fromImage(image);

    qDebug("RMainWindow::createNewImage(QImage &image):: pixMap.size() = [%i ; %i]", pixMap.size().width(), pixMap.size().height());
    //QPainter *painter = new QPainter(&image);

    //if (graphicsView == NULL)
    //{
        QSize lastSize = ui->mdiArea->currentSubWindow()->size();
        QPoint lastPos = ui->mdiArea->currentSubWindow()->pos();

        pixMapItem = scene->addPixmap(pixMap);
        // QPointF QGraphicsSceneMouseEvent::lastScenePos()
        //scene->mouseGrabberItem()

        QGraphicsView *newGraphicsView= new QGraphicsView(scene);
        //graphicsView->scale(1, -1);
        newGraphicsView->setDragMode(QGraphicsView::NoDrag);
        newGraphicsView->scale(currentROpenGLWidget->getResizeFac(), currentROpenGLWidget->getResizeFac());
        newGraphicsView->setCursor(Qt::CrossCursor);

        subWindow->setWidget(newGraphicsView);
        ui->mdiArea->addSubWindow(subWindow);
        subWindow->show();
        subWindow->resize(lastSize);
        subWindow->move(lastPos);
        subWindow->setWindowTitle(windowTitle);
        currentSubWindow = subWindow;

    //}
//    else
//    {
//        QSize lastSize = ui->mdiArea->currentSubWindow()->size();
//        QPoint lastPos = ui->mdiArea->currentSubWindow()->pos();

//        pixMapItem->setPixmap(pixMap);

//        QMdiSubWindow *sceneSubWindow = new QMdiSubWindow;
//        sceneSubWindow->setWidget(graphicsView);
//        ui->mdiArea->addSubWindow(sceneSubWindow);
//        sceneSubWindow->show();
//        sceneSubWindow->resize(lastSize);
//        sceneSubWindow->move(lastPos);
//        sceneSubWindow->setWindowTitle(windowTitle);
//    }


}

void RMainWindow::selectROI()
{
    if (currentROpenGLWidget == NULL)
    {
        ui->actionROISelect->setChecked(false);
        return;
    }


    lastROpenGLWidget = currentROpenGLWidget;
    /// Use copy constructor of the RMat to copy the current RMat image.
    RMat tempRMat(*currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value()));
    cv::Mat matImage = processing->normalizeByThresh(tempRMat.matImage, tempRMat.getIntensityLow(), tempRMat.getIntensityHigh(), tempRMat.getDataRange());
//    int width = 1000;
//    int height = 1000;
//    cv::Rect ROI(0, matImage.rows - 1 - height, width, height);
    cv::Mat matImageROI = matImage; //(ROI);

    matImageROI.convertTo(matImageROI, CV_8U, 256.0f/currentROpenGLWidget->getRMatImageList().at(0)->getDataRange());

    //QImage targetImage(matImageROI.cols, matImageROI.rows, QImage::Format_Grayscale8);
    QImage imageROI(matImageROI.data, matImageROI.cols, matImageROI.rows, QImage::Format_Grayscale8);
    QImage targetImage = imageROI.mirrored(false, true);
//    QPainter painter(&targetImage);
//    painter.scale(currentROpenGLWidget->getResizeFac(), -currentROpenGLWidget->getResizeFac());
//    painter.drawImage(0, -matImage.cols, imageROI);

    RGraphicsScene *roiScene = new RGraphicsScene();
    QMdiSubWindow *roiSubWindow = new QMdiSubWindow();
    roiSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    displayQImage(targetImage, roiScene, roiSubWindow, QString("Select ROI"));

    connect(roiScene, SIGNAL(ROIsignal(QRect)), this, SLOT(setRect(QRect)));
    connect(roiSubWindow, SIGNAL(destroyed(QObject*)), this, SLOT(disableROIaction()));

}

void RMainWindow::setRect(QRect rect)
{
    qDebug("RMainWindow::setRect() rect.x(), rect.y(), rect.width(), rect.height() = [%i, %i, %i, %i]", rect.x(), rect.y(), rect.width(), rect.height());
    this->rect = rect;
    cv::Rect cvRectROI(rect.x(), rect.y(), rect.width(), rect.height());
    processing->setCvRectROI(cvRectROI);
    ui->xROIBox->setValue(rect.x());
    ui->yROIBox->setValue(rect.y());
    ui->widthROIBox->setValue(rect.width());
    ui->heightROIBox->setValue(rect.height());

}

void RMainWindow::extractNewImageROI()
{

    if (currentROpenGLWidget == NULL)
    {
        ui->actionROIExtract->setCheckable(false);
        return;
    }



    if (rect.isEmpty())
    {
        ui->actionROIExtract->setCheckable(false);
        return;
    }



    currentROpenGLWidget = lastROpenGLWidget;
    cv::Rect cvRectROI(rect.x(), rect.y(), rect.width(), rect.height());

    float minThresh =  currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getIntensityLow();
    float maxThresh = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getIntensityHigh();
    cv::Mat matImage = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->matImage;
    //cv::Mat matImage = processing->normalizeByThresh(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value()), minThresh, maxThresh);
    cv::Mat matImageROI = matImage(cvRectROI);

    RMat *tempRMat = new RMat(matImageROI, false, currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getInstrument());
    tempRMat->setImageTitle(QString("ROI"));
    createNewImage(tempRMat);
    ui->sliderHigh->setValue(convertScaleToSlider(maxThresh));
    ui->sliderLow->setValue(convertScaleToSlider(minThresh));


//    float minThresh =  currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getIntensityLow();
//    float maxThresh = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getIntensityHigh();

//    cv::Mat matImage = processing->normalizeByThresh(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value()), minThresh, maxThresh);
//    matImage.convertTo(matImage, CV_8U, 256.0f/ currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataRange());

//    int x = 614;
//    int y = 1156;

//    // buggy
//    int width = 237;
//    int height = 273;

//    //working
//    //    int width = 400;
//    //    int height = 400;
//    QRect ROI(x, y, width, height);

//    QImage imageInit(matImage.data, matImage.cols, matImage.rows,   QImage::Format_Grayscale8);
//    QImage imageROI = imageInit.copy(ROI);
//    createNewImage(imageROI);

//    unsigned char* dataBuffer = imageROI.bits();
//    cv::Mat tempImage(cv::Size(imageROI.width(), imageROI.height()), CV_8UC1, dataBuffer, imageROI.bytesPerLine());

//    cv::namedWindow( "openCV imshow() from a cv::Mat image", cv::WINDOW_AUTOSIZE );
//    cv::imshow( "openCV imshow() from a cv::Mat image", tempImage);

}

void RMainWindow::disableROIaction()
{
    ui->actionROISelect->setChecked(false);
}

//void RMainWindow::addEllipseToScene(cv::RotatedRect rect)
//{
////    QGraphicsEllipseItem * QGraphicsScene::addEllipse(qreal x, qreal y, qreal w, qreal h, const QPen & pen = QPen(), const QBrush & brush = QBrush())
//    QPen pen;
//    pen.setColor(Qt::green);
//    QGraphicsEllipseItem *ellipseItem = new QGraphicsEllipseItem(0, 0, rect.size.width, rect.size.height);
//    ellipseItem->setPen(pen);
//    //ellipseItem->setScale(-currentROpenGLWidget->getResizeFac());
//    scene->addItem(ellipseItem);
//    qDebug("RMainWindow::addEllipseToScene():: rect.size.width = %f", rect.size.width);
//}

void RMainWindow::dispatchRMatImages(QList<RMat*> rMatList)
{
    //emit radioLightImages(rMatList);
    ui->treeWidget->rMatLightList = rMatList;
    processing->rMatLightList = rMatList;

    ui->treeWidget->rMatFromLightRButton(rMatList);
//    // Try to guess the calibration type (Light, Bias, Dark, or Flat)
//    // with the file names, or directory name, etc... and "radio" it out as if we were pressing the corresponding radio button
//    // to assign it to the corresponding QTreeWidgetItem in the treeWidget.

//    bool noBias = !(rMatList.at(0)->getImageTitle().contains(QString("bias"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getImageTitle().contains(QString("offset"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getFileInfo().absolutePath().contains(QString("bias"), Qt::CaseInsensitive));

//    bool noDark = !(rMatList.at(0)->getImageTitle().contains(QString("dark"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getFileInfo().absolutePath().contains(QString("dark"), Qt::CaseInsensitive));

//    bool noFlat = !(rMatList.at(0)->getImageTitle().contains(QString("flat"), Qt::CaseInsensitive) |
//                   rMatList.at(0)->getFileInfo().absolutePath().contains(QString("flat"), Qt::CaseInsensitive));

//    bool isAmbiguous = rMatList.at(0)->getFileInfo().absolutePath().contains(QString("light"), Qt::CaseInsensitive);

//    bool isBias = (!noBias & noDark & noFlat & !isAmbiguous) | rMatList.at(0)->getImageTitle().contains(QString("bias"), Qt::CaseInsensitive) |  rMatList.at(0)->getImageTitle().contains(QString("offset"), Qt::CaseInsensitive);
//    bool isDark = (!noDark & noBias & noFlat & !isAmbiguous) | rMatList.at(0)->getImageTitle().contains(QString("dark"), Qt::CaseInsensitive) ;
//    bool isFlat = (!noFlat & noDark & noBias & !isAmbiguous) | rMatList.at(0)->getImageTitle().contains(QString("flat"), Qt::CaseInsensitive) ;


//    if (isBias)
//    {
//        ui->biasRButton->click();
//    }
//    else if (isDark)
//    {
//        ui->darkRButton->click();
//    }
//    else if (isFlat)
//    {
//        ui->flatRButton->click();
//    }

}

void RMainWindow::addImage(ROpenGLWidget *rOpenGLWidget)
{

    ui->sliderFrame->blockSignals(true);

    ui->sliderFrame->setRange(0, rOpenGLWidget->getRMatImageList().size()-1);
    ui->sliderFrame->setValue(0);
    ui->imageLabel->setText(QString("1") + QString("/") + QString::number(rOpenGLWidget->getRMatImageList().size()));

    currentScrollArea = new QScrollArea();
    currentScrollArea->setWidget(rOpenGLWidget);
    currentScrollArea->setAttribute(Qt::WA_DeleteOnClose, true);
    /// The size of the scrollArea must be set to the size of whatever is needed for the ROpenGLWidget
    /// to display the whole FOV in the central widget when adding the height of the scroll bars.
    /// Note that the "height" of the scrollBar always represent the dimension perpendicular to the direction
    /// of the slider.
    resizeScrollArea(rOpenGLWidget, currentScrollArea); // Resize scrollArea only
    loadSubWindow(currentScrollArea); // also resize the current ROpenGLWidget. Poor design choice.
    rOpenGLWidget->update();


    processing->setCurrentROpenGLWidget(rOpenGLWidget);

    ui->sliderFrame->blockSignals(false);

    connect(rOpenGLWidget, SIGNAL(gotSelected(ROpenGLWidget*)), this, SLOT(changeROpenGLWidget(ROpenGLWidget*)));
    connect(rOpenGLWidget, SIGNAL(sendSubQImage(QImage*,float,int,int)), this, SLOT(updateSubFrame(QImage*,float,int,int)));


}


void RMainWindow::resizeScrollArea(ROpenGLWidget *rOpenGLWidget, QScrollArea *scrollArea)
{

    /// Need to leave just enough space for the scroll bars. So we have to remove the scrollBarHeight from the
    /// the size of ROpenGLWidget to make sure the latter fits tightly in the QScrollArea.
    int naxis1 = rOpenGLWidget->getRMatImageList().at(0)->matImage.cols;
    int naxis2 = rOpenGLWidget->getRMatImageList().at(0)->matImage.rows;

    float widthFac = (float) ((this->centralWidget()->width() - scrollArea->horizontalScrollBar()->height()) / (float) naxis1);
    float heightFac = (float) ((this->centralWidget()->height() - scrollArea->horizontalScrollBar()->height()) / (float) naxis2);
    float resizeFac = 1;

    if (widthFac > 1 && heightFac > 1)
    {
        qDebug("RMainWindow::resizeScrollArea():: Keeping original image size");
        //then default size = image size
        oglSize = QSize(naxis1, naxis2);
    }
    else // set other default size, smaller than original image size
    {
        qDebug("RMainWindow::resizeScrollArea():: resizing image");
        resizeFac = std::min(widthFac, heightFac);
        oglSize = QSize((int) ((float) naxis1 * resizeFac), (int) ((float) naxis2 * resizeFac));
    }

    rOpenGLWidget->setResizeFac(resizeFac);
    scrollArea->resize(oglSize + QSize(5, 25));


//    oglSize = QSize(naxis1, naxis2);
//    rOpenGLWidget->setResizeFac(1);
//    scrollArea->resize(QSize(600, 600) + QSize(5, 25));

}

void RMainWindow::loadSubWindow(QScrollArea *scrollArea)
{
    currentSubWindow = new QMdiSubWindow;
    //currentSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    currentSubWindow->setWidget(scrollArea);
    ui->mdiArea->addSubWindow(currentSubWindow);
    currentSubWindow->setWindowTitle(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getImageTitle());

    /// ROpenGLWidget can only be resized once it has been assigned to the QMdiSubWindow.
    /// Otherwise the shaders will not be bound.
    currentROpenGLWidget->resize(oglSize);
    currentSubWindow->resize(scrollArea->size());
    currentSubWindow->setAttribute(Qt::WA_DeleteOnClose, true);

}



void RMainWindow::updateDoubleSpinBox(int value)
{
    if (this->sender() == ui->sliderHigh)
    {
        float valueF = convertSliderToScale(value);
        ui->doubleSpinBoxHigh->setValue(valueF);
        qDebug("updateDoubleSpinBox:: (high) valueF = %f", valueF);
    }
    else if (this->sender() == ui->sliderLow)
    {
        float valueF = convertSliderToScale(value);
        ui->doubleSpinBoxLow->setValue(valueF);
    }
    if (this->sender() == ui->sliderGamma)
    {
        float valueF = convertSliderToGamma(value);
        ui->doubleSpinBoxGamma->setValue(valueF);
    }


}

void RMainWindow::scaleImageSlot(int value)
{
    if (currentROpenGLWidget == NULL)
        return;

    if (this->sender() == ui->sliderHigh)
    {
        currentROpenGLWidget->setNewMax(convertSliderToScale(value));
    }
    else if (this->sender() == ui->sliderLow)
    {
        currentROpenGLWidget->setNewMin(convertSliderToScale(value));
    }
    else
    {
        return;
    }

    if (currentROpenGLWidget->getNewMin() == currentROpenGLWidget->getNewMax())
    {
        return;
    }

    updateCurrentROpenGLWidget();
}


void RMainWindow::gammaScaleImageSlot(int value)
{
    if (currentROpenGLWidget == NULL)
        return;

    if (currentROpenGLWidget->getNewMin() == currentROpenGLWidget->getNewMax())
        return;

    gamma = convertSliderToGamma(value);
    qDebug("RMainWindow::gammaScaleImageSlot() value =%i, gamma = %f", value, gamma);
    currentROpenGLWidget->setGamma(gamma);
    currentROpenGLWidget->update();
}

void RMainWindow::setupSliders(ROpenGLWidget* rOpenGLWidget)
{
    // The slider range of integer values must be consistent with the maximum data range.
    // For USET for example, it is 4096.
    // We also need to define the number of decimals in the spinBox High and Low.
    int decimals = 0;
    ui->sliderHigh->blockSignals(true);
    ui->sliderLow->blockSignals(true);


    if (rOpenGLWidget->getRMatImageList().at(0)->matImage.type() != CV_32F && rOpenGLWidget->getRMatImageList().at(0)->matImage.type() != CV_32FC3)
    {
        sliderRange = (int) rOpenGLWidget->getRMatImageList().at(0)->getDataRange();
        ui->sliderHigh->setRange(1, sliderRange);
        ui->sliderLow->setRange(1, sliderRange);

        sliderScale = 1.0;
        sliderToScaleMinimum = 0;

    }
    else
    {

         /// Here the image is assumed to be seen as scientific data
         /// for which scaling needs to be tightly set around the min and max
         /// so we can scan through with maximum dynamic range.

        sliderRange = 65536;
        ui->sliderHigh->setRange(1, sliderRange);
        ui->sliderLow->setRange(1, sliderRange);

        float dataMax = (float) rOpenGLWidget->getRMatImageList().at(0)->getDataMax();
        float dataMin = (float) rOpenGLWidget->getRMatImageList().at(0)->getDataMin();
        float dataRange = dataMax - dataMin;
        sliderScale = (float) dataRange / sliderRange;
        sliderToScaleMinimum = dataMin;
        qDebug() << "RMainWindow::setupSliders() dataMax =" << dataMax;
        qDebug() << "RMainWindow::setupSliders() dataMin =" << dataMin;
        qDebug() << "RMainWindow::setupSliders() sliderScale =" << sliderScale;
        // update the number of decimals in the spinBox High and Low
        decimals = 2;
    }


    ui->sliderHigh->blockSignals(false);
    ui->sliderLow->blockSignals(false);

    ui->doubleSpinBoxHigh->blockSignals(true);
    ui->doubleSpinBoxLow->blockSignals(true);

    ui->doubleSpinBoxHigh->setMaximum(convertSliderToScale(ui->sliderHigh->maximum()));
    ui->doubleSpinBoxHigh->setMinimum(convertSliderToScale(ui->sliderHigh->minimum()));
    ui->doubleSpinBoxHigh->setDecimals(decimals);
    ui->doubleSpinBoxLow->setMaximum(convertSliderToScale(ui->sliderLow->maximum()));
    ui->doubleSpinBoxLow->setMinimum(convertSliderToScale(ui->sliderLow->minimum()));
    ui->doubleSpinBoxLow->setDecimals(decimals);

    ui->doubleSpinBoxHigh->blockSignals(false);
    ui->doubleSpinBoxLow->blockSignals(false);
}

void RMainWindow::setupSubImage()
{
    subQImage = new QImage(ui->subFrame->getNaxis1(), ui->subFrame->getNaxis2(), QImage::Format_ARGB32);
    subQImage->fill(QColor(125, 125, 125, 125));

    ui->subFrame->setImage(subQImage);
    ui->subFrame->setDrawCross(true);
    ui->subFrame->update();

    //subPaintWidget = new PaintWidget(subQImage, subMagnification);
    //subPaintWidget->setDrawCross(true);


}


void RMainWindow::cannyEdgeDetection()
{
    if (ui->treeWidget->rMatLightList.isEmpty() && ui->treeWidget->getLightUrls().empty())
    {
        ui->statusBar->showMessage(QString("No lights for limb detection"), 3000);
        return;
    }

    QPoint lastPos;
    bool restoreCannyView = false;
    if (cannySubWindow != NULL)
    {
        lastPos = cannySubWindow->pos();
        cannySubWindow->close();
        restoreCannyView = true;
    }

    processing->setShowContours(ui->contoursCheckBox->isChecked());
    processing->setShowLimb(ui->limbCheckBox->isChecked());
    processing->setBlurSigma(ui->blurSpinBox->value());
    processing->setUseHPF(ui->hpfCheckBox->isChecked());
    processing->setHPFSigma(ui->hpfSpinBox->value());


    if (!ui->treeWidget->rMatLightList.isEmpty())
    {
        bool success;
        if (!ui->wernerCheckBox->isChecked())
        {
            success = processing->cannyEdgeDetection(ui->cannySlider->value());
        }
        else
        {
            success = processing->wernerLimbFit();
        }


        if (!success)
        {
            return;
        }
        /// Results need to be displayed as ROpenGLWidget as we will, de facto,
        /// deal with time series
        //createNewImage(processing->getContoursRMatList());

        currentROpenGLWidget = new ROpenGLWidget(processing->getContoursRMatList(), this);
        addImage(currentROpenGLWidget);
        setupSliders(currentROpenGLWidget);

        cannySubWindow = currentSubWindow;
        cannyContoursROpenGLWidget = currentROpenGLWidget;
    }
    else if (!ui->treeWidget->getLightUrls().empty())
    {
        if (!ui->wernerCheckBox->isChecked())
        {
            processing->cannyEdgeDetectionOffScreen(ui->cannySlider->value());
        }
        else
        {
            return;
        }

    }


    if (!restoreCannyView)
    {
        cannySubWindow->show();
    }
    else
    {
        cannySubWindow->move(lastPos);
        cannySubWindow->show();
        ui->sliderFrame->setValue(lastFrameIndex);
    }

    autoScale(currentROpenGLWidget);
    displayPlotWidget(currentROpenGLWidget);
}

void RMainWindow::cannyRegisterSlot()
{
    processing->setShowContours(ui->contoursCheckBox->isChecked());
    processing->setShowLimb(ui->limbCheckBox->isChecked());

    processing->cannyRegisterSeries();

    createNewImage(processing->getLimbFitResultList1());
    //rangeScale();
    autoScale();
}



void RMainWindow::normalizeCurrentSeries()
{
    QList<RMat*> normalizedRMatList = processing->normalizeSeriesByStats(currentROpenGLWidget->getRMatImageList());

    createNewImage(normalizedRMatList);
    autoScale();
}


void RMainWindow::previewMatImageHPFSlot()
{
    if (ui->treeWidget->rMatLightList.empty())
    {
        emit tempMessageSignal(QString("No image"));
        return;
    }

    double sigma = (double) ui->hpfSpinBox->value();
    cv::Mat matImage = ui->treeWidget->rMatLightList.at(ui->sliderFrame->value())->matImage;
    matImageHPFPreview = processing->makeImageHPF(matImage, (double) sigma);

    createNewImage(matImageHPFPreview, false, instruments::generic, QString("High-pass filtered image, sigma = %1").arg(ui->hpfSpinBox->value()));
}

void RMainWindow::showLimbFitStats()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    if (processing->getCircleOutList().empty())
    {
        return;
    }

    QPoint windowPos(1,1);

    bool addWindow = false;

    if (plotSubWindow == NULL)
    {
        plotSubWindow = new QMdiSubWindow();
        addWindow = true;
    }
    else
    {
        // Restore last used position
        windowPos = plotSubWindow->pos();
    }

    // Prepare the plot data

    int nFrames = currentROpenGLWidget->getRMatImageList().size();

    QVector<double> x(nFrames);
    QVector<double> y(nFrames);
    for (int i = 0 ; i < nFrames ; ++i)
    {
        x[i] = i;
        y[i] = processing->getCircleOutList().at(i).r;
    }

    QCustomPlot *limbFitPlot = new QCustomPlot();
    limbFitPlot->addGraph();
    limbFitPlot->plottable(0)->setPen(QPen(QColor(0, 0, 255, 100)));
    limbFitPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle));

    limbFitPlot->graph(0)->setData(x, y);
    limbFitPlot->rescaleAxes();

    limbFitPlot->xAxis->setAutoTickStep(false);
    limbFitPlot->xAxis->setTickStep(1);
    limbFitPlot->xAxis->setRange(limbFitPlot->xAxis->range().lower, limbFitPlot->xAxis->range().upper + 1);
    limbFitPlot->xAxis->setLabel(QString("Image #"));

    limbFitPlot->yAxis->setRange(900, 930);
    limbFitPlot->yAxis->setAutoTickStep(false);
    limbFitPlot->yAxis->setTickStep(5);
    limbFitPlot->yAxis->setSubTickCount(4);
    limbFitPlot->yAxis->setLabel(QString("Fitted limb radius (px)"));

    plotSubWindow->setWidget(limbFitPlot);
    if (addWindow)
    {
        ui->mdiArea->addSubWindow(plotSubWindow);
    }
    plotSubWindow->resize(defaultWindowSize);
    plotSubWindow->move(windowPos);
    plotSubWindow->show();

}

void RMainWindow::setupToneMappingCurve()
{
    plotSubWindow = new QMdiSubWindow();
    ui->mdiArea->addSubWindow(plotSubWindow);


    int nBins = 256;
    QVector<double> x(nBins);
    QVector<double> yRef(nBins);
    QVector<double> y(nBins);
    for (int i = 0 ; i < nBins ; ++i)
    {
        x[i] = i;
        yRef[i] = (double) x.at(i);
        y[i] = (double) x.at(i);

    }


    toneMappingPlot = new QCustomPlot();
    QCPGraph *toneMappingGraphRef = new QCPGraph(toneMappingPlot->xAxis, toneMappingPlot->yAxis);
    toneMappingGraphRef->setData(x, yRef);

    toneMappingGraph = new QCPGraph(toneMappingPlot->xAxis, toneMappingPlot->yAxis);
    toneMappingGraph->setData(x, y);


    toneMappingPlot->addPlottable(toneMappingGraphRef);
    toneMappingPlot->graph(0)->setPen(QPen(QColor(0, 0, 255, 100)));

    toneMappingPlot->addPlottable(toneMappingGraph);
    toneMappingPlot->graph(1)->setPen(QPen(QColor(255, 0, 0, 100)));

    toneMappingPlot->rescaleAxes();

    /// X axis
    toneMappingPlot->xAxis->setRange(0, 255);
    toneMappingPlot->xAxis->setAutoTickStep(false);
    toneMappingPlot->xAxis->setTickStep(25);
    toneMappingPlot->xAxis->setLabel(QString("Intensity"));

    /// Y axis
    toneMappingPlot->yAxis->setRange(0, 255);
    toneMappingPlot->xAxis->setScaleRatio(toneMappingPlot->xAxis, 1.0);
    toneMappingPlot->yAxis->setAutoTickStep(false);
    toneMappingPlot->yAxis->setTickStep(25);
    toneMappingPlot->yAxis->setLabel(QString("Re-scaled intensity"));
    plotSubWindow->resize(defaultWindowSize);

//    else
//    {
//        qDebug("Updating plot");
//        toneMappingPlot->graph(0)->setData(x, y);
//        toneMappingPlot->replot();
//        //toneMappingPlot->update();
//    }

    plotSubWindow->setWidget(toneMappingPlot);
    plotSubWindow->show();

    updateToneMappingSlot();
}

void RMainWindow::updateToneMappingSlot()
{
    iMax = (double) ui->toneMappingSlider1->value();
    lambda = (double) ui->toneMappingSlider2->value();
    mu = (double) ui->toneMappingSlider3->value();

    if (toneMappingGraph == NULL)
    {
        return;
    }

    int nBins = 256;
    double mu2 = pow(mu, 2.0);

    QVector<double> x(nBins);
    QVector<double> y(nBins);

    if (ui->InverseGaussianRButton->isChecked())
    {
        for (int i = 0 ; i < nBins ; ++i)
        {
            x[i] = (double) i;
            y[i] = iMax * sqrt(lambda / (2.0 * 3.1415 * pow(x[i], 3.0))) * exp(-lambda * pow( (x[i] -  mu), 2.0) / (2.0 * mu2 * x[i]) ) + x[i];
        }
        toneMappingPlot->graph(1)->setData(x, y);
        toneMappingPlot->replot();

        applyToneMappingCurve();

    }

}

void RMainWindow::applyToneMappingCurve()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    currentROpenGLWidget->setApplyToneMapping(ui->applyToneMappingCheckBox->isChecked());
    currentROpenGLWidget->setUseInverseGausssian(ui->InverseGaussianRButton->isChecked());
    currentROpenGLWidget->setImax(iMax);
    currentROpenGLWidget->setLambda(lambda);
    currentROpenGLWidget->setMu(mu);
    currentROpenGLWidget->update();
}

void RMainWindow::displayPlotWidget(ROpenGLWidget* rOpenGLWidget)
{
    ui->histoDock->setWidget(rOpenGLWidget->fetchCurrentCustomPlot());
    rOpenGLWidget->updateCustomPlotLineItems();
}

void RMainWindow::addHeaderWidget()
{
    int frameIndex = ui->sliderFrame->value();

    if (currentROpenGLWidget == NULL)
    {
        qDebug("RMainWindow::addHeaderWidget():: currentROpenGLWidget == NULL");
        return;
    }
    else if (currentROpenGLWidget->getRListImageManager()->getUrlList().empty())
    {
      qDebug("RMainWindow::addHeaderWidget():: currentROpenGLWidget->getRListImageManager() empty");
      return;
    }
    else if (currentROpenGLWidget->getRMatImageList().at(frameIndex)->isBayer())
    {
        qDebug("RMainWindow::addHeaderWidget():: current image is Bayer type");
        return;
    }

    qDebug("Showing headerWidget");

    ui->mdiArea->addSubWindow(currentROpenGLWidget->getTableRSubWindow());
    currentROpenGLWidget->getTableRSubWindow()->show();
    currentROpenGLWidget->getTableRSubWindow()->resize(currentROpenGLWidget->getRListImageManager()->getTableWidgetList().at(0)->size());

    //Connect the slider value changed with the header dock change
    //QObject::connect(ui->sliderFrame, SIGNAL(valueChanged(int)), this, SLOT(updateTableWidget(int)));

    connect(ui->sliderFrame, SIGNAL(valueChanged(int)), currentROpenGLWidget, SLOT(setupTableWidget(int)));
    connect(currentROpenGLWidget->getTableRSubWindow(), SIGNAL(gotHidden()), this, SLOT(uncheckActionHeaderState()));
}

void RMainWindow::updateSubFrame(QImage *image, float intensity, int x, int y)
{
    char format = 'f';
    int precision = 0;

    if (std::abs(currentROpenGLWidget->getRMatImageList().at(0)->getDataMax()) <= 255)
    {
        format = 'g';
        precision = 3;
    }

    QString intensityQStr = QString::number(intensity, format, precision);

    ui->subFrame->setImage(image);
    ui->subFrame->setCursorText(intensityQStr);
    ui->subFrame->setImageCoordX(x);
    ui->subFrame->setImageCoordY(y);
    ui->subFrame->update();
}

QString RMainWindow::checkExistingDir()
{
    QString openDir = QString();
    if (!ui->calibrateDirLineEdit->text().isEmpty())
    {
        openDir = ui->calibrateDirLineEdit->text();
    }
    else
    {
        openDir = QDir::homePath();
    }

    return openDir;
}

void RMainWindow::autoScale()
{
    /// This sends new Minimum and Maximum values to the ROpenGLWidget, depending on the instrument.
    /// Then it moves the slider according to these values.
    /// Finally, by moving the sliders, who are connected to updateCurrentROpenGLwidget(), the display
    /// gets updated.

    if (currentROpenGLWidget == NULL)
        return;

    if (currentROpenGLWidget->getRMatImageList().at(0)->getInstrument() == instruments::MAG)
    {
        currentROpenGLWidget->setNewMax(100.0f);
        currentROpenGLWidget->setNewMin(-100.0f);

        sliderValueHigh = convertScaleToSlider(100.0f);
        sliderValueLow = convertScaleToSlider(-100.0f);
    }
    else if (currentROpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8U ||
             currentROpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8UC3)
    {
        currentROpenGLWidget->setNewMax(255);
        currentROpenGLWidget->setNewMin(0);

        sliderValueHigh = convertScaleToSlider(255);
        sliderValueLow = convertScaleToSlider(0);
    }
    else
    {
        currentROpenGLWidget->setNewMax(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityHigh());
        currentROpenGLWidget->setNewMin(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityLow());

        /// Make the slider use the histogram autoscaling values.
        sliderValueHigh = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityHigh());
        sliderValueLow = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(0)->getIntensityLow());
    }

    gamma = 1.0f;
    sliderValueGamma = convertGammaToSlider(gamma);
    // Below, the slider ignore the input if it is equal to the current state, but it prevents from going through
    // scaleImageSlot subsequently. So we have to tickle it a bit.
    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
    ui->sliderGamma->setValue(sliderValueGamma);

    qDebug("RMainWindow::autoScale() sliderValueHigh = %i , sliderValueLow = %i", sliderValueHigh, sliderValueLow);

}

void RMainWindow::autoScale(ROpenGLWidget *rOpenGLWidget)
{
    if (rOpenGLWidget == NULL)
        return;

    if (rOpenGLWidget->getRMatImageList().at(0)->getInstrument() == instruments::MAG)
    {
        rOpenGLWidget->setNewMax(100.0f);
        rOpenGLWidget->setNewMin(-100.0f);

        sliderValueHigh = convertScaleToSlider(rOpenGLWidget->getNewMax());
        sliderValueLow = convertScaleToSlider(rOpenGLWidget->getNewMin());
    }
    else if (rOpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8U ||
             rOpenGLWidget->getRMatImageList().at(0)->matImage.type() == CV_8UC3)
    {
        rOpenGLWidget->setNewMax(255);
        rOpenGLWidget->setNewMin(0);

        sliderValueHigh = convertScaleToSlider(rOpenGLWidget->getNewMax());
        sliderValueLow = convertScaleToSlider(rOpenGLWidget->getNewMin());

    }
    else
    {
        /// Make the slider use the histogram autoscaling values.
        rOpenGLWidget->setNewMax(rOpenGLWidget->getRMatImageList().at(0)->getIntensityHigh());
        rOpenGLWidget->setNewMin(rOpenGLWidget->getRMatImageList().at(0)->getIntensityLow());

        sliderValueHigh = convertScaleToSlider(rOpenGLWidget->getNewMax());
        sliderValueLow = convertScaleToSlider(rOpenGLWidget->getNewMin());

    }

    gamma = 1.0f;
    sliderValueGamma = convertGammaToSlider(gamma);
    // Below, the slider ignore the input if it is equal to the current state, but it prevents from going through
    // scaleImageSlot subsequently. So we have to tickle it a bit.
    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
    ui->sliderGamma->setValue(sliderValueGamma);

    qDebug("RMainWindow::autoScale() sliderValueHigh = %i , sliderValueLow = %i", sliderValueHigh, sliderValueLow);

}

void RMainWindow::minMaxScale()
{
    if (currentROpenGLWidget == NULL)
        return;

    gamma = 1.0f;
    sliderValueGamma = convertGammaToSlider(gamma);

    sliderValueHigh = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMax());
    sliderValueLow = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMin());

    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
    ui->sliderGamma->setValue(sliderValueGamma);

    qDebug("RMainWindow::autoScale() sliderValueHigh = %i , sliderValueLow = %i", sliderValueHigh, sliderValueLow);

}

void RMainWindow::rangeScale()
{

    if (currentROpenGLWidget == NULL)
        return;

    instruments intrument = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getInstrument();
    int dataType = currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->matImage.type();
    if ( intrument == instruments::USET)
    {
        sliderValueHigh = convertScaleToSlider(4095.0);
        sliderValueLow = convertScaleToSlider(0.0);

    }
    else if (dataType == CV_32F || dataType == CV_32FC3)
    {
        sliderValueHigh = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMax());
        sliderValueLow = convertScaleToSlider(currentROpenGLWidget->getRMatImageList().at(ui->sliderFrame->value())->getDataMin());
    }
    else if (dataType == CV_16U || dataType == CV_16UC3)
    {
        sliderValueHigh = convertScaleToSlider(65535.0);
        sliderValueLow = convertScaleToSlider(0.0);

    }
    else if (dataType == CV_8U || dataType == CV_8UC3)
    {
        sliderValueHigh = convertScaleToSlider(255.0);
        sliderValueLow = convertScaleToSlider(0.0);
    }
    else
    {
        qDebug("Unrecognized data type");
        exit(1);
    }

    ui->sliderHigh->setValue(sliderValueHigh-1);
    ui->sliderHigh->setValue(sliderValueHigh);
    ui->sliderLow->setValue(sliderValueLow +1);
    ui->sliderLow->setValue(sliderValueLow);
}

void RMainWindow::updateCurrentROpenGLWidget()
{
    float dataRange = (float) (currentROpenGLWidget->getNewMax() - currentROpenGLWidget->getNewMin()) ;
    qDebug() << "RMainWindow::updateCurrentROpenGLWidget()   dataRange =" << dataRange;
    alpha = 1.0f / dataRange;
    beta = (float) (- currentROpenGLWidget->getNewMin() / dataRange);

    currentROpenGLWidget->setAlpha(alpha);
    currentROpenGLWidget->setBeta(beta);
    currentROpenGLWidget->updateSubQImage();
    currentROpenGLWidget->update();
    currentROpenGLWidget->updateCustomPlotLineItems();
}

void RMainWindow::updateSliderValueSlot()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    if (this->sender() == ui->doubleSpinBoxHigh)
    {
        double valueD = ui->doubleSpinBoxHigh->value();
        sliderValueHigh = convertScaleToSlider((float) valueD);
        ui->sliderHigh->setValue(sliderValueHigh);
    }
    else if (this->sender() == ui->doubleSpinBoxLow)
    {
        double valueD = ui->doubleSpinBoxLow->value();
        sliderValueLow = convertScaleToSlider((float) valueD);
        ui->sliderLow->setValue(sliderValueLow);
    }
    else if (this->sender() == ui->doubleSpinBoxGamma)
    {
        double valueD = ui->doubleSpinBoxGamma->value();
        sliderValueGamma = convertGammaToSlider((float) valueD);
        qDebug("RMainWindow::updateSliderValueSlot() valueD = %f , sliderValueGamma = %i", valueD, sliderValueGamma);
        ui->sliderGamma->setValue(sliderValueGamma);
    }
}

void RMainWindow::updateWB(int value)
{

    double colorFactor = (double) (value * sliderScaleWB);

    if (this->sender() == ui->redSlider)
    {
        ui->redSpinBox->setValue(colorFactor);
        if (currentROpenGLWidget == NULL)
        {
            return;
        }
        currentROpenGLWidget->setWbRed(colorFactor);
    }

    if (this->sender() == ui->greenSlider)
    {
        ui->greenSpinBox->setValue(colorFactor);
        if (currentROpenGLWidget == NULL)
        {
            return;
        }
        currentROpenGLWidget->setWbGreen(colorFactor);
    }

    if (this->sender() == ui->blueSlider)
    {
        ui->blueSpinBox->setValue(colorFactor);
        if (currentROpenGLWidget == NULL)
        {
            return;
        }
        currentROpenGLWidget->setWbBlue(colorFactor);
    }

    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    currentROpenGLWidget->update();
    currentROpenGLWidget->updateSubQImage();
}

int RMainWindow::convertGammaToSlider(float gamma)
{
    int sliderValueGamma = (int) std::round((gamma - gammaMin)/gammaScale + 1.0f);
    qDebug("RMainWindow::convertGammaToSlider()  gammaMin = %f, gammaScale = %f , gamma = %f ,sliderValueGamma = %i", gammaMin, gammaScale, gamma, sliderValueGamma);

    return sliderValueGamma;
}


float RMainWindow::convertSliderToScale(int value)
{

    float scaledValue = 0.0f;

    if (currentROpenGLWidget->getRMatImageList().empty())
    {
        qDebug("currentROpenGLWidget->getRMatImageList() is empty.");
        return (float) scaledValue;
    }

    scaledValue = sliderToScaleMinimum + sliderScale * ((float) (value-1)) ;
    qDebug("RMainWindow::convertSliderToScale:: sliderToScaleMinimum = %f ; sliderScale = %f ; value = %i ; scaledValue = %f", sliderToScaleMinimum, sliderScale, value, scaledValue);
    return scaledValue;
}


int RMainWindow::convertScaleToSlider(float valueF)
{

    int sliderValue = 1;

    if (currentROpenGLWidget->getRMatImageList().empty())
    {
        qDebug("currentROpenGLWidget->getRMatImageList() is empty.");
        return sliderValue;
    }

    // If the slider Value is greater than the range of the slider, it is ignored instead of clipped.
    // So we need to clip to the max value of the slider which is the slider range.
    sliderValue = (int) std::min(std::round((valueF - sliderToScaleMinimum)/sliderScale) + 1, sliderRange);

    return sliderValue;
}

float RMainWindow::convertSliderToGamma(int value)
{
    float valueF = (float) (gammaMin +  gammaScale * (value -1));
    return valueF;
}


void RMainWindow::changeROpenGLWidget(ROpenGLWidget *rOpenGLWidget)
{

    currentROpenGLWidget = rOpenGLWidget;

    // Restore slider values
    qDebug("RMainWindow::changeROpenGLWidget:: rOpenGLWidget->getNewMin() = %f", rOpenGLWidget->getNewMin());
    setupSliders(rOpenGLWidget);
    // Scaling
    qDebug("RMainWindow::changeROpenGLWidget:: rOpenGLWidget->getNewMin() = %f", rOpenGLWidget->getNewMin());
    ui->sliderHigh->setValue(convertScaleToSlider(currentROpenGLWidget->getNewMax()));
    ui->sliderLow->setValue(convertScaleToSlider(currentROpenGLWidget->getNewMin()));

    qDebug("RMainWindow::changeROpenGLWidget:: currentROpenGLWidget->getNewMin() = %f", currentROpenGLWidget->getNewMin());
    qDebug("RMainWindow::changeROpenGLWidget:: rOpenGLWidget->getNewMin() = %f", rOpenGLWidget->getNewMin());

    ui->sliderGamma->setValue(convertGammaToSlider(currentROpenGLWidget->getGamma()));
    // White balance
    ui->redSlider->setValue((int) std::round(currentROpenGLWidget->getWbRed() / sliderScaleWB));
    ui->greenSlider->setValue((int) std::round(currentROpenGLWidget->getWbGreen() / sliderScaleWB));
    ui->blueSlider->setValue((int) std::round(currentROpenGLWidget->getWbBlue() / sliderScaleWB));
    // Time series
    ui->sliderFrame->blockSignals(true);
    ui->sliderFrame->setRange(0, currentROpenGLWidget->getRMatImageList().size()-1);
    ui->sliderFrame->setValue(currentROpenGLWidget->getFrameIndex());
    ui->sliderFrame->blockSignals(false);
    ui->imageLabel->setText(QString::number(currentROpenGLWidget->getFrameIndex()+1) + QString("/") + QString::number(currentROpenGLWidget->getRMatImageList().size()));


    displayPlotWidget(rOpenGLWidget);

    // Refresh state of QAction buttons in toolbar
    if (currentROpenGLWidget->getTableRSubWindow() != NULL && currentROpenGLWidget->getTableRSubWindow()->isVisible())
    {
        ui->actionHeader->setChecked(true);
    }
    else
    {
        ui->actionHeader->setChecked(false);
    }

}

void RMainWindow::updateFrameInSeries(int frameIndex)
{
    /// Updates the display of the currentROpenGLWidget: change current image in the series, subImage, and histogram
    // The sliderValue minimum is 1. The frameIndex minimum is 0;
    ui->imageLabel->setText(QString::number(frameIndex+1) + QString("/") + QString::number(currentROpenGLWidget->getRMatImageList().size()));
    currentROpenGLWidget->changeFrame(frameIndex);
    ui->mdiArea->currentSubWindow()->setWindowTitle(currentROpenGLWidget->getRMatImageList().at(frameIndex)->getImageTitle());
    displayPlotWidget(currentROpenGLWidget);

    lastFrameIndex = frameIndex;

}


void RMainWindow::setupExportCalibrateDir()
{
    QString dir = QFileDialog::getExistingDirectory(
                            0,
                            "Select directory to export Masters",
                            checkExistingDir());

    ui->calibrateDirLineEdit->setText(dir);

    emit tempMessageSignal(QString(" Exporting calibrated files to: ") + dir, 0);
}

void RMainWindow::exportMastersToFits()
{
    processing->exportMastersToFits();
}

void RMainWindow::exportFrames()
{
    if (ui->jpegExportCheckBox->isChecked())
    {
        exportFramesToJpeg();
    }

    if (ui->fitsExportCheckBox->isChecked())
    {
        exportFramesToFits();
    }
}

void RMainWindow::exportFramesToJpeg()
{
    QDir exportDir(ui->exportDirEdit->text());

    QString format("jpg");

    for (int i = 0 ; i < currentROpenGLWidget->getRMatImageList().size() ; i++)
    {
        QString fileName(QString("image_") + QString::number(i) + QString(".") + format);
        QFileInfo fileInfo(exportDir.filePath(fileName));
        QString filePath = processing->setupFileName(fileInfo, format);
        qDebug() << "RMainWindow::exportFrames():: filePath =" << filePath;
        QImage newQImage = currentROpenGLWidget->grabFramebuffer();

        QPainter painter( &newQImage);
        painter.setPen(QColor(255, 255, 255, 255));
        painter.setFont( QFont("Arial", 16) );
        painter.drawText( QPoint(10, 20), QString("USET / ROB ") + currentROpenGLWidget->getRMatImageList().at(i)->getDate_time() + QString(" UT"));
//        painter.drawRect(QRect(QPoint(1,1), QPoint(200, 200)));

//        createNewImage(newQImage);
        newQImage.save(filePath, "JPG", 100);
        ui->sliderFrame->setValue(ui->sliderFrame->value() + 1);

    }
}

void RMainWindow::exportFramesToFits()
{
    QDir exportDir(ui->exportDirEdit->text());
    QString format("fits");

    for (int i = 0 ; i < currentROpenGLWidget->getRMatImageList().size() ; i++)
    {
        QString fileName(QString("image_") + QString::number(i) + QString(".") + format);
        QFileInfo fileInfo(exportDir.filePath(fileName));
        QString filePath = processing->setupFileName(fileInfo, format);
        processing->exportToFits(currentROpenGLWidget->getRMatImageList().at(i), filePath);
    }
}

void RMainWindow::convertTo8Bit()
{
    if (currentROpenGLWidget == NULL)
    {
        return;
    }

    QList<RMat*> rMat8bitList;

    for (int i = 0 ; i < currentROpenGLWidget->getRMatImageList().size() ; ++i)
    {
        cv::Mat mat8bit = currentROpenGLWidget->getRMatImageList().at(i)->matImage.clone();

        mat8bit.convertTo(mat8bit, CV_8U, 256.0f / currentROpenGLWidget->getRMatImageList().at(i)->getDataRange());

        RMat *rMat8bit = new RMat(mat8bit.clone(), false);
        rMat8bit->setImageTitle(QString("8-bit image # %1").arg(i));
        rMat8bit->setDate_time(currentROpenGLWidget->getRMatImageList().at(i)->getDate_time());
        rMat8bitList << rMat8bit;
    }

    createNewImage(rMat8bitList);
}

void RMainWindow::convertToNeg()
{
    int idx = ui->sliderFrame->value();
    cv::Mat matNeg = currentROpenGLWidget->getRMatImageList().at(idx)->matImage.clone();
    matNeg = currentROpenGLWidget->getRMatImageList().at(idx)->getDataMax() - matNeg;

    RMat *negRMat = new RMat(matNeg, false);
    negRMat->setImageTitle(QString("Negative"));
    negRMat->setInstrument(currentROpenGLWidget->getRMatImageList().at(idx)->getInstrument());
    createNewImage(negRMat);
}


void RMainWindow::calibrateOffScreenSlot()
{
    processing->calibrateOffScreen();
}

void RMainWindow::registerSlot()
{
    if (this->sender() != ui->phaseCorrPushB)
    {
        processing->setUseROI(ui->roiRadioButton->isChecked());

        if (ui->limbFitCheckBox->isChecked())
        {
            processing->registerSeriesOnLimbFit();
            createNewImage(processing->getLimbFitResultList2());
            autoScale();
        }
        else
        {
            processing->registerSeries();
            createNewImage(processing->getResultList());
            autoScale();
        }
    }
    else
    {
        processing->registerSeriesByPhaseCorrelation();
        createNewImage(processing->getResultList());
        autoScale();
    }


}


void RMainWindow::stepForward()
{
    int newValue = ui->sliderFrame->value() + 1;
    if (newValue > ui->sliderFrame->maximum())
    {
        newValue = 0;
    }
    ui->sliderFrame->setValue(newValue);
    updateFrameInSeries(newValue);
}

void RMainWindow::stepBackward()
{
    int newValue = ui->sliderFrame->value() - 1;
    if (newValue < ui->sliderFrame->minimum())
    {
        newValue = ui->sliderFrame->maximum();
    }

    ui->sliderFrame->setValue(newValue);
    updateFrameInSeries(newValue);
}

void RMainWindow::playForward()
{
// Need to use QThread for using interface buttons. Otherwise intercept some keyboard key.
    stopButtonStatus = false;
    int fps = ui->spinBoxFps->value();
    ulong delay = (ulong) std::round( 1000.0f / (float) fps );
    while (stopButtonStatus == false)
    {
        qApp->processEvents();
        stepForward();
        QThread::msleep(delay);
        fps = ui->spinBoxFps->value();
        delay = (ulong) std::round( 1000.0f / (float) fps );
    }

}

void RMainWindow::increaseFps()
{
    int fps = ui->spinBoxFps->value();

    if (fps < 5)
    {
        fps = 5;
    }
    else
    {
        fps += 5;
    }

    ui->spinBoxFps->setValue(fps);
}

void RMainWindow::decreaseFps()
{
    int fps = std::max(1, ui->spinBoxFps->value() -5);
    ui->spinBoxFps->setValue(fps);
}

void RMainWindow::stopButtonPressed()
{
    stopButtonStatus = true;
    qDebug("Stop Button pressed");
}


void RMainWindow::radioRMatSlot()
{
    if (currentROpenGLWidget->getRMatImageList().isEmpty())
    {
        qDebug("nothing to radio out");
        return;
    }

    if (this->sender() == ui->lightRButton)
    {
        emit radioLightImages(currentROpenGLWidget->getRMatImageList());
    }

    if (this->sender() == ui->biasRButton)
    {
        double min = 0;
        double max = 0;
        cv::minMaxLoc(currentROpenGLWidget->getRMatImageList().at(0)->matImage, &min, &max);
        qDebug("RMainWindow::radioRMatSlot():: min =%f , max =%f", min, max );
        emit radioBiasImages(currentROpenGLWidget->getRMatImageList());
    }

    if (this->sender() == ui->darkRButton)
    {
        emit radioDarkImages(currentROpenGLWidget->getRMatImageList());
    }

    if (this->sender() == ui->flatRButton)
    {
        emit radioFlatImages(currentROpenGLWidget->getRMatImageList());
    }
}



void RMainWindow::tileView()
{
    ui->mdiArea->setViewMode(QMdiArea::SubWindowView);
    ui->mdiArea->tileSubWindows();
}



void RMainWindow::on_actionHeader_toggled(bool arg1)
{
    if (arg1)
    {
        currentROpenGLWidget->setupTableWidget(ui->sliderFrame->value());
        addHeaderWidget();
    }
    else
    {
        if (currentROpenGLWidget->getTableRSubWindow() != NULL && currentROpenGLWidget->getTableRSubWindow()->isVisible())
        {
            currentROpenGLWidget->getTableRSubWindow()->hide();
        }
    }
}

void RMainWindow::uncheckActionHeaderState()
{
    ui->actionHeader->setChecked(false);
}



void RMainWindow::on_actionTileView_triggered()
{
    ui->mdiArea->tileSubWindows();
}

void RMainWindow::on_actionROISelect_triggered()
{
    selectROI();
}

void RMainWindow::on_actionROIExtract_triggered()
{
    extractNewImageROI();
}
