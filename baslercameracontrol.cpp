#include "baslercameracontrol.h"
#include <QDateTime>

BaslerCameraControl::BaslerCameraControl(QObject *parent) : QObject(parent)
{
    stopped = true;
    m_isOpen = false;
    isConnectedCam = false;
}

BaslerCameraControl::~BaslerCameraControl()
{
    std::cout<<"this is delete m_basler"<<std::endl;
}

void BaslerCameraControl::initSome()
{
    try
    {
        //方案一：
        std::cout<<"SBaslerCameraControl: PylonInitialize initSome"<<std::endl;
        PylonInitialize();
        CTlFactory &TLFactory = CTlFactory::GetInstance();
        ITransportLayer * pTl = TLFactory.CreateTl("BaslerUsb");
        DeviceInfoList_t devices;
        int n = pTl->EnumerateDevices(devices);
        CInstantCameraArray cameraArray(devices.size());
        if (n == 0) {
            std::cout<<"Cannot find Any camera!"<<std::endl;
            return;
        }
        for (uint i = 0; i<cameraArray.GetSize(); i++) {
            cameraArray[i].Attach(TLFactory.CreateDevice(devices[i]));
            Pylon::String_t sn = cameraArray[i].GetDeviceInfo().GetSerialNumber();
            m_cameraSerial = QString::fromStdString(sn.c_str());
            isConnectedCam = true;
            }
    }
    catch(...)
    {
        return ;
    }

    /*
    try
    {
        //方案二：
        std::cout<<"SBaslerCameraControl: PylonInitialize initSome"<<std::endl;
        PylonInitialize();
        //use ID to attach cameras
        CTlFactory &TlFactory = CTlFactory::GetInstance();
        ITransportLayer* pTl = static_cast<Pylon::ITransportLayer*> (TlFactory.CreateTl("BaslerUsb"));
        DeviceInfoList_t devices;
        if ( pTl->EnumerateDevices(devices) > 0 )
        {
            CInstantCamera camera;
            camera.Attach( TlFactory.CreateDevice( devices[0]));
            // If the camera has been found.
            if ( camera.IsPylonDeviceAttached())
            {
                Pylon::String_t cameraSerialNumber = camera.GetDeviceInfo().GetSerialNumber();
                m_cameraSerial = QString::fromStdString(cameraSerialNumber.c_str());
                isConnectedCam = true;
            }
        }else {
            std::cout<<"Cannot find Any camera!"<<std::endl;
            return;
        }
    }
    catch(...)
    {
        return ;
    }
    */

    /*
    try
    {
        //方案三：
        std::cout<<"SBaslerCameraControl: PylonInitialize initSome"<<std::endl;
        PylonInitialize();
        //use ID to attach cameras
        CTlFactory &TlFactory = CTlFactory::GetInstance();
        ITransportLayer* pTl = static_cast<Pylon::ITransportLayer*> (TlFactory.CreateTl("BaslerUsb"));
        DeviceInfoList_t devices;
        TlFactory.EnumerateDevices( devices );
        IPylonDevice * pDevice = pTl->CreateFirstDevice();
        if(!devices.empty())
        {
            CBaslerUsbCamera  camera(pDevice);
            Pylon::String_t cameraSerialNumber = camera.GetDeviceInfo().GetSerialNumber();
            m_cameraSerial = QString::fromStdString(cameraSerialNumber.c_str());
            isConnectedCam = true;
        }else{
            std::cout<< "Cannot find Any camera!"<<std::endl;
            return ;
        }
    }
    catch(...)
    {
        return ;
    }
    */
}

void BaslerCameraControl::deleteAll()
{
    stopped = true;
    if (m_basler.IsOpen()) {
        m_basler.DetachDevice();
        m_basler.Close();
    }
    CloseCamera();
    //关闭库
    m_basler.DestroyDevice();//这个才是真正的强行关闭相机
    std::cout<<"SBaslerCameraControl deleteAll: PylonTerminate"<<std::endl;
    PylonTerminate();
    std::cout<< "SBaslerCameraControl deleteAll: Close"<<std::endl;
}

QString BaslerCameraControl::cameras()
{
    return m_cameraSerial;
}

void BaslerCameraControl::CopyToImage(CGrabResultPtr pInBuffer, QImage &OutImage)
{

    try {

        uchar* buff = (uchar*)pInBuffer->GetBuffer();
        int nHeight = pInBuffer->GetHeight();
        int nWidth = pInBuffer->GetWidth();
        QImage imgBuff(buff, nWidth, nHeight, QImage::Format_Indexed8);
        OutImage = imgBuff;
    }
    catch (GenICam::GenericException &e) {
        qDebug() << "CopyToImage Error:" << QString(e.GetDescription());
    }
}

void BaslerCameraControl::onTimerGrabImage()
{
    while (!stopped)
    {
        QImage image;
        GrabImage(image, 600);

        if (!image.isNull())
        {
            emit sigCurrentImage(image);
        }
    }
}

int BaslerCameraControl::OpenCamera(QString cameraSN)
{
        CDeviceInfo cInfo;
        String_t str = String_t(cameraSN.toStdString().c_str());
        cInfo.SetSerialNumber(str);
        m_basler.Attach(CTlFactory::GetInstance().CreateDevice(cInfo));
        m_basler.Open();
        m_isOpen =  m_basler.IsOpen();

        //获取触发模式
        getFeatureTriggerSourceType();

        shezhicanshu();
        return 0;
}


int BaslerCameraControl::CloseCamera()
{
    if (!m_isOpen) {
        return -1;
    }
    try {
        if (m_basler.IsOpen()) {
            m_basler.DetachDevice();
            m_basler.Close();
        }
    }
    catch (GenICam::GenericException &e) {
        qDebug() << "CloseCamera Error:" << QString(e.GetDescription());
        return -2;
    }
    return 0;
}


QString BaslerCameraControl::getFeatureTriggerSourceType()
{
    INodeMap &cameraNodeMap = m_basler.GetNodeMap();
    CEnumerationPtr  ptrTriggerSel = cameraNodeMap.GetNode("TriggerSelector");
    ptrTriggerSel->FromString("FrameStart");
    CEnumerationPtr  ptrTrigger = cameraNodeMap.GetNode("TriggerMode");
    ptrTrigger->SetIntValue(1);
    CEnumerationPtr  ptrTriggerSource = cameraNodeMap.GetNode("TriggerSource");

    String_t str = ptrTriggerSource->ToString();
    m_currentMode = QString::fromLocal8Bit(str.c_str());
    return m_currentMode;
}

//设置一系列相机参数，例如相机的曝光和外触发的模式
int BaslerCameraControl::shezhicanshu()
{
    INodeMap &cameraNodeMap = m_basler.GetNodeMap();

    CEnumerationPtr  ptrTriggerSel = cameraNodeMap.GetNode("TriggerSelector");
    ptrTriggerSel->FromString("FrameStart");

    CEnumerationPtr  ptrTrigger = cameraNodeMap.GetNode("TriggerMode");
    ptrTrigger->SetIntValue(1);

    CEnumerationPtr  ptrTriggerAcqMode = cameraNodeMap.GetNode("AcquisitionMode");
    ptrTriggerAcqMode->FromString("Continuous");//单帧采集SingleFrame  连续采集  Continuous

    CEnumerationPtr ptrCounterResetActivation = cameraNodeMap.GetNode("TriggerActivation");
    ptrCounterResetActivation->FromString("RisingEdge");//上升沿触发

    CEnumerationPtr  ptrTriggerSource = cameraNodeMap.GetNode("TriggerSource");
    ptrTriggerSource->FromString("Line1");//外触发模式


    CFloatPtr  ptrDebouncerTime = cameraNodeMap.GetNode("LineDebouncerTime");
    ptrDebouncerTime->SetValue(20.0);//滤波

    const CFloatPtr exposureTime = cameraNodeMap.GetNode("ExposureTime");
    exposureTime->SetValue(2300);

    CEnumerationPtr  ptrExposureAuto = cameraNodeMap.GetNode("ExposureAuto");
    ptrExposureAuto->SetIntValue(0);

    String_t str = ptrTriggerSource->ToString();
    m_currentMode = QString::fromLocal8Bit(str.c_str());

    return 0;
}


double BaslerCameraControl::GetCamera(BaslerCameraControl::SBaslerCameraControl_Type index)
{
    INodeMap &cameraNodeMap = m_basler.GetNodeMap();
    switch (index) {
    case Type_Basler_ExposureTimeAbs: {
        const CFloatPtr exposureTime = cameraNodeMap.GetNode("ExposureTime");
        return exposureTime->GetValue();
    } break;
    case Type_Basler_GainRaw: {
        const CIntegerPtr cameraGen = cameraNodeMap.GetNode("GainRaw");
        return cameraGen->GetValue();
    } break;

    case Type_Basler_AcquisitionFrameRateAbs:
    {
        const CBooleanPtr frameRate = cameraNodeMap.GetNode("AcquisitionFrameRateEnable");
        frameRate->SetValue(TRUE);
        const CFloatPtr frameRateABS = cameraNodeMap.GetNode("AcquisitionFrameRateAbs");
        return frameRateABS->GetValue();
    } break;
    case Type_Basler_Width:
    {
        const CIntegerPtr widthPic = cameraNodeMap.GetNode("Width");
        return widthPic->GetValue();
    } break;
    case Type_Basler_Height:
    {
        const CIntegerPtr heightPic = cameraNodeMap.GetNode("Height");
        return heightPic->GetValue();
    } break;
    default:
        return -1;
        break;
    }
}
//开始采集
long BaslerCameraControl::StartAcquire()
{

    m_isOpenAcquire = true;
    try {
        qDebug() << "SBaslerCameraControl StartAcquire" << m_currentMode;

        if (m_currentMode == "Software")
        {
            m_basler.StartGrabbing(GrabStrategy_LatestImageOnly);
            onTimerGrabImage();
        }
        else if (m_currentMode == "Line1")
        {
            m_basler.StartGrabbing(GrabStrategy_OneByOne);
            onTimerGrabImage();
        }
    }
    catch (GenICam::GenericException &e) {
        //        OutputDebugString(L"StartAcquire error:");
        qDebug() << "StartAcquire Error:" << QString(e.GetDescription());
        return -2;
    }
    return 0;
}


long BaslerCameraControl::StopAcquire()
{
    stopped = true;
    m_isOpenAcquire = false;
    std::cout<<"SBaslerCameraControl StopAcquire"<<std::endl;
    try {
        if (m_basler.IsGrabbing())
        {
            m_basler.StopGrabbing();
        }
    }

    catch (GenICam::GenericException &e)
    {
        qDebug() << "StopAcquire Error:" << QString(e.GetDescription());
        return -2;

    }
    return 0;
}

long BaslerCameraControl::GrabImage(QImage &image, int timeout)
{
    try {
        if (!m_basler.IsGrabbing()) {
            StartAcquire();
        }
        CGrabResultPtr ptrGrabResult;
        if (m_currentMode == "Freerun") {
        }
        else if (m_currentMode == "Software") {
            if (m_basler.WaitForFrameTriggerReady(1000, TimeoutHandling_Return)) {
                m_basler.ExecuteSoftwareTrigger();
                m_basler.RetrieveResult(timeout, ptrGrabResult, TimeoutHandling_Return);
            }
        }
        else if (m_currentMode == "Line1") {
            m_basler.RetrieveResult(timeout, ptrGrabResult, TimeoutHandling_Return);
        }
        else if (m_currentMode == "Line2")
        {
            m_basler.RetrieveResult(timeout, ptrGrabResult, TimeoutHandling_Return);
        }

        if (ptrGrabResult == NULL)
        {
            return -5;
        }
        if (ptrGrabResult->GrabSucceeded()) {
            std::cout<<"what: ptrGrabResult GrabSucceeded"<<std::endl;
            if (!ptrGrabResult.IsValid()) { OutputDebugString(L"GrabResult not Valid Error\n"); return -1; }
            EPixelType pixelType = ptrGrabResult->GetPixelType();
            switch (pixelType) {
            case PixelType_Mono8: {
                CopyToImage(ptrGrabResult, image);
            } break;
            case PixelType_BayerRG8: {
                std::cout<<"what: PixelType_BayerRG8"<<std::endl;
            }  break;
            default:
                std::cout<<"what: default"<<std::endl;
                break;
            }
        }
        else {
            std::cout<<"Grab Error!!!"<<std::endl;
            //            OutputDebugString(L"Grab Error!!!");
            return -3;
        }
    }
    catch (GenICam::GenericException &e) {
        //        OutputDebugString(L"GrabImage Error\n");
        qDebug() << "GrabImage Error:" << QString(e.GetDescription());
        //        OutputDebugString((e.GetDescription()));
        return -2;
    }
    catch (...) {
        OutputDebugString(L"ZP 11 Shot GetParam Try 12 No know Error\n");
        return -1;
    }
    return 0;
}
