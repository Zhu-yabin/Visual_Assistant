#ifndef CAMERA_CALIBRATION_H
#define CAMERA_CALIBRATION_H

#include <QWidget>
#include <iostream>
#include <grabthread01.h>
#include <imagebuffer.h>
#include <QDebug>
#include "HalconCpp.h"
#include "HDevThread.h"
#include <opencv2/opencv.hpp>
#include <QMessageBox>

using namespace HalconCpp;
using namespace cv;
using namespace std;

namespace Ui {
class Camera_Calibration;
}

class Camera_Calibration : public QWidget
{
    Q_OBJECT

public:
    explicit Camera_Calibration(QWidget *parent = nullptr);
    ~Camera_Calibration();

private:
    Ui::Camera_Calibration *ui;
    ImageBuffer *imagebuffer;//实例化对象
    GrabThread01 *Camdo;//实例化对象
    void initwidget();//构造函数初始化
    void DisplayImg(QImage);//显示图像到界面
    void CVsave_Pic2Dir(QImage);//图片以OpenCV格式进行存储
    void DealImg_GetCamParInternal();//处理采集到的标定图像，获得相机内参
    void DealImg_GetCamParExternal();//处理采集到的标定图像，获得相机外参
    cv::Mat QImage2cvMat(QImage);//将QImage格式的图像转化为OpenCV格式图像
    void Assign_PicDir();//指定标定图像保存的文件夹
    void Assign_DealDir();//该函数可以单独指定标定相机内外参保存路径
    void Picture_Clear();//清除采集到的标定图像
    void ChooseCalibrateBoard();//在计算相机内参时，需要选择使用的标定板
    void ChooseCamInParamter();//在计算相机外参时，选择相机内参文件

    //Halcon算子中调用的参函数
    void gen_cam_par_area_scan_division (HTuple hv_Focus, HTuple hv_Kappa, HTuple hv_Sx,
       HTuple hv_Sy, HTuple hv_Cx, HTuple hv_Cy, HTuple hv_ImageWidth, HTuple hv_ImageHeight,
       HTuple *hv_CameraParam);


    //私有变量
    int pic2Time=1;
    Hlong winID;
    QString cal_SaveDir;            //指定标定数据存放根路径
    QString Pic_SaveDir;            //指定标定图像存放路径
    String Pic_AbsPath;            //带有文件名的绝对文件路径
    QString IntPar_SaveDir;         //相机标定内参存放路径
    QString ExtPar_SaveDir;         //相机标定外参存放路径
    QString IntPar_Name;            //相机内参文件名称
    QString ExtPar_Name;            //相机外参文件名称
    QStringList nameFilters;        //设置文件过滤器
    QStringList files;              //存放标定图像名称的字符串列表
    QString Cal_Board;              //标定板绝对路径(文件路径包含文件名)
    QString Cam_Internal_Paramter;  //相机内参文件路径(文件路径包含文件名)
    QString Cam_Paramter;           //相机参数文件路径(文件路径包含文件名)
    bool b_ExPar_state=false;

signals:
    void return2FS();

private slots:
    void on_pushButton_trigger_clicked();//通过按键触发相机拍照

    void on_pushButton_CalibrateInternal_clicked();

    void on_pushButton_Browse_clicked();

    void on_pushButton_PicClear_clicked();

    void on_pushButton_BroRestult_clicked();

    void on_lineEdit_PicDir_editingFinished();

    void on_pushButton_BroCalibrateBoard_clicked();

    void on_pushButton_CalibrateExternal_clicked();

    void on_pushButton_BroInternalParamter_clicked();

    void on_pushButton_quit_clicked();
};

#endif // CAMERA_CALIBRATION_H
