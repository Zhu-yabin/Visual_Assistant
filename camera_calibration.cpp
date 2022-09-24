#include "camera_calibration.h"
#include "ui_camera_calibration.h"

Camera_Calibration::Camera_Calibration(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Camera_Calibration)
{
    ui->setupUi(this);
    winID = (Hlong) this->ui->label_image->winId();
    initwidget();
}

Camera_Calibration::~Camera_Calibration()
{
    delete ui;
    delete imagebuffer;
    delete Camdo;
}

//函数只有在调试阶段才会被用到调用，该函数的功能如下所示：
//1）需要实例化相机，通过控件进行软触发拍照，并将照片显示在界面，并保存到指定的路径地址下；
//2）通过控件来计算相机的内外参，将计算的内外参导出文件存储
void Camera_Calibration::initwidget()
{
    //实例化imageBuffer
    imagebuffer = new ImageBuffer(50);
    //实例化相机
    Camdo = new GrabThread01(imagebuffer);
    //创建信号与槽函数的连接
    //connect(Camdo, &GrabThread01::showImage1, this, &Camera_Calibration::DisplayImg);
}

//图像采集按键功能说明：
//触发相机采集图像，会跳入图像采集程序中调用ExecuteSoftTrig()函数
void Camera_Calibration::on_pushButton_trigger_clicked()
{
    //图像采集
    on_lineEdit_PicDir_editingFinished();//目的是为CVsave_Pic2Dir函数内的图像保存路径
    ui->pushButton_trigger->setEnabled(false);
    //Camdo->takephoto();
    ui->pushButton_trigger->setEnabled(true);
}

//槽函数：处理采集到的标定图像，获得相机的内参
void Camera_Calibration::on_pushButton_CalibrateInternal_clicked()
{
      DealImg_GetCamParInternal();
      //备注1：弹出一个对话框，告知操作者已完成相机内参的标定
      qDebug() << "123456";
}


//读取图像，解算相机的内外参，该函数的调用条件是在图像采集完后，通过控件进行调用
//传进来的参数包括：路径为lineEdit_DealRestult指定的标定结果存放路径
void Camera_Calibration::DealImg_GetCamParInternal()
{
    // Local iconic variables
    HObject  ho_Image, ho_Caltab;
    // Local control variables
    HTuple  hv_StartCamPar, hv_CalibDataID;
    HTuple  hv_Error, hv_CamParam;

    IntPar_SaveDir = ui->lineEdit_DealRestult->text();
    QDir dir(IntPar_SaveDir);  //设置要遍历的目录,遍历路径为传过来的路径变量IntPar_SaveDir
    //设置文件过滤格式，选择bmp格式文件
    nameFilters << "*.bmp";
    //将过滤后的文件名称存入到files列表中，其目的是获得该路径下的图像个数和图像名称
    files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    //采用QDir类进行图像的删除，需要先遍历指定路径下的图像名称
    //先根据文件名对文件进行删除，然后再将文件夹删除
    //初始化相机内参,这个也要做成接口
    gen_cam_par_area_scan_division(0.025, 0, 0.0000048, 0.0000048, 960, 600, 1920,
          1200, &hv_StartCamPar);
    CreateCalibData("calibration_object", 1, 1, &hv_CalibDataID);
    SetCalibDataCamParam(hv_CalibDataID, 0, HTuple(), hv_StartCamPar);
    //判断Cal_Board值是否为空，如果值为空说明没有选择标定板，给出提示窗口:
    //要求操作人员选择标定图像所使用的标定板
    if(Cal_Board.isEmpty())
    {
        QMessageBox::warning(this,"警告","请选择所使用的标定板!");
        return;
    }
    SetCalibDataCalibObject(hv_CalibDataID, 0, Cal_Board.toStdString().c_str());
    for (int i = 0; i < files.count(); i++)
    {
        QString Pic_name = QString("%1/%2").arg(IntPar_SaveDir).arg(files.at(i));
        ReadImage(&ho_Image, Pic_name.toStdString().c_str());
        FindCalibObject(ho_Image, hv_CalibDataID, 0, 0, i, HTuple(), HTuple());
        GetCalibDataObservContours(&ho_Caltab, hv_CalibDataID, "caltab", 0, 0, i);
    }

    CalibrateCameras(hv_CalibDataID, &hv_Error);//反投影的均方根误差(RMSE),参考值为0.1像素
    ui->lineEdit_CalibrateError->setText(QString("%1").arg(hv_Error.D()));
    GetCalibData(hv_CalibDataID, "camera", 0, "params", &hv_CamParam);
    //指定相机内参文件到保存路径
    IntPar_SaveDir = ui->lineEdit_DealRestult->text();
    QDir PicDir(IntPar_SaveDir);
    if (!PicDir.exists()) {
        PicDir.mkdir(IntPar_SaveDir);
    }
    IntPar_Name= QString("%1/Cam_Int_Parameters.dat").arg(IntPar_SaveDir);
    WriteCamPar(hv_CamParam, IntPar_Name.toStdString().c_str());
    ui->lineEdit_CamInternalParamter->setText("Cam_Int_Parameters.dat");
    ClearCalibData(hv_CalibDataID);
}

