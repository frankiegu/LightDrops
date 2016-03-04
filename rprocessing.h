#ifndef RPROCESSING_H
#define RPROCESSING_H

#include <QApplication>
#include <QtCore>

#include "rmat.h"
#include "rlistimagemanager.h"
#include "rtreewidget.h"
#include "rlineedit.h"

class RProcessing: public QObject
{
    Q_OBJECT

public:
    RProcessing(QObject *parent = 0);


    void loadRMatLightList(QList<QUrl> urls);
    void loadRMatBiasList(QList<QUrl> urls);
    void loadRMatDarkList(QList<QUrl> urls);
    void loadRMatFlatList(QList<QUrl> urls);

    // Calibration (bias, dark, ...)
    void makeMasters();
    bool makeMasterBias();
    bool makeMasterDark();
    bool makeMasterFlat();

    RMat average(QList<RMat> rMatList);

    // export methods
    void exportMastersToFits();
    void exportToFits(RMat rImage, QString QStrFilename);
    void loadMasterDark();
    void loadMasterFlat();

    // setters
    void setTreeWidget(RTreeWidget *treeWidget);

    // getters
    QString getExportMastersDir();
    QString getExportCalibrateDir();

    // public properties (for "easier" referencing)
    QList<RMat> rMatLightList;
    QList<RMat> rMatBiasList;
    QList<RMat> rMatDarkList;
    QList<RMat> rMatFlatList;

    RMat masterBias;
    RMat masterDark;
    RMat masterFlat;

signals:

   void resultSignal(RMat &rMatResult, QString windowTitle = QString("Result"));
   void listResultSignal(QList<RMat> &rMatListResult);
   void messageSignal(QString message);
   void tempMessageSignal(QString message, int = 3000);

public slots:
   // The following are setters and slots as well.
   // They are used as slots by the RMainWindow for sending the
   // urls from the treeWidget and a drag and drop of files.

   void createMasters();

   // Preprocessing
   void calibrateOffScreen();
   //void calibrateOnScreen();

   void setExportMastersDir(QString dir);
   void setExportCalibrateDir(QString dir);


private:

    RMat normalizeByMean(RMat rImage);
    RTreeWidget *treeWidget;

    QString exportMastersDir;
    QString exportCalibrateDir;

    QString masterBiasPath, masterDarkPath, masterFlatPath;

    bool biasSuccess, darkSuccess, flatSuccess;


};

#endif // RPROCESSING_H