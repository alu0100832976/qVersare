#include "cameradialog.h"
#include "ui_cameradialog.h"
#include <QCamera>
#include <QCameraViewfinder>
#include <QCameraImageCapture>
#include <QVBoxLayout>

cameradialog::cameradialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cameradialog)
{

    ui->setupUi(this);
    QList<QByteArray> devices = QCamera::availableDevices();
    if (!devices.empty()) {
        mCamera_ = new QCamera(devices.at(0));

        mCameraViewfinder_ = new QCameraViewfinder;
        mCameraImageCapture_ = new QCameraImageCapture(mCamera_, this);
        mLayout_ = new QVBoxLayout;

        mCamera_->setViewfinder(mCameraViewfinder_);

        mLayout_->addWidget(mCameraViewfinder_);
        mLayout_->setMargin(0);
        ui->scrollArea->setLayout(mLayout_);
        mCamera_->start();
    } else {
        mCamera_ = nullptr;
    }
}

cameradialog::~cameradialog()
{
    if (mCamera_ != nullptr) {
        mCamera_->stop();
        delete mCamera_;
    }
    delete ui;
}

void cameradialog::setfileName(QString filename)
{
    fileName_ = filename;
}

void cameradialog::on_captPushButton_clicked()
{
    if (mCamera_ != nullptr) {
        mCameraImageCapture_->setCaptureDestination(
                    QCameraImageCapture::CaptureToFile);
        QImageEncoderSettings imageEncoderSettings;
        //imageEncoderSettings.setCodec("image/jpeg");
        //imageEncoderSettings.setResolution(100, 100);
        //mCameraImageCapture->setEncodingSettings(imageEncoderSettings);
        mCamera_->setCaptureMode(QCamera::CaptureStillImage);
        mCamera_->start();
        mCamera_->searchAndLock();
        if(mCameraImageCapture_->isReadyForCapture()) {
            QString aux = "/tmp/cameraCapture.jpg";
            mCameraImageCapture_->capture(aux);
            mCamera_->unlock();
            emit_load_data(aux);

        } else {
            mCamera_->unlock();
            mCamera_->stop();
        }
    }
}