//显示图像
void Camera_Calibration::DisplayImg(QImage img)
{
    ui->label_image->clear();
    img = img.scaled(ui->label_image->width(), ui->label_image->height());
    ui->label_image->setPixmap(QPixmap::fromImage(img));
    //图像显示后，将图像采集按键释放
    ui->pushButton_trigger->setEnabled(true);
    //如果标定外参的按键没有按下，采集到的图像都会自动存储在内参标定的文件夹中；
    //标定外参的按键被按下后，采集到的图像才会被转存到外参标定的文件夹中；
    //由外参到内参路径由两种方式：1）退出程序；2）触发【图像清空】按键
    if(b_ExPar_state){
        Pic_SaveDir = ExtPar_SaveDir;
    }else {
        Pic_SaveDir = IntPar_SaveDir;
    }
    CVsave_Pic2Dir(img);
}


//该函数的功能是用实现对采集获得的标定图像保存
//默认图像保存路径为："E:/Image/"，在界面上指定保存路径，如果路径的文件夹真实存在，可以进行存储，反之, 通过mkdir()创建该路径。
//照片的保存格式为"Calib_20211125_0"
void Camera_Calibration::CVsave_Pic2Dir(QImage img)
{
    //判断Pic_SaveDir是否存在，如果存在不做判断，如果不存在就需要mkdir(QString)
    QDir PicDir(Pic_SaveDir);
    if(!PicDir.exists()){
        PicDir.mkdir(Pic_SaveDir);
    }
    cv::Mat Mat_image = QImage2cvMat(img);//将Qimage转化为OpenCV格式
    QDateTime time = QDateTime::currentDateTime();
    QString sdt = time.toString("yyyyMMdd");  //这里"yyyyMMddhhmmss"月份MM要大写
    QString file = QString("%1/Calib_%2_%3.bmp").arg(Pic_SaveDir).arg(sdt).arg(pic2Time);
    Pic_AbsPath = file.toStdString();
    cv::imwrite(Pic_AbsPath, Mat_image);
    pic2Time++;
}


//将Qimage格式图片转化成OpenCV格式
cv::Mat Camera_Calibration::QImage2cvMat(QImage image)
{
    cv::Mat mat;
    mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
    return mat;
}


//Halcon算子
void Camera_Calibration::gen_cam_par_area_scan_division (HTuple hv_Focus, HTuple hv_Kappa, HTuple hv_Sx,
    HTuple hv_Sy, HTuple hv_Cx, HTuple hv_Cy, HTuple hv_ImageWidth, HTuple hv_ImageHeight,
    HTuple *hv_CameraParam)
{
    // Local iconic variables
    //Generate a camera parameter tuple for an area scan camera
    //with distortions modeled by the division model.
    (*hv_CameraParam).Clear();
    (*hv_CameraParam)[0] = "area_scan_division";
    (*hv_CameraParam).Append(hv_Focus);
    (*hv_CameraParam).Append(hv_Kappa);
    (*hv_CameraParam).Append(hv_Sx);
    (*hv_CameraParam).Append(hv_Sy);
    (*hv_CameraParam).Append(hv_Cx);
    (*hv_CameraParam).Append(hv_Cy);
    (*hv_CameraParam).Append(hv_ImageWidth);
    (*hv_CameraParam).Append(hv_ImageHeight);
    return;
}


