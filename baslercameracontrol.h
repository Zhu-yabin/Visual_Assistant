#ifndef BASLERCAMERACONTROL_H
#define BASLERCAMERACONTROL_H

#include <QObject>
#include <iostream>
#include <pylon/PylonIncludes.h>
#include <pylon/usb/BaslerUsbCamera.h>
#include <QImage>
#include <QTimer>
#include <QDebug>

#define DOUBLE_MAX_Basler 100000
#define DOUBLE_MIN_Basler 0

using namespace std;
using namespace Pylon;
using namespace GenApi;

class BaslerCameraControl :public QObject
{
    Q_OBJECT
public:
    explicit BaslerCameraControl(QObject *parent = 0);
    ~BaslerCameraControl();
    enum SBaslerCameraControl_Type {
        Type_Basler_Freerun, //设置相机的内触发
        Type_Basler_Line1, //设置相机的外触发
        Type_Basler_ExposureTimeAbs, //设置相机的曝光时间
        Type_Basler_GainRaw, //设置相机的增益
        Type_Basler_AcquisitionFrameRateAbs, //设置相机的频率
        Type_Basler_Width, //图片的宽度
        Type_Basler_Height, //图片的高度
        Type_Basler_LineSource, //灯的触发信号
    };
    bool stopped;
    bool isConnectedCam = false; //相机是否已经初始化
    void initSome();
    void deleteAll();
    QString cameras();
    int OpenCamera(QString cameraSN);
    int CloseCamera();
    int shezhicanshu();//设置参数
    bool get_CamStatus(){ return m_isOpen;}
    QString getFeatureTriggerSourceType(); // 获取种类：软触发、外触发等等
    double GetCamera(BaslerCameraControl::SBaslerCameraControl_Type index); // 获取各种参数

    long GrabImage(QImage& image, int timeout = 5000);
    long StartAcquire(); // 开始采集
    long StopAcquire(); // 结束采集

signals:
    void sigCurrentImage(QImage);

private:
    void CopyToImage(CGrabResultPtr pInBuffer, QImage &OutImage);

private slots:
    void onTimerGrabImage();

private:
    CInstantCamera m_basler;
    QString m_cameraSerial;
    QString m_currentMode;
    bool m_isOpenAcquire = false;   //是否开始采集
    bool m_isOpen = false;   //是否打开摄像头
};

#endif // BASLERCAMERACONTROL_H