//图像路径后的浏览（...）按键功能说明：
//选择路径存放的地址，并将存放地址路径显示在LineEdite_PicDir中,默认保存路径为"E:/Image/"
void Camera_Calibration::on_pushButton_Browse_clicked()
{
    Assign_PicDir();
}


//该函数被槽函数on_pushButton_Browse_clicked调用，其目的是指定标定图像存放的路径，并将路径显示在界面上
void Camera_Calibration::Assign_PicDir()
{
    cal_SaveDir =QFileDialog::getExistingDirectory(this,"选择标定图像存放路径...","E:/Image/");
    ui->lineEdit_PicDir->setText(cal_SaveDir);
    IntPar_SaveDir = cal_SaveDir + "/Camera_Internal_Parameters";//相机内参存放路径，"E:/Image/Camera_Internal_Parameters"
    ExtPar_SaveDir = cal_SaveDir+"/Camera_External_Parameters";//相机外参存放路径，"E:/Image/Camera_External_Parameters"
    ui->lineEdit_DealRestult->setText(IntPar_SaveDir);//将相机内参存放路径显示在lineEdit_DealRestult上；
}


//图像清空按键功能说明：
//用来清空LineEdit_PicDir所到指定路径下的标定图像，便于重新采集标定图像
void Camera_Calibration::on_pushButton_PicClear_clicked()
{
    Picture_Clear();
}


//该函数被槽函数on_pushButton_PicClear_clicked调用，其目的是清空LineEdit_PicDir所到指定路径下的图像文件和文件夹
void Camera_Calibration::Picture_Clear()
{
    IntPar_SaveDir = ui->lineEdit_DealRestult->text();
    //设置要遍历的目录
    QDir dir(IntPar_SaveDir);
    //设置文件过滤格式，选择bmp格式文件
    nameFilters << "*.bmp";
    //将过滤后的文件名称存入到files列表中
    files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    //采用QDir类进行图像的删除，需要先遍历指定路径下的图像名称
    //先根据文件名对文件进行删除，然后再将文件夹删除
    for (int i = 0; i < files.count(); i++) {
        dir.remove(files.at(i));//删除文件
    }
    dir.rmdir(IntPar_SaveDir);//删除文件夹，只能删除空文件夹
    pic2Time = 1;
    b_ExPar_state = false;//切换图像保存路径（切换后图像会自动保存到内参文件夹下）
}

//标定内参->标定结果后的浏览(...)按键功能说明：
//其目的是指定相机内、外参存放路径，并将存放地址路径显示在lineEdit_DealRestult中,默认保存路径为"E:/Image/Camera_Internal_Parameters"
void Camera_Calibration::on_pushButton_BroRestult_clicked()
{
    Assign_DealDir();
}

//该函数被槽函数on_pushButton_BroRestult_clicked调用，其目的是指定标定图像存放的路径，并将路径显示在界面上
void Camera_Calibration::Assign_DealDir()
{
    cal_SaveDir = QFileDialog::getExistingDirectory(this,"选择相机内参的标定结果存放的路径...","E:/Image/");
    IntPar_SaveDir = cal_SaveDir + "/Camera_Internal_Parameters";//相机内参存放路径，"E:/Image/Camera_Internal_Parameters"
    ExtPar_SaveDir = cal_SaveDir+"/Camera_External_Parameters";//相机外参存放路径，"E:/Image/Camera_External_Parameters"
    ui->lineEdit_DealRestult->setText(IntPar_SaveDir);//将相机内参存放路径显示在lineEdit_DealRestult上；
}


//当lineEdit_PicDir完成编辑的时候，会读取当前文本，并对lineEdit_DealRestult进行复制
void Camera_Calibration::on_lineEdit_PicDir_editingFinished()
{
    cal_SaveDir = ui->lineEdit_PicDir->text();
    IntPar_SaveDir = cal_SaveDir + "/Camera_Internal_Parameters";//相机内参存放路径
    ExtPar_SaveDir = cal_SaveDir+"/Camera_External_Parameters";//相机外参存放路径
    ui->lineEdit_DealRestult->setText(IntPar_SaveDir);//将相机内参存放路径显示在lineEdit_DealRestult上；
}


//标定板路径后的浏览（...）按键功能说明：
//选择相机标定所使用的标定板，将选择的标定板文件名称显示在lineEdit_CalibrateBoard控件上
void Camera_Calibration::on_pushButton_BroCalibrateBoard_clicked()
{
    ChooseCalibrateBoard();
}


//该函数被槽函数on_pushButton_BroCalibrateBoard_clicked调用，其目的是选择标定相机内外参所使用的的标定板，并将选择的标定板名称显示在界面上
//标定板文件暂时放置在项目程序内("./")，后期在做发布版的时候，需要设置为资源文件
void Camera_Calibration::ChooseCalibrateBoard()
{
    //Cal_Board = QFileDialog::getExistingDirectory(this,"选择标定相机内外参所使用的标定板...","./");
    //第一个参数：this
    //第二个参数：tr() 对话框名称
    Cal_Board = QFileDialog::getOpenFileName(this,tr("选择标定相机内外参所使用的标定板..."),".",tr("image files(*.*)"));
    QFileInfo fileInfo(Cal_Board);
    ui->lineEdit_CalibrateBoard->setText(fileInfo.fileName());//返回去除路径的文件名
}



//槽函数：处理采集到的标定图像，获得相机的外参
void Camera_Calibration::on_pushButton_CalibrateExternal_clicked()
{
    DealImg_GetCamParExternal();
}


//槽函数：处理图像，获得相机外参

//功能1）要给出提示信息，提示操作人员，将标定板放置在测量位置；
//功能2）解算相机的外参
void Camera_Calibration::DealImg_GetCamParExternal()
{
    //功能1：选择标定外参的标定，实际上是在相机内参的基础上，采集一张照片进行标定
    if(!b_ExPar_state)
    {
        QMessageBox::warning(this,"提示","为精确标定相机外参，请操作人员将标定板放置在测量位置!");
        b_ExPar_state = true;
        return;
    }

    // Local iconic variables
      HObject  ho_Image, ho_Caltab;

      // Local control variables
      HTuple  hv_WindowHandle, hv_CamParam;
      HTuple  hv_CalibDataID, hv_RCoord, hv_CCoord, hv_Index;
      HTuple  hv_PoseForCalibrationPlate_position_one;

      SetWindowAttr("background_color","black");
      OpenWindow(0,0,1920/3,1200/3,winID,"visible","",&hv_WindowHandle);
      HDevWindowStack::Push(hv_WindowHandle);
      //判断指定相机内参的文件路径是否为空，如果值为空说明没有指定相机内参文件，给出提示窗口:请指定相机内参文件
      //要求操作人员指向相机内参文件
      if(IntPar_Name.isEmpty())
      {
          QMessageBox::warning(this,"警告","请指定相机内参文件!");
          return;
      }
      ReadCamPar(IntPar_Name.toStdString().c_str(), &hv_CamParam);//读取内参
      //判断标定相机外参所使用的图像是否为空，如果值为空说明没有制定图像路径，给出提示窗口:标定外参所使用的图像路径为NULL
      //要求操作人员指向相机内参文件
      Pic_AbsPath = "E:/Image/100";
     QString  temp = QString::fromStdString(Pic_AbsPath);
      if(temp.isEmpty())
      {
          QMessageBox::warning(this,"警告","标定外参所使用的图像路径为NULL!");
          return;
      }
      ReadImage(&ho_Image, Pic_AbsPath.c_str());
      //判断Cal_Board值是否为空，如果值为空说明没有选择标定板，给出提示窗口:
      //要求操作人员选择标定图像所使用的标定板
      if(Cal_Board.isEmpty())
      {
          QMessageBox::warning(this,"警告","请指定标定所使用的标定板!");
          return;
      }
      CreateCalibData("calibration_object", 1, 1, &hv_CalibDataID);
      SetCalibDataCamParam(hv_CalibDataID, 0, HTuple(), hv_CamParam);
      SetCalibDataCalibObject(hv_CalibDataID, 0, Cal_Board.toStdString().c_str());
      FindCalibObject(ho_Image, hv_CalibDataID, 0, 0, 1, HTuple(), HTuple());
      GetCalibDataObservContours(&ho_Caltab, hv_CalibDataID, "caltab", 0, 0, 1);
      GetCalibDataObservPoints(hv_CalibDataID, 0, 0, 1, &hv_RCoord, &hv_CCoord, &hv_Index,
          &hv_PoseForCalibrationPlate_position_one);
      DispCaltab(hv_WindowHandle, Cal_Board.toStdString().c_str(), hv_CamParam, hv_PoseForCalibrationPlate_position_one,
          1);
      DispCircle(hv_WindowHandle, hv_RCoord, hv_CCoord, HTuple(hv_RCoord.TupleLength(),1.5));
      SetOriginPose(hv_PoseForCalibrationPlate_position_one, 0, 0, 0.00176, &hv_PoseForCalibrationPlate_position_one);
      //
      Cam_Paramter = QString("%1/CaliRestult.ccd").arg(ExtPar_SaveDir);
      QDir PicDir(ExtPar_SaveDir);
      if (!PicDir.exists()) {
          PicDir.mkdir(ExtPar_SaveDir);
      }
      WriteCalibData(hv_CalibDataID, Cam_Paramter.toStdString().c_str());//保存标定结果句柄到外参，保存名称为CaliRestult.ccd

      QString  Temp_list =QString("        相机外参标定数据\nX平移:      %1\nY平移:      %2\nZ平移:      %3\nX轴旋转:  %4\nY轴旋转:  %5\nZ轴旋转:  %6")
              .arg(hv_PoseForCalibrationPlate_position_one[0].D())
              .arg(hv_PoseForCalibrationPlate_position_one[1].D())
              .arg(hv_PoseForCalibrationPlate_position_one[2].D())
              .arg(hv_PoseForCalibrationPlate_position_one[3].D())
              .arg(hv_PoseForCalibrationPlate_position_one[4].D())
              .arg(hv_PoseForCalibrationPlate_position_one[5].D());
      QMessageBox::information(this,"标定外参消息框",Temp_list,QMessageBox::Ok,QMessageBox::NoButton);

}


//内参路径后的浏览（...）按键功能说明：
//当相机相对拍摄位置发生改变时，需要重新标定相机的外参，
//因为相机的焦距等参数并没有发生改变，可以直接调用标定好的内参
void Camera_Calibration::on_pushButton_BroInternalParamter_clicked()
{
    ChooseCamInParamter();
}


//该函数被槽函数on_pushButton_BroExternalParamter_clicked调用，其目的是在计算相机外参时，
//如果不需要进行外参标定可以直接选择相机内参文件
void Camera_Calibration::ChooseCamInParamter()
{
    Cam_Internal_Paramter = QFileDialog::getOpenFileName(this,tr("选择标定相机内参文件（ag.Cam_Int_Parameters.dat）..."),".",tr("image files(*.*)"));
    QFileInfo fileInfo(Cam_Internal_Paramter);
    IntPar_Name = fileInfo.absoluteFilePath();
    ui->lineEdit_CamInternalParamter->setText(fileInfo.fileName());//返回去除路径的文件名
    IntPar_SaveDir = fileInfo.path();//返回不包含文件名的路径
    QStringList list = IntPar_SaveDir.split("/");
    cal_SaveDir = list.at(0);   //cal_SaveDir表示IntPar_SaveDir的上一级目录路径
    for (int i = 1; i < list.count()-1; i++) {
       QString  Temp_list= list.at(i);
       cal_SaveDir = QString("%1/%2").arg(cal_SaveDir).arg(Temp_list);
    }
    ExtPar_SaveDir = cal_SaveDir+"/Camera_External_Parameters";//设置相机外参存放路径
}

void Camera_Calibration::on_pushButton_quit_clicked()
{
    emit return2FS();
}
