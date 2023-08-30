#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QTextBlock>
#include <QMessageBox>
#include <QDateTime>
#include <QtPrintSupport/qtprintsupportglobal.h>
//#include <QPrintDialog>
//#include <QPrinter>
#include <QMetaType>
#include <QTextCodec>
#include <QLabel>
#include <cmath>
#include <QDesktopServices>
#include <QDebug>
//#include <QIcon>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "rvlink_ftd2xx.h"
#include "flashrom.h"
// for debug
#include <vector>
#include <iostream>
#include <map>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
//    , m_console(new Console)
{
    ui->setupUi(this);

    write_buffer = new char[bufsize];
    read_buffer  = new char[bufsize];
//    cmd_buffer   = new char[1000];

    flashCtl.writeBufferAddr = (ULONGLONG*)write_buffer;
    flashCtl.readBufferAddr  = (ULONGLONG*)read_buffer;

    my_thread = new FlashThread(this);

    stimer = new QTimer(this);

    mstimer = new QTimer(this);

    timeDataLable = new QLabel(this);

    statusLabel = new QLabel(this);

    timerElapsedLable = new QLabel(this);

#ifdef _WIN32
    m_usb_listener = new usb_listener();
    qApp->installNativeEventFilter(m_usb_listener);
#endif

    ui->comboBoxPort->installEventFilter(this);

    qRegisterMetaType<uint64_t>("uint64_t");

    //initial default flash address
    //connect(ui->lineEditAddress, &QLineEdit::editingFinished, this, &MainWindow::on_lineEditAddress_editingFinished);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::open);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveas);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveas);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::help);

    connect(ui->actionConfig, &QAction::triggered, this, &MainWindow::config);

    connect(my_thread, &FlashThread::progress, this, &MainWindow::updateProgress);

    connect(stimer,SIGNAL(timeout()),this,SLOT(timerUpdate()));

    connect(mstimer,SIGNAL(timeout()),this,SLOT(timerElapsed()));

    connect(ui->console, &Console::getData, this, &MainWindow::consoleDebug);

#ifdef _WIN32
    connect(m_usb_listener, &usb_listener::DevicePlugOut, [=](){
        //do something...
        if(~ft_list_device) {
            cmd_debug = false;
            ui->console->LOGI("Device Plug Out!\n>>");
        }
//        qDebug("do something...");

    });
#endif
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8")); //set codec

    RvFlashInit();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete my_thread;
    delete stimer;
    delete timeDataLable;
    delete statusLabel;
#ifdef _WIN32
    delete m_usb_listener;
#endif
    delete write_buffer;
    delete read_buffer;
//    delete cmd_buffer;
}

void MainWindow::on_pushButtonRun_clicked()
{
    RvFlashCurtCfg();
    flashCtl.runFlag = true;
    if(false == checkStatus() || false == checkConnect() || false == checkCurrentFile())
    {
        flashctl_reset();
        return;
    }
    my_thread->start();
}

//void MainWindow::on_pushButtonConnect_clicked()
//{
//    if(false == checkConnect())
//    {
//        return;
//    }

//    ft_open();
//    process_reset();
//    ft_close();

////    QMessageBox::information(NULL,QStringLiteral("提示！"),QStringLiteral("连接成功！"),QStringLiteral("确定"));
//    QMessageBox::information(NULL,QStringLiteral("Notice！"),QStringLiteral("Connect succeeded！"),QStringLiteral("Confirm"));
//}

void MainWindow::on_pushButtonRead_clicked()
{
    RvFlashReadCfg();
    flashCtl.readFlag = true;
    if(false == checkStatus())
    {
        return;
    }

    if(false == checkConnect())
    {
        return;
    }

    //ULONGLONG dwLegth = filesize64;
    ULONGLONG dwLegth = ceil((float)mtplenth / 8);
    flashCtl.readFlag = true;
    ui->progressBar->setRange(0, dwLegth);
    my_thread->start();
}

//void MainWindow::on_pushButtonRead_clicked()
//{
//    ULONGLONG data;
//    ULONGLONG addr = flashCtl.address;
//    ft_dev_init(ft_freq);
////    ReadDataAndCheckACK(addr, &data, 40, 64);
//    rv_read(addr, &data);
//    ft_close();
//    ui->console->LOGI( "read address=0x%x, read data=0x%llx\n", addr, data);
//}

void MainWindow::on_pushButtonErase_clicked()
{
    RvFlashCurtCfg();
    flashCtl.eraseFlag = true;
    if(false == checkStatus() || false == checkConnect() || false == checkCurrentFile())
    {
        flashctl_reset();
        return;
    }

    ULONGLONG dwLegth = filesize64;
    ui->progressBar->setRange(0, dwLegth);
    my_thread->start();
}

void MainWindow::on_pushButtonProgram_clicked()
{
    RvFlashCurtCfg();
    flashCtl.writeFlag = true;
    if(false == checkStatus() || false == checkConnect() || false == checkCurrentFile())
    {
        flashctl_reset();
        return;
    }

    ULONGLONG dwLegth = filesize64;
    ui->progressBar->setRange(0, dwLegth);
    my_thread->start();
}

void MainWindow::on_pushButtonVerify_clicked()
{
    RvFlashReadCfg();
    flashCtl.verifyFlag = true;
    if(false == checkStatus() || false == checkConnect() || false == checkCurrentFile())
    {
        flashctl_reset();
        return;
    }

    ULONGLONG dwLegth = filesize64;
    ui->progressBar->setRange(0, dwLegth);
    my_thread->start();
}

void MainWindow::on_pushButtonAuto_clicked()
{
    RvFlashCurtCfg();
    flashCtl.autoFlag = true;
    if(false == checkStatus() || false == checkConnect() || false == checkCurrentFile())
    {
        flashctl_reset();
        return;
    }

    ULONGLONG dwLegth = filesize64;
    ui->progressBar->setRange(0, dwLegth);
    my_thread->start();
}

void MainWindow::on_pushButtonOpen_clicked()
{
    MainWindow::open();
}

void MainWindow::on_pushButtonSave_clicked()
{
    MainWindow::saveas();
}

void MainWindow::open()
{
    uint64_t display_size;
    QString fileName, filePath;
    QFileInfo fileInfo;

    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
    if (currentFile.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(this, "Open the file", qApp->applicationDirPath(), \
                                                    QString::fromLocal8Bit("bin File(*.bin);;hex File(*.hex);;elf File(*.elf);;All File(*)"));
    }
    else {
        //get filepath
        fileInfo = QFileInfo(currentFile);
        filePath = fileInfo.path();
        //ui->console->LOGI("filePath=%s\n", filePath.toLocal8Bit().data());
        //get new filename
        fileName = QFileDialog::getOpenFileName(this, "Open the file", filePath, \
                                                QString::fromLocal8Bit("bin File(*.bin);;hex File(*.hex);;elf File(*.elf);;All File(*)"));
    }
    //update currentFile parameter
    if(!fileName.isEmpty())
    {
        currentFile = fileName;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        //QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }

    // convert fileName to char, save it to flashCtl.fileName
    QByteArray filename = fileName.toLocal8Bit();
    flashCtl.fileName = filename.data();
    flashCtl.fileSize = file.size();//get file size/lenth
    flashCtl.currentfileSize = file.size();//backup file.size

    //filesize up to mtpsize
    if(flashCtl.fileSize > mtpsize)
    {
//        QMessageBox::information(NULL,QStringLiteral("通知！"),QStringLiteral("仅显示前64KB数据！"),QStringLiteral("确定"));
//        QMessageBox::information(NULL,QStringLiteral("Notice！"),QStringLiteral("Only display 64KB data！"),QStringLiteral("Confirm"));
        ui->console->LOGI("Notice！Only display 64KB data！\n");
        display_size = mtpsize;
    } else
    {
        display_size = flashCtl.fileSize;
    }

    float lenth = (float)flashCtl.fileSize / 1024;
    ui->console->LOGI("fileName=%s\nfileSize=%.1fK (%dBYTES)\n", flashCtl.fileName, lenth, flashCtl.fileSize);

    //read data from file
    QDataStream readDataStream(&file);
//    statusLabel->setText(QStringLiteral("准备读取数据"));
    statusLabel->setText(QStringLiteral("Reading"));
    QApplication::processEvents();
    readDataStream.readRawData((char*)write_buffer,flashCtl.fileSize);//读取数据到缓存
//    statusLabel->setText(QStringLiteral("读取数据完毕"));
    statusLabel->setText(QStringLiteral("Read succeeded"));
    QApplication::processEvents();
    //for debug
//    ui->console->LOGI("flashCtl.writeBufferAddr[0]=0x%llx, flashCtl.writeBufferAddr[1]=0x%llx", flashCtl.writeBufferAddr[0], flashCtl.writeBufferAddr[1]);

    QByteArray *tempByte = new QByteArray(write_buffer,display_size);
    QString *tempStr = new QString(tempByte->toHex().toUpper());

    ULONGLONG addrData = flashCtl.address;

    QString addrStr;

    //display data as hex
    BYTE cnt = 1;
    WORD kcnt = 0;
//    ULONGLONG * temp = (ULONGLONG* )flashCtl.mtpWriteBuffer;
//    for(DWORD i = 0;i < dwLegth; i++)
//    {
//        fprintf(stderr, "printf Verifing data from MTP @ addr 0x%x, rdata @ 0x%llx\n", STATION_SDIO_MTP_ADDR + i * 8, temp[i]);
//    }
    for(ULONGLONG i = 2; i < tempStr->size();)
    {
        if(1 == cnt)// start with address of readout data
        {
//            addrStr = "0x";//indicate hex string
            addrStr = addrStr.sprintf("0x%08X",addrData);//add address information after "0x"
            addrStr = addrStr.append(": ");//add ":" after address
            tempStr->insert(i-2,addrStr);
            i += 12;
            addrData = addrData + 16;
        }
        tempStr->insert(i, ' ');//每个字节之间空一格
        i += 3;
        cnt++;
        if(cnt == 8)//每8个字节空2格
        {
            tempStr->insert(i, ' ');
            i += 1;
        }
        if(cnt == 16)//每16个字节空一格
        {
            cnt = 1;
            kcnt ++;
            if(kcnt == 64)//每64行，即1K数据，空一行
            {
                kcnt = 0;
                tempStr->insert(i, '\n');
                i++;
            }
            tempStr->insert(i, '\n');
            i += 3;         //避免换行后开头一个空格,换行相当于从新插入
        }

    }

    //QString text = QString::fromLocal8Bit((char*)flashCtl.mtpBuffer,flashCtl.fileSize);
    //setWindowTitle(fileName);
    //QTextStream in(&file);
    //QString text = in.readAll();
    ui->lineEditDir->setText(fileName);
    ui->textEdit->setText(*tempStr);
//    statusLabel->setText(QStringLiteral("显示数据完毕"));
    statusLabel->setText(QStringLiteral("Display succeeded"));
    QApplication::processEvents();
    file.close();
}

void MainWindow::save()
{
    QString fileName;
    // If we don't have a filename from before, get one.
    if (currentFile.isEmpty()) {
        fileName = QFileDialog::getSaveFileName(this, "Save");
    } else {
        fileName = currentFile;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
        return;
    }
    //setWindowTitle(fileName);
    QTextStream out(&file);
    QString text = ui->textEdit->toPlainText();
    out << text;
    file.close();
}

void MainWindow::saveas()
{
    QString fileName, filePath, fileSuffix;
    QFileInfo fileInfo;

    //get path of file
    if(!currentFile.isEmpty()) {
        fileInfo = QFile(currentFile);
        filePath = fileInfo.path();
    }
    else {
        filePath = QString(qApp->applicationDirPath());
    }

    fileName = QFileDialog::getSaveFileName(this, "Save as", QString(filePath).append("/readout.bin"), \
                                                    QString::fromLocal8Bit("bin File(*.bin);;txt File(*.txt);;All File(*)"));

    //get suffix of filename
    if(!fileName.isEmpty()) {
        fileSuffix = QFileInfo(QFile(fileName)).suffix();
    }
    else {
        return;
    }

    //match file type
    QFile file(fileName);

    if(fileSuffix == "txt") {
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            //QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
            return;
        }
        setWindowTitle(fileName);
        QTextStream out(&file);
        QString text = ui->textEdit->toPlainText();
        out << text;
    }
    else /*if(fileSuffix == "bin")*/ {
        if (!file.open(QFile::WriteOnly)) {
            //QMessageBox::warning(this, "Warning", "Cannot save file: " + file.errorString());
            return;
        }
        setWindowTitle(fileName);
        QDataStream out(&file);
        out.writeRawData((char*)flashCtl.readBufferAddr, mtplenth);
    }

    file.close();
}

void MainWindow::about()
{
//    QMessageBox::information(NULL,QStringLiteral("关于"),QStringLiteral("名称：RvFlash\n版本：V1.1\n时间：2022-04-29"),QStringLiteral("确定"));
//    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.2\nTime    ：2022-12-13"),QStringLiteral("Confirm"));
//    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.3\nTime    ：2023-04-13"),QStringLiteral("Confirm"));
//    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.4\nTime    ：2023-04-17"),QStringLiteral("Confirm"));
//    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.5\nTime    ：2023-05-08"),QStringLiteral("Confirm"));
//    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.6\nTime    ：2023-07-25"),QStringLiteral("Confirm"));
    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.7\nTime    ：2023-07-25"),QStringLiteral("Confirm"));
}

void MainWindow::help()
{
    QDesktopServices helpDoc;
    QString strDoc = QString(qApp->applicationDirPath()).append(QStringLiteral("/doc/RvFlash使用说明.pdf"));
    if (!helpDoc.openUrl(QUrl::fromLocalFile(strDoc))) {
//        QMessageBox::information(NULL,QStringLiteral("警告！"),QStringLiteral("无法打开帮助文档!"),QStringLiteral("确定"));
        QMessageBox::information(NULL,QStringLiteral("Warning！"),QStringLiteral("Can't open user guide!"),QStringLiteral("Confirm"));
    }
}

void MainWindow::config()
{
//    if(configInit()) {
//        QMessageBox::information(NULL,QStringLiteral("通知"),QStringLiteral("配置成功!"),QStringLiteral("确定"));
//    } else {
//        QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("配置失败，没有找到配置文件!\n请检查目录下RvFlash.ini！"),QStringLiteral("确定"));
//    }
    if(configInit()) {
        QMessageBox::information(NULL,QStringLiteral("Notice！"),QStringLiteral("Config succeeded!"),QStringLiteral("Confirm"));
    } else {
        QMessageBox::warning(NULL,QStringLiteral("Warning！"),QStringLiteral("Config failed，did not find config file!\nPlease check RvFlash.ini！"),QStringLiteral("Confirm"));
    }
}

void MainWindow::on_pushButtonClear_clicked()
{
    ui->console->clear();
}

void MainWindow::on_lineEditAddress_editingFinished()
{
    bool ok;

    QString text = ui->lineEditAddress->text();
    flashCtl.address = text.toLongLong(&ok,16);
    sysCtl.pcValue   = flashCtl.address;
//    ui->console->LOGI(QString(QStringLiteral("设置起始地址 @ 0x%llx\n")).toStdString().data(), flashCtl.address);
    ui->console->LOGI(QString(QStringLiteral("Config Base Addr @ 0x%llx\n")).toStdString().data(), flashCtl.address);

//    ULONGLONG sector = 0;
//    DWORD sectorlocat = 0;
//    ULONGLONG dwLegth = flashCtl.fileSize;
//    sector = ceil((float)dwLegth / (8 * 1024));
//    sectorlocat = (float)(flashCtl.address - STATION_SDIO_MTP_ADDR) / mtpsecsize;
//    sectorlocat = sectorlocat + 1;
//    fprintf(stderr,  "flashCtl.address @ 0x%llx sector & %lld sectorlocat @ %ld\n", flashCtl.address, sector, sectorlocat);
}

void MainWindow::on_lineEditDir_editingFinished()
{
    uint64_t display_size;
    QString fileName, filePath;
    QFileInfo fileInfo;

    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
    fileName = ui->lineEditDir->text();
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(this, "Open the file", qApp->applicationDirPath(), \
                                                    QString::fromLocal8Bit("bin File(*.bin);;hex File(*.hex);;elf File(*.elf);;All File(*)"));
    }
    else {
        //get filepath
        currentFile = fileName;
        fileInfo = QFileInfo(currentFile);
        filePath = fileInfo.path();
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        //QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }

    //convert fileName to char, save it to flashCtl.fileName
    QByteArray filename = fileName.toLocal8Bit();
    flashCtl.fileName = filename.data();
    flashCtl.fileSize = file.size();//get file size/lenth
    flashCtl.currentfileSize = file.size();//backup file.size

    //filesize up to mtpsize
    if(flashCtl.fileSize > mtpsize)
    {
//        QMessageBox::critical(NULL,QStringLiteral("警告！"),QStringLiteral("仅显示前64KB数据！"),QStringLiteral("确定"));
//        QMessageBox::critical(NULL,QStringLiteral("Warning！"),QStringLiteral("Only display 64KB data！"),QStringLiteral("Confirm"));
        display_size = mtpsize;
    } else
    {
        display_size = flashCtl.fileSize;
    }

    float lenth = (float)flashCtl.fileSize / 1024;
    ui->console->LOGI("fileName=%s\nfileSize=%.1fK (%dBYTES)\n", flashCtl.fileName, lenth, flashCtl.fileSize);

    //read data from file
    QDataStream readDataStream(&file);
//    statusLabel->setText(QStringLiteral("准备读取数据"));
    statusLabel->setText(QStringLiteral("Reading"));
    QApplication::processEvents();
    readDataStream.readRawData((char*)write_buffer,flashCtl.fileSize);//读取数据到缓存
//    statusLabel->setText(QStringLiteral("读取数据完毕"));
    statusLabel->setText(QStringLiteral("Read succeeded"));
    QApplication::processEvents();
    //for debug
//    ui->console->LOGI("flashCtl.writeBufferAddr[0]=0x%llx, flashCtl.writeBufferAddr[1]=0x%llx", flashCtl.writeBufferAddr[0], flashCtl.writeBufferAddr[1]);

    QByteArray *tempByte = new QByteArray(write_buffer,display_size);
    QString *tempStr = new QString(tempByte->toHex().toUpper());

    ULONGLONG addrData = flashCtl.address;

    QString addrStr;

    //display data as hex
    BYTE cnt = 1;
    WORD kcnt = 0;
//    ULONGLONG * temp = (ULONGLONG* )flashCtl.mtpWriteBuffer;
//    for(DWORD i = 0;i < dwLegth; i++)
//    {
//        fprintf(stderr, "printf Verifing data from MTP @ addr 0x%x, rdata @ 0x%llx\n", STATION_SDIO_MTP_ADDR + i * 8, temp[i]);
//    }
    for(ULONGLONG i = 2; i < tempStr->size();)
    {
        if(1 == cnt)// start with address of readout data
        {
//            addrStr = "0x";//indicate hex string
            addrStr = addrStr.sprintf("0x%08X",addrData);//add address information after "0x"
            addrStr = addrStr.append(": ");//add ":" after address
            tempStr->insert(i-2,addrStr);
            i += 12;
            addrData = addrData + 16;
        }
        tempStr->insert(i, ' ');//每个字节之间空一格
        i += 3;
        cnt++;
        if(cnt == 8)//每8个字节空2格
        {
            tempStr->insert(i, ' ');
            i += 1;
        }
        if(cnt == 16)//每16个字节空一格
        {
            cnt = 1;
            kcnt ++;
            if(kcnt == 64)//每64行，即1K数据，空一行
            {
                kcnt = 0;
                tempStr->insert(i, '\n');
                i++;
            }
            tempStr->insert(i, '\n');
            i += 3;         //避免换行后开头一个空格,换行相当于从新插入
        }

    }

    ui->lineEditDir->setText(fileName);
    ui->textEdit->setText(*tempStr);
//    statusLabel->setText(QStringLiteral("显示数据完毕"));
    statusLabel->setText(QStringLiteral("Display succeeded"));
    QApplication::processEvents();
    file.close();
}

void MainWindow::on_checkBoxErase_stateChanged(int arg1)
{
    if(ui->checkBoxErase->isChecked())
    {
        flashCtl.autoEraseFlag = true;
    }
    else
    {
        flashCtl.autoEraseFlag = false;
    }
}

void MainWindow::on_checkBoxProgram_stateChanged(int arg1)
{
    if(ui->checkBoxProgram->isChecked())
    {
        flashCtl.autoWriteFlag = true;
    }
    else
    {
        flashCtl.autoWriteFlag = false;
    }
}

void MainWindow::on_checkBoxVerify_stateChanged(int arg1)
{
    if(ui->checkBoxVerify->isChecked())
    {
        flashCtl.autoVerifyFlag = true;
    }
    else
    {
        flashCtl.autoVerifyFlag = false;
    }
}

void MainWindow::on_checkBoxRun_stateChanged(int arg1)
{
    if(ui->checkBoxRun->isChecked())
    {
        flashCtl.autoRunFlag   = true;
        sysCtl.holdRstValue    = 0x8000001f; // run to rst pc
        sysCtl.releaseRstValue = 0x80000000;
    }
    else
    {
        flashCtl.autoRunFlag   = true;
        sysCtl.holdRstValue    = 0x0000001f; // run to debug loop
        sysCtl.releaseRstValue = 0x00000000;
    }
}

void MainWindow::doSomething(void)
{
    my_thread->start();
}

void MainWindow::updateProgress(int p, float speed, const char* msg, int type)
{
    QString speedMsg;
    static time_t ltime;

    ltime = time(NULL);
    if(threadstart == (type & 0x3))
    ui->console->LOGI(msg, asctime(localtime(&ltime)));

    ui->progressBar->setValue(p);

    ltime = time(NULL);
    if(threadend == (type & 0x3))
    ui->console->LOGI(msg, asctime(localtime(&ltime)));

    ltime = time(NULL);
    if((threadstart | holdReset) == type)
    ui->console->LOGI(msg, asctime(localtime(&ltime)));

    ltime = time(NULL);
    if((threadend | releaseReset) == type)
    ui->console->LOGI(msg, asctime(localtime(&ltime)));

    if((threadend | setPc) == type)
    ui->console->LOGI(msg, sysCtl.pcValue);

    //ui->console->LOGI(msg);
//    if((threadstart | writetype) == type)  statusLabel->setText(QStringLiteral("正在写入"));
//    if((threadend | writetype) == type)  statusLabel->setText(QStringLiteral("写入完成"));
    if((threadstart | writetype) == type)  statusLabel->setText(QStringLiteral("Writing"));
    if((threadend | writetype) == type)  statusLabel->setText(QStringLiteral("Write successed"));
    if((threadworking | writetype) == type)
    {
        speedMsg = speedMsg.sprintf("Writing %.1fKB/s", speed);
        statusLabel->setText(speedMsg);
    }
    if((threadworking | writeerror) == type)
    {
//        statusLabel->setText(QStringLiteral("写入错误"));
        statusLabel->setText(QStringLiteral("Write error"));
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
//        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("写入错误！"),QStringLiteral("确定"));
        QMessageBox::critical(NULL,QStringLiteral("Error！"),QStringLiteral("Write error！"),QStringLiteral("Confirm"));
    }

//    if((threadstart | readtype) == type)  statusLabel->setText(QStringLiteral("正在读取"));
//    if((threadend | readtype) == type)  statusLabel->setText(QStringLiteral("读取完成"));
    if((threadstart | readtype) == type)  statusLabel->setText(QStringLiteral("Reading"));
    if((threadend | readtype) == type)  statusLabel->setText(QStringLiteral("Read succeeded"));
    if((threadworking | readtype) == type)
    {
        speedMsg = speedMsg.sprintf("Verifying %.1fKB/s", speed);
        statusLabel->setText(speedMsg);
    }
    if((threadworking | readerror) == type)
    {
//        statusLabel->setText(QStringLiteral("校验错误"));
        statusLabel->setText(QStringLiteral("Verify error"));
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
//        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("校验错误！"),QStringLiteral("确定"));
        QMessageBox::critical(NULL,QStringLiteral("Error！"),QStringLiteral("Verify error！"),QStringLiteral("Confirm"));
    }

//    if((threadstart | readouttype) == type)  statusLabel->setText(QStringLiteral("正在读取"));
    if((threadstart | readouttype) == type)  statusLabel->setText(QStringLiteral("Reading"));
    if((threadworking | readouttype) == type)
    {
        speedMsg = speedMsg.sprintf("Reading %.1fKB/s", speed);
        statusLabel->setText(speedMsg);
    }
    if((threadend | readouttype) == type)
    {
//        statusLabel->setText(QStringLiteral("读取完成"));
        statusLabel->setText(QStringLiteral("Read succeeded"));
        dataToConsole();
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
//        QMessageBox::information(NULL,QStringLiteral("通知！"),QStringLiteral("读取完成！"),QStringLiteral("确定"));
        QMessageBox::information(NULL,QStringLiteral("Notice！"),QStringLiteral("Read succeeded！"),QStringLiteral("Confirm"));
    }

    if((threadworking | readouterror) == type)
    {
//        statusLabel->setText(QStringLiteral("读出错误"));
        statusLabel->setText(QStringLiteral("Read error"));
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
//        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("读出错误！"),QStringLiteral("确定"));
        QMessageBox::critical(NULL,QStringLiteral("Error！"),QStringLiteral("Read error！"),QStringLiteral("Confirm"));
    }

//    if((threadstart | verifytype) == type) statusLabel->setText(QStringLiteral("正在校验"));
//    if((threadend | verifytype) == type)  statusLabel->setText(QStringLiteral("校验完成"));
    if((threadstart | verifytype) == type) statusLabel->setText(QStringLiteral("Verifying"));
    if((threadend | verifytype) == type)  statusLabel->setText(QStringLiteral("Verify succeeded"));
    if((threadend | verifyerror) == type)
    {
//        statusLabel->setText(QStringLiteral("校验错误"));
        statusLabel->setText(QStringLiteral("Verify error"));
//        QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("校验错误！！！"),QStringLiteral("确定"));
        QMessageBox::warning(NULL,QStringLiteral("Warning！"),QStringLiteral("Verify error！！！"),QStringLiteral("Confirm"));
    }

//    if((threadstart | erasetype) == type) statusLabel->setText(QStringLiteral("正在擦除"));
//    if((threadend | erasetype) == type)  statusLabel->setText(QStringLiteral("擦除完成"));
    if((threadstart | erasetype) == type) statusLabel->setText(QStringLiteral("Erasing"));
    if((threadend | erasetype) == type)  statusLabel->setText(QStringLiteral("Erase succeeded"));


    //flashtimer start record and show
    if((threadstart | flashtimer) == type)
    {
        mstimerInit();
    }
    //flashtimer stop record and show
    if((threadend | flashtimer) == type)
    {
        mstimer->stop();
    }
}

void MainWindow::stimerInit()
{
    stimer->start(1000);//1s=1000ms
    QDateTime stime = QDateTime::currentDateTime();//get current datetime
    //QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");//change to string
    QString str = stime.toString("yyyy-MM-dd hh:mm:ss");//change to string

    //timeDataLable->setMinimumSize(0, 20); // 大小
    timeDataLable->setFrameStyle(QFrame::NoFrame | QFrame::Raised);
    timeDataLable->setText(str);
    timeDataLable->setTextFormat(Qt::AutoText);

    ui->statusbar->addPermanentWidget(timeDataLable);
}

void MainWindow::mstimerInit()
{
    baseTime = QTime::currentTime();
    mstimer->start(1);//1ms timer
}

void MainWindow::timerUpdate()
{
    QDateTime time = QDateTime::currentDateTime();//get current datetime
    //QString str = time.toString("yyyy-MM-dd hh:mm:ss dddd");//change to string
    QString str = time.toString("yyyy-MM-dd hh:mm:ss");//change to string

    timeDataLable->setText(str);

    ui->statusbar->addWidget(timeDataLable);
}

void MainWindow::timerElapsedInit()
{
    //timerElapsedLable->setMinimumSize(150, 20); // 大小

    timerElapsedLable->setFrameShape(QFrame::NoFrame); // 形状

    timerElapsedLable->setFrameShadow(QFrame::Raised); // 阴影

    ui->statusbar->addWidget(timerElapsedLable);

//    timerElapsedLable->setText(QStringLiteral("耗时："));
    timerElapsedLable->setText(QStringLiteral("Elapsed time："));

}


void MainWindow::timerElapsed()
{
//    QString timeconsumption = QStringLiteral("耗时：");
    QString timeconsumption = QStringLiteral("Elapsed time：");

    QTime mstime = QTime::currentTime();//get current time

    int t = this->baseTime.msecsTo(mstime);

    QTime showtime(0,0,0);

    showtime = showtime.addMSecs(t);

    showStr = showtime.toString("mm:ss");

    showStr = timeconsumption.append(showStr);

    timerElapsedLable->setText(showStr);
}

void MainWindow::statusBarInit()
{
    statusLabel->setMinimumSize(400, 20); // 大小

    statusLabel->setFrameShape(QFrame::NoFrame); // 形状

    statusLabel->setFrameShadow(QFrame::Raised); // 阴影

    ui->statusbar->addWidget(statusLabel);

    statusLabel->setText(QStringLiteral("RvFlash"));
}

void MainWindow::dataToConsole()
{
    ULONGLONG addrData = flashCtl.address;
    QByteArray *tempByte = new QByteArray(read_buffer, mtpsize);
    QString *tempStr = new QString(tempByte->toHex().toUpper());
    QString addrStr;
    //display data as hex
    BYTE cnt = 1;
    WORD kcnt = 0;
//    ULONGLONG * temp = (ULONGLONG* )flashCtl.mtpWriteBuffer;
//    for(DWORD i = 0;i < dwLegth; i++)
//    {
//        fprintf(stderr, "printf Verifing data from MTP @ addr 0x%x, rdata @ 0x%llx\n", STATION_SDIO_MTP_ADDR + i * 8, temp[i]);
//    }
    for(ULONGLONG i = 2; i < tempStr->size();)
    {
        if(1 == cnt)// start with address of readout data
        {
//            addrStr = "0x";//indicate hex string
            addrStr = addrStr.sprintf("0x%08X",addrData);//add address information after "0x"
            addrStr = addrStr.append(": ");//add ":" after address
            tempStr->insert(i-2,addrStr);
            i += 12;
            addrData = addrData + 16;
        }
        tempStr->insert(i, ' ');//每个字节之间空一格
        i += 3;
        cnt++;
        if(cnt == 8)//每8个字节空2格
        {
            tempStr->insert(i, ' ');
            i += 1;
        }
        if(cnt == 16)//每16个字节空一格
        {
            cnt = 1;
            kcnt ++;
            if(kcnt == 64)//每64行，即1K数据，空一行
            {
                kcnt = 0;
                tempStr->insert(i, '\n');
                i++;
            }
            tempStr->insert(i, '\n');
            i += 3;         //避免换行后开头一个空格,换行相当于从新插入
        }

    }

    ui->textEdit->setText(*tempStr);
    //statusLabel->setText(QStringLiteral("读出数据显示"));
    QApplication::processEvents();
}

bool MainWindow::checkStatus()
{
    ULONGLONG dwLegth = 0;
    ULONGLONG overSize;
    ULONGLONG overLength;
//    ULONGLONG mtpoffset = 0;

    //recover filesize value
    flashCtl.fileSize = flashCtl.currentfileSize;

    if(flashCtl.proj == PEP)
    {
        dwLegth  = filesize64 + ceil((float)(mtpoffset) / 8);
        overSize = STATION_SDIO_MTP_WIDTH_IN_BYTE;
        overLength= mtplenth;
    }
    else
    {
        dwLegth  = filesize64;
        overSize = bufsize;
        overLength= bufsize;
    }
    dwLegth = (dwLegth * 8);//calculate requred space = offset + filesize

    if(0 != (flashCtl.address % 8))//align with 8bytes
    {
//        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("地址需按8字节对齐！"),QStringLiteral("确定"));
        QMessageBox::critical(NULL,QStringLiteral("Error！"),QStringLiteral("Need align address with 8 Bytes！"),QStringLiteral("Confirm"));
        return false;
    }

    if(dwLegth > overSize)
    {
        flashCtl.fileSize = overLength;
        if((true == flashCtl.writeFlag) || (true == flashCtl.autoFlag))
        {
//            QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("文件超出flash容量限制,超出部分将会忽略！"),QStringLiteral("确定"));
            QMessageBox::warning(NULL,QStringLiteral("Warning！"),QStringLiteral("Size over limit，drop oversize data！"),QStringLiteral("Confirm"));
        }
        //return false;
    }
    return true;
}

bool MainWindow::checkConnect()
{
    BOOL ft_status = true;
    BYTE ft_connect_count = 0;
    static time_t ltime;
    ft_close();
    ft_status = ft_dev_init(ft_freq);
    ft_status &= ft_close();

    //try to recover when connect fail
    while(false == ft_status)
    {
        ft_status = ft_dev_init(ft_freq);
        ft_status &= ft_close();
        ft_connect_count ++;
        if(ft_connect_count == 3)
        {
            //my_thread->exit(0);
            //my_thread->quit();
            my_thread->terminate();//if connect fail, try to teminate my_thread and release ftxxx device.
            //my_thread->wait();
            //my_thread->stop();
//            process_reset();
            if(true == ft_status)
            {
//                QMessageBox::information(NULL,QStringLiteral("通知！"),QStringLiteral("RvFlash重连成功！"),QStringLiteral("确定"));
                QMessageBox::information(NULL,QStringLiteral("Notice！"),QStringLiteral("RvFlash reconnect succeeded ！"),QStringLiteral("Confirm"));
                //重置状态栏和进度条
                statusLabel->setText(QStringLiteral("*Restore connection*"));
                ui->progressBar->setValue(0);
                flashctl_reset();
            }
        }
        if(ft_connect_count == 10)
        {
//            QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("RvFlash连接失败,请重新连接！"),QStringLiteral("确定"));
            QMessageBox::warning(NULL,QStringLiteral("Warning！"),QStringLiteral("RvFlash connect failed, please reconnect！"),QStringLiteral("Confirm"));
            return false;
        }
    }

    ltime = time(NULL);
    //ui->console->LOGI( "Start connecting to soc @ %s\n", asctime(localtime(&ltime)));

    //ft_open before operation
    ft_open();

    //reset chip before operation
    process_reset();

    //check connect status up to 3 times
    ft_connect_count = 0;
    while(ft_connect_count < 3)
    {
        flashCtl.connectStatus = (flashCtl.proj == PEP) ? chipConnect() : true;
//        ui->console->LOGI( "Start connecting to soc @ %d\n", ft_connect_count);
//        flashCtl.connectStatus = true;
        if (true == flashCtl.connectStatus) {
            break;
        }
        ft_connect_count ++;
    }
    //ft_close after operation
    ft_close();

    if (false == flashCtl.connectStatus)
    {
        ltime = time(NULL);
        ui->console->LOGI( "Fail connecting to soc @ %s\n", asctime(localtime(&ltime)));
        return false;
    }

    ltime = time(NULL);
    ui->console->LOGI( "Done connecting to soc @ %s\n", asctime(localtime(&ltime)));
    return true;
}

bool MainWindow::checkCurrentFile()
{
    if(currentFile.isEmpty())
    {
//        QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("请加载程序文件！！！"),QStringLiteral("确定"));
        QMessageBox::warning(NULL,QStringLiteral("Warning！"),QStringLiteral("Please load file！！！"),QStringLiteral("Confirm"));
        return false;
    }
    return true;
}

void MainWindow::consoleInit()
{
    QFont qf;
    qf.setPointSize(12);
    //qf.setFamily("sans-serif");
#ifdef _WIN32
    qf.setFamily("Source Code Pro");
#else
    qf.setFamily("Liberation Mono");
#endif
    qf.setBold(true);
    //qf.setPixelSize(18);
    //qf.setStyle(QFont::StyleNormal);
    ui->textEdit->setFont(qf);
    ui->console->setFont(qf);

    //Init color of textEdit
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    ui->textEdit->setPalette(p);
    ui->console->setPalette(p);

    ui->textEdit->setReadOnly(true);

//    // get font famlily which have same character width
//    QFontDatabase fontDB;
//    foreach (const QString& family, fontDB.families())
//    {
//        QFont font(family, 8, QFont::Bold);
//        QFontMetrics fm(font);
//        int nLen = fm.width(' ');
//        int nLen0 = fm.width('0');
//        int nLen1 = fm.width('-');
//        if (nLen == nLen0 && nLen == nLen1)
//        {
//            qDebug()<<family;
//        }
//    }
}

void MainWindow::consoleDebug(const QByteArray &data)
{
    bool cmd_valid = false;
    bool rvStatus  = false;
    char cmd_buffer[100];
    const char *debug_buffer = "debug";
//    BYTE *ft_mode;

    memset(cmd_buffer, 0, 100);
//    ui->textEdit->insertPlainText(QString("plug test!\n"));
    ui->console->putData(data);
//    ui->console->LOGI("0x%x",*(data.data()));
    if(cmd_buffer_index > 100) {
        cmd_valid = true;
        cmd_buffer_index = 0;
    } else if(*(data.data()) == '\r') {
        cmd_valid = true;
        QString str;
        ULONGLONG lineCount = 0;
        //get line number
        lineCount = ui->console->document()->lineCount();
//        ui->console->LOGI("lineCount=%d\n",lineCount);
        //print data of specified line
        if(lineCount >= 2) {
            str = ui->console->document()->findBlockByLineNumber(lineCount-2).text();
        }
//        //only for debug
//        ui->textEdit->insertPlainText(str.append('\r'));
        if(cmd_debug == false) {
            ui->console->LOGI(">>");
        }
//        ui->textEdit->insertPlainText(str.append('\r'));
//        cmd_buffer[cmd_buffer_index++] = '\0';
//        cmd_buffer_index = 0;
        QByteArray cmdBuf = str.toLocal8Bit();
        if(0 == memcmp(cmdBuf.data(), debug_buffer, strlen(debug_buffer))) {
            memcpy(cmd_buffer, cmdBuf.data(), strlen(cmdBuf.data()));
        } else {
            memcpy(cmd_buffer, &cmdBuf.data()[2] , strlen(cmdBuf.data()));
        }
    } else {
        cmd_valid = false;
//        cmd_buffer[cmd_buffer_index++] = *(data.data());
    }

    std::string cmd;
    std::string delimiter = " ";
    size_t pos = 0;
    std::vector <std::string> tokenQ;
    if(cmd_valid == true)
    {
        cmd = cmd_buffer;

        while ((pos = cmd.find(delimiter)) != std::string::npos) {
            tokenQ.push_back(cmd.substr(0, pos));
            cmd.erase(0, pos + delimiter.length());
        }
        if (!cmd.empty()) {
            tokenQ.push_back(cmd);
        }

        if(!cmd.empty() && (tokenQ.size() != 0)){
            ui->console->cmd_status = 0;
            memset(ui->console->cmd_buffer[ui->console->cmd_buffer_index], 0, 100);
            memcpy(ui->console->cmd_buffer[ui->console->cmd_buffer_index], cmd_buffer, strlen(cmd_buffer));
            ui->console->cmd_record_index = ui->console->cmd_buffer_index;
            if(ui->console->cmd_record_index_max < 9) {
                ui->console->cmd_record_index_max = ui->console->cmd_buffer_index;
            } else {
                ui->console->cmd_record_index_max = 9;
            }
//            QString print_buf_index;
//            QString print_record_index_max;
//            print_buf_index = print_buf_index.sprintf("cmd_buffer_index=%d\n",ui->console->cmd_buffer_index);
//            print_record_index_max = print_record_index_max.sprintf("cmd_record_index_max=%d\n",ui->console->cmd_record_index_max);
//            ui->textEdit->insertPlainText(print_buf_index);
//            ui->textEdit->insertPlainText(print_record_index_max);
            if(ui->console->cmd_buffer_index >= 9) {
                ui->console->cmd_buffer_index = 0;
            } else {
                ui->console->cmd_buffer_index = ui->console->cmd_buffer_index + 1;
            }
        }

        if ((tokenQ.size() == 1) && (tokenQ[0] == "debug")) {
            RvFlashReadCfg();
            ft_close();
            if(ft_dev_init(ft_freq)) {
                cmd_debug = true;
            } else {
                cmd_debug = false;
                ui->console->LOGI("Connect failed!\n");
                ui->console->LOGI(">>");
            }
            ft_close();
        }
    }

    if(cmd_debug == true)
    {
        if(cmd_valid == true)
        {
            rvStatus  = ft_open();
            if ((tokenQ.size() == 1) && (tokenQ[0] == "reset")) {
                process_reset();
                if(flashCtl.proj != PEP) {
                    sysHoldReset();
                    usleep(50000);
                    sysReleaseReset();
                    ui->console->LOGI("Reset Core\n");
                } else {
                    ui->console->LOGI("Reset System\n");
                }
            } else if ((tokenQ.size() == 1) && (tokenQ[0] == "debug")) {
                ui->console->LOGI("Enter Debug Mode\n");
            } else if ((tokenQ.size() == 0) || (tokenQ[0] == "help") || (tokenQ[0] == "h")) {
                ui->console->LOGI("This is Help Info\n");
            } else if ((tokenQ[0] == "quit") || (tokenQ[0] == "q") || (tokenQ[0] == "exit")) {
                cmd_debug = false;
            } else if ((tokenQ[0] == "fe_pc")) {
                uint64_t pcgen_pc[2];
                uint64_t inst_pc;
                uint64_t addr;
                addr        = 0xf0000000;
                pcgen_pc[0] = rv_do_read(addr, (ULONGLONG*)&pcgen_pc[0]);
                pcgen_pc[1] = rv_do_read(addr + 0x4, (ULONGLONG*)&pcgen_pc[1]);
                inst_pc     = (pcgen_pc[1] << 32) | pcgen_pc[0];
                ui->console->LOGI("Do Read to Addr 0x%llx, Got Data 0x%llx\n", addr, inst_pc);
            } else if ((tokenQ[0] == "be_pc")) {
                uint64_t data = 0;
                uint64_t addr_lo = 0xf0020010;
                uint64_t addr_hi;
                addr_hi = addr_lo + 0x8;
                ui->console->LOGI("addr_lo = 0x%llx, addr_hi = 0x%llx\n", addr_lo, addr_hi);
                for (uint64_t addr = addr_lo; addr <= addr_hi; addr += ((flashCtl.proj == PEP) ? 8 : 4)) {
                    rvStatus = rv_do_read(addr, (ULONGLONG*)&data);

                    ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 0, (data >>  0) & 0xffffffff);
                    if(flashCtl.proj == PEP) {
                        ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 4, (data >> 32) & 0xffffffff);
                    }
                }
            } else if ((tokenQ[0] == "dfx_fe")) {
                uint64_t data = 0;
                uint64_t addr_lo = 0xf0000000;
                uint64_t addr_hi;
                addr_hi = addr_lo + 0x3c;
                ui->console->LOGI("addr_lo = 0x%llx, addr_hi = 0x%llx\n", addr_lo, addr_hi);
                for (uint64_t addr = addr_lo; addr <= addr_hi; addr += ((flashCtl.proj == PEP) ? 8 : 4)) {
                    rvStatus = rv_do_read(addr, (ULONGLONG*)&data);

                    ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 0, (data >>  0) & 0xffffffff);
                    if(flashCtl.proj == PEP) {
                        ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 4, (data >> 32) & 0xffffffff);
                    }
                }
            } else if ((tokenQ[0] == "dfx_be")) {
                uint64_t data = 0;
                uint64_t addr_lo = 0xf0020000;
                uint64_t addr_hi;
                addr_hi = addr_lo + 0x3c;
                ui->console->LOGI("addr_lo = 0x%llx, addr_hi = 0x%llx\n", addr_lo, addr_hi);
                for (uint64_t addr = addr_lo; addr <= addr_hi; addr += ((flashCtl.proj == PEP) ? 8 : 4)) {
                    rvStatus = rv_do_read(addr, (ULONGLONG*)&data);

                    ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 0, (data >>  0) & 0xffffffff);
                    if(flashCtl.proj == PEP) {
                        ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 4, (data >> 32) & 0xffffffff);
                    }
                }
            } else if ((tokenQ.size() == 3) && ((tokenQ[0] == "read") || (tokenQ[0] == "r"))) {
                char * p;
                uint64_t data = 0;
                uint64_t addr = strtoul(tokenQ[1].c_str(), & p, 16);
                std::string target = tokenQ[2];
                if (*p == 0) {
                  if (target == "rb") {
                    rvStatus = rv_do_read(addr, (ULONGLONG*)&data);
                  } else if (target == "dma") {
                    // rv_do_write(STATION_DMA_DMA_DEBUG_ADDR_ADDR, addr);
                    // rv_do_write(STATION_DMA_DMA_DEBUG_REQ_TYPE_ADDR, 0);
                    // data = rv_do_read(STATION_DMA_DMA_DEBUG_RD_DATA_ADDR);
                  } else if (target == "dt") {
                    //rv_do_write(STATION_DT_DBG_ADDR_ADDR, addr);
                    //data = rv_do_read(STATION_DT_DBG_DATA_ADDR);
                  } else {
                    rvStatus = rv_do_read(addr, (ULONGLONG*)&data);
                  }
                   ui->console->LOGI("Do Read to Addr 0x%llx, Got Data 0x%llx\n", addr, data);
                }
            } else if ((tokenQ.size() == 4) && ((tokenQ[0] == "write") || (tokenQ[0] == "w"))) {
                char * p;
                uint64_t addr = strtoul(tokenQ[1].c_str(), & p, 16);
                uint64_t data = strtoul(tokenQ[2].c_str(), & p, 16);
                std::string target = tokenQ[3];
                if (*p == 0) {
                  if (target == "rb") {
                    rv_do_write(addr, data);
                  } else if (target == "dma") {
                    // rv_do_write(STATION_DMA_DMA_DEBUG_ADDR_ADDR, addr);
                    // rv_do_write(STATION_DMA_DMA_DEBUG_REQ_TYPE_ADDR, 2);
                    // rv_do_write(STATION_DMA_DMA_DEBUG_WR_DATA_ADDR, data);
                  } else if (target == "dt") {
                    //rv_do_write(STATION_DT_DBG_ADDR_ADDR, addr);
                    //rv_do_write(STATION_DT_DBG_DATA_ADDR, data);
                  } else {
                    rv_do_write(addr, data);
                  }
                  ui->console->LOGI("Do Write to Addr 0x%llx with Data 0x%llx\n", addr, data);
                }
            } else if ((tokenQ.size() == 4) && (tokenQ[0] == "dump")) {
                char * p;
                uint64_t data = 0;
                uint64_t addr_lo = strtoul(tokenQ[1].c_str(), & p, 16);
                uint64_t addr_hi = strtoul(tokenQ[2].c_str(), & p, 16);
                std::string target = tokenQ[3];
                if (*p == 0) {
                    ui->console->LOGI("addr_lo = 0x%llx, addr_hi = 0x%llx\n", addr_lo, addr_hi);
                    for (uint64_t addr = addr_lo; addr <= addr_hi; addr += ((flashCtl.proj == PEP) ? 8 : 4)) {
                        if (target == "rb") {
                          rvStatus = rv_do_read(addr, (ULONGLONG*)&data);
                        } else if (target == "dma") {
                          // rv_do_write(STATION_DMA_DMA_DEBUG_ADDR_ADDR, addr);
                          // rv_do_write(STATION_DMA_DMA_DEBUG_REQ_TYPE_ADDR, 0);
                          // rvStatus = rv_do_read(STATION_DMA_DMA_DEBUG_RD_DATA_ADDR, &data);
                        } else if (target == "dt") {
                          //rv_do_write(STATION_DT_DBG_ADDR_ADDR, addr);
                          //rvStatus = rv_do_read(STATION_DT_DBG_DATA_ADDR, &data);
                        } else {
                          rvStatus = rv_do_read(addr, (ULONGLONG*)&data);
                        }

                        ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 0, (data >>  0) & 0xffffffff);
                        if(flashCtl.proj == PEP) {
                            ui->console->LOGI("0x%llx: 0x%08llx\n", addr + 4, (data >> 32) & 0xffffffff);
                        }
                    }
                }
            } else {
                    ui->console->LOGI("Unrecognized Command; Please use help or h to see supported command list.\n");
            }
            rvStatus  = ft_close();
            if(cmd_debug == false) {
                ui->console->LOGI(">>");
            } else {
                ui->console->LOGI("Please enter command: (All Data in HEX no matter 0x is added or not)\n: ");
            }
        }
    }
}

bool MainWindow::configInit()
{
    bool rvStatus = false;
    char tempBuffer[mtpsize];
    char * productBuffer;
    const char *configMsg;
    ULONGLONG fileSize;
    QString fileName, filePath;

    //get filepath
    filePath = qApp->applicationDirPath();
    fileName = QString(filePath).append("/RvFlash.ini");

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        //QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return false;
    }

    fileSize = file.size();//get file size/lenth

    //fill tempBuffer，all 0xff
    memset(tempBuffer, 0xff, mtpsize);//fill buffer with 0xff

    //read data
    QDataStream readDataStream(&file);
    readDataStream.readRawData((char*)tempBuffer,fileSize);//read data to tempBuffer
#ifdef _WIN32
    for (int i = 0; i < fileSize; i++) {
       if (std::strncmp(&tempBuffer[i], "SPEED=", 6) == 0) {
            rvCfgInfo.speed = std::atoi(&tempBuffer[i] + 6);
       } else if (std::strncmp(&tempBuffer[i], "WDEALY=", 7) == 0) {
            rvCfgInfo.wdelay = atoi(&tempBuffer[i] + 7);
       } else if (std::strncmp(&tempBuffer[i], "RDEALY=", 7) == 0) {
            rvCfgInfo.rdelay = atoi(&tempBuffer[i] + 7);
       } else if (std::strncmp(&tempBuffer[i], "WDELAY1=", 8) == 0) {
            rvCfgInfo.wdelay1 = atoi(&tempBuffer[i] + 8);
       } else if (std::strncmp(&tempBuffer[i], "RDELAY1=", 8) == 0) {
            rvCfgInfo.rdelay1 = atoi(&tempBuffer[i] + 8);
       } else if (std::strncmp(&tempBuffer[i], "WDELAY2=", 8) == 0) {
            rvCfgInfo.wdelay2 = atoi(&tempBuffer[i] + 8);
       } else if (std::strncmp(&tempBuffer[i], "RDELAY2=", 8) == 0) {
            rvCfgInfo.rdelay2 = atoi(&tempBuffer[i] + 8);
       } else if (std::strncmp(&tempBuffer[i], "PROJ=", 5) == 0) {
            rvCfgInfo.proj = std::atoi(&tempBuffer[i] + 5);
       } else if (std::strncmp(&tempBuffer[i], "PRODUCT=", 8) == 0) {
            productBuffer = std::strtok(&tempBuffer[i] + 8, " \r\n");
            strncpy(rvCfgInfo.product, productBuffer, strlen(productBuffer));
       }
     }
#else
    for (int i = 0; i < fileSize; i++) {
       if (strncmp(&tempBuffer[i], "SPEED=", 6) == 0) {
            rvCfgInfo.speed = atoi(&tempBuffer[i] + 6);
       } else if (strncmp(&tempBuffer[i], "WDELAY=", 7) == 0) {
            rvCfgInfo.wdelay = atoi(&tempBuffer[i] + 7);
       } else if (strncmp(&tempBuffer[i], "RDELAY=", 7) == 0) {
            rvCfgInfo.rdelay = atoi(&tempBuffer[i] + 7);
       } else if (strncmp(&tempBuffer[i], "WDELAY1=", 8) == 0) {
            rvCfgInfo.wdelay1 = atoi(&tempBuffer[i] + 8);
       } else if (strncmp(&tempBuffer[i], "RDELAY1=", 8) == 0) {
            rvCfgInfo.rdelay1 = atoi(&tempBuffer[i] + 8);
       } else if (strncmp(&tempBuffer[i], "WDELAY2=", 8) == 0) {
            rvCfgInfo.wdelay2 = atoi(&tempBuffer[i] + 8);
       } else if (strncmp(&tempBuffer[i], "RDELAY2=", 8) == 0) {
            rvCfgInfo.rdelay2 = atoi(&tempBuffer[i] + 8);
       } else if (strncmp(&tempBuffer[i], "PROJ=", 5) == 0) {
            rvCfgInfo.proj = atoi(&tempBuffer[i] + 5);
       } else if (strncmp(&tempBuffer[i], "PRODUCT=", 8) == 0) {
            productBuffer = strtok(&tempBuffer[i] + 8, " \r\n");
            strncpy(rvCfgInfo.product, productBuffer, strlen(productBuffer));
       }
    }
#endif

    // update PEP speed <= Freq2500(2500KHz)
    rvCfgInfo.speed = (rvCfgInfo.proj != PEP) ? rvCfgInfo.speed : (rvCfgInfo.speed > Freq2500) ? Freq2500 : rvCfgInfo.speed;
    // save rvlink config to rvCurrentCfgInfo
    rvCurrentCfgInfo = rvCfgInfo;
    // update rvCurrentCfgInfo.ft_wdelay, rvCurrentCfgInfo.ft_rdelay
    RvFlashCalcDelay();

    // update comboBoxFreq text
    QString devFreqQstring;
    devFreqQstring = devFreqQstring.sprintf("%d",ft_freq);
    ui->comboBoxFreq->setCurrentText(devFreqQstring);

    // initial rvlink config
    strncpy(ft_product, rvCurrentCfgInfo.product, strlen(rvCurrentCfgInfo.product)); // init ft_product
    RvFlashCurtCfg();
    flashCtl.proj    = rvCurrentCfgInfo.proj;  // init project

    // print config info
    ui->console->LOGI("Config Product   @ %s\r\n",ft_product);
    ui->console->LOGI(QString(QStringLiteral("Config Speed     @ %dKHz\n")).toStdString().data(),ft_freq);//rvlink/test io clk frequency
    ui->console->LOGI(QString(QStringLiteral("Config WDELAY    @ %dus\n")).toStdString().data(),ft_wdelay);//stable time of write data
    ui->console->LOGI(QString(QStringLiteral("Config RDELAY    @ %dus\n")).toStdString().data(),ft_rdelay);//stable time of read data

    switch(flashCtl.proj) {
        case PEP:
            configMsg = "Config Proj      @ PEP\n";
            break;
        case P506:
            configMsg = "Config Proj      @ 506\n";
            break;
        case P600:
            configMsg = "Config Proj      @ P600\n";
            break;
        case P800:
            configMsg = "Config Proj      @ P800\n";
            break;
        default:
            configMsg = "Config Proj      @ PEP\n";
    }
    ui->console->LOGI(configMsg);//initial proj

    if(flashCtl.proj == PEP)
    {
//        ui->pushButtonConnect->setEnabled(true);
        ui->pushButtonRead->setEnabled(true);
        ui->pushButtonErase->setEnabled(true);
        ui->checkBoxErase->setEnabled(true);
        ui->checkBoxErase->setChecked(true);
        ui->checkBoxVerify->setEnabled(true);
        ui->checkBoxVerify->setChecked(true);
        ui->checkBoxRun->setEnabled(true);
        ui->checkBoxRun->setChecked(true);
        flashCtl.autoEraseFlag = true;
        flashCtl.autoVerifyFlag = true;
        flashCtl.autoRunFlag = true;
    }
    else
    {
//        ui->pushButtonConnect->setEnabled(false);
        ui->pushButtonRead->setEnabled(false);
        ui->pushButtonErase->setEnabled(false);
        ui->checkBoxErase->setEnabled(false);
        ui->checkBoxErase->setChecked(false);
        ui->checkBoxVerify->setEnabled(true);
        ui->checkBoxVerify->setChecked(false);
        ui->checkBoxRun->setEnabled(true);
        ui->checkBoxRun->setChecked(true);
        flashCtl.autoEraseFlag = false;
        flashCtl.autoVerifyFlag = false;
        flashCtl.autoRunFlag = true;

//        ui->pushButtonConnect->setEnabled(true);
//        ui->pushButtonRead->setEnabled(true);
//        ui->pushButtonErase->setEnabled(false);
//        ui->checkBoxErase->setEnabled(false);
//        ui->checkBoxErase->setChecked(false);
//        ui->checkBoxVerify->setEnabled(true);
//        ui->checkBoxVerify->setChecked(false);
//        flashCtl.autoEraseFlag = true;
//        flashCtl.autoVerifyFlag = true;
    }

    return true;
}

void MainWindow::RvFlashReadCfg()
{
    ft_freq   = (rvCurrentCfgInfo.speed < Freq2000) ? rvCurrentCfgInfo.speed : Freq2000;
    ft_wdelay = rvCfgInfo.wdelay;
    ft_rdelay = rvCfgInfo.rdelay;
}

void MainWindow::RvFlashCurtCfg()
{
    // recover rvCurrentCfgInfo with rvCfgInfo
    ft_freq   = rvCurrentCfgInfo.speed;
    ft_wdelay = rvCurrentCfgInfo.wdelay;
    ft_rdelay = rvCurrentCfgInfo.rdelay;
}

void MainWindow::RvFlashCalcDelay()
{
    if(rvCurrentCfgInfo.speed <= Freq2500) {
        rvCurrentCfgInfo.wdelay = rvCfgInfo.wdelay;
        rvCurrentCfgInfo.rdelay = rvCfgInfo.rdelay;
    }
    else if(rvCurrentCfgInfo.speed <= Freq5000) {
        rvCurrentCfgInfo.wdelay = rvCfgInfo.wdelay1;
        rvCurrentCfgInfo.rdelay = rvCfgInfo.rdelay1;
    }
    else {
        rvCurrentCfgInfo.wdelay = rvCfgInfo.wdelay2;
        rvCurrentCfgInfo.rdelay = rvCfgInfo.rdelay2;
    }
}

void MainWindow::availableDevs(rvDevInfo* rvDev, BYTE* rvDevNum)
{
    DWORD numDevs;
    BYTE  rvNumDevs = 0;
    FT_DEVICE_LIST_INFO_NODE* devInfo;
    FT_STATUS ftStatus;

    *rvDevNum = 0;

    ftStatus = FT_CreateDeviceInfoList(&numDevs);
    if (ftStatus == FT_OK)
    {
//        ui->console->LOGI("Number of devices is %d\n", numDevs);
    }
    else
    {
        ui->console->LOGI("FT_CreateDeviceInfoList failed \n");// FT_CreateDeviceInfoList failed
    }

    if (numDevs > 0)
    {
        // allocate storage for list based on numDevs
        devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevs);
        // get the device information list
        ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
        if (ftStatus == FT_OK)
        {
            for (DWORD i = 0; i < numDevs; i++)
            {
//                ui->console->LOGI("Dev %d:\n", i);
//                ui->console->LOGI("Flags=0x%x\n", devInfo[i].Flags);
//                ui->console->LOGI("Type=0x%x\n", devInfo[i].Type);
//                ui->console->LOGI("ID=0x%x\n", devInfo[i].ID);
//                ui->console->LOGI("LocId=0x%x\n", devInfo[i].LocId);
//                ui->console->LOGI("SerialNumber=%s\n", devInfo[i].SerialNumber);
//                ui->console->LOGI("Description=%s\n", devInfo[i].Description);
//                ui->console->LOGI("ftHandle=0x%x\n", devInfo[i].ftHandle);
                if (strncmp(devInfo[i].Description, "RVLINK", 6) == 0)
                {
                    rvDev[rvNumDevs].devIndex     = i;
                    strcpy(rvDev[rvNumDevs].serialNumber, devInfo[i].SerialNumber);
                    strcpy(rvDev[rvNumDevs].description , devInfo[i].Description);
                    rvNumDevs                     ++;
                }
            }
            *rvDevNum = rvNumDevs;
        }
    }
}

void MainWindow::fillPortInfo()
{
    BYTE i;
    QString devIndex;
    ui->comboBoxPort->setInsertPolicy(QComboBox::NoInsert);
    for(i = 0; i < rvDevNum; i ++ )
    {
//        ui->console->LOGI("rvDevNum=%d\r\n",rvDevNum);
        devIndex = devIndex.sprintf("%d",rvPortInfo[i].devIndex);
        ui->comboBoxPort->addItem(QString(rvPortInfo[i].serialNumber).append("_dev").append(devIndex));
    }
//    ui->comboBoxPort->addItem(tr("Custom"));
}

void MainWindow::fillFreqInfo()
{
    QString devFreqQstring;
    ui->comboBoxFreq->setInsertPolicy(QComboBox::NoInsert);
    ui->comboBoxFreq->addItem(QString("1000"), MainWindow::Freq1000);
    ui->comboBoxFreq->addItem(QString("2000"), MainWindow::Freq2000);
    ui->comboBoxFreq->addItem(QString("2500"), MainWindow::Freq2500);
    ui->comboBoxFreq->addItem(QString("4000"), MainWindow::Freq4000);
    ui->comboBoxFreq->addItem(QString("5000"), MainWindow::Freq5000);
    ui->comboBoxFreq->addItem(QString("6000"), MainWindow::Freq6000);
    ui->comboBoxFreq->addItem(QString("7000"), MainWindow::Freq7000);
    ui->comboBoxFreq->addItem(QString("8000"), MainWindow::Freq8000);
    ui->comboBoxFreq->addItem(tr("Custom"));

    devFreqQstring = devFreqQstring.sprintf("%d",ft_freq);
    ui->comboBoxFreq->setCurrentText(devFreqQstring);
}

void MainWindow::on_comboBoxFreq_activated(int index)
{
    bool ok;
    QString devFreqQstring;
    DWORD   devFreq;
    devFreqQstring          = ui->comboBoxFreq->currentText();
    devFreq                 = devFreqQstring.toInt(&ok,10);
    const bool isCustomPath = (devFreqQstring == "Custom");
    ui->comboBoxFreq->setEditable(isCustomPath);
    if (isCustomPath)
    {
        ui->comboBoxFreq->clearEditText();
    }
    if(devFreq > 0)
    {
        rvCurrentCfgInfo.speed = devFreq;
        RvFlashCalcDelay();
    }
    RvFlashCurtCfg();
    ui->console->LOGI("Config Speed     @ %dKHz\n", ft_freq);
    ui->console->LOGI("Config WDELAY    @ %dus\n", ft_wdelay);
    ui->console->LOGI("Config RDELAY    @ %dus\n", ft_rdelay);
}

void MainWindow::on_comboBoxFreq_editTextChanged(const QString &)
{
    bool ok;
    QString devFreqQstring;
    DWORD   devFreq;
    devFreqQstring   = ui->comboBoxFreq->currentText();
    devFreq          = devFreqQstring.toInt(&ok,10);
    if((devFreq > Freq1000) && (devFreq < 10000) && (devFreq != currentFreq))
    {
        currentFreq = devFreq;
    }
    if(devFreq > 0)
    {
        rvCurrentCfgInfo.speed = devFreq;
        RvFlashCalcDelay();
    }
    RvFlashCurtCfg();
}

// update port infomation if MouseButtonPress happened
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    BYTE currentIndex;
    if(obj == ui->comboBoxPort)
    {
        if(event->type() == QEvent::MouseButtonPress)
        {// press mouse event
            currentIndex = ui->comboBoxPort->currentIndex();
            availableDevs(rvPortInfo, &rvDevNum);
            ui->comboBoxPort->clear();
            fillPortInfo();
            //    ui->pushButtonActive->setIcon(qicon.fromTheme("media-record"));
            ui->pushButtonActive->setIcon(QIcon(":/image/disconnect.png"));
            ui->pushButtonActive->setIconSize(QSize(26, 26));
            ui->comboBoxPort->setCurrentIndex(currentIndex);
//            ui->console->LOGI("comboBoxPort pressed!\r");
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MainWindow::on_comboBoxPort_activated(int index)
{
    on_pushButtonActive_clicked();
}

// apply config
void MainWindow::on_pushButtonActive_clicked()
{
    bool ok;
    QString portInfoQstring;
    QByteArray portInfoByteArray;
    char* portInfoStr;
    QString devStr;
    int indexTh;

    portInfoQstring   = ui->comboBoxPort->currentText();
    portInfoByteArray = portInfoQstring.toLocal8Bit();
    portInfoStr       = portInfoByteArray.data();

    indexTh = portInfoQstring.length();
    for(int i = 0; i < portInfoQstring.length(); i ++)
    {
        if(portInfoQstring[i] == '_')
        {
            indexTh = i;
        }
        if(i > indexTh && portInfoQstring[i] >= '0' && portInfoQstring[i] <= '9')
        {
            devStr.append(portInfoQstring[i]);
        }
    }
    ft_dev_index  = devStr.toInt(&ok,10);
//    ui->pushButtonActive->setIcon(qicon.fromTheme("media-playback-start"));
    ui->pushButtonActive->setIcon(QIcon(":/image/connect.png"));
    ui->pushButtonActive->setIconSize(QSize(30, 30));
    ui->console->LOGI("Config Port      @ %s, devIndex @ %d\r\n", portInfoStr, ft_dev_index);
}

void MainWindow::RvFlashInit()
{   
    QString baseAddr;

    configInit();

    if(flashCtl.proj != PEP)
    {
        flashCtl.address = 0x80000000;
    }
    baseAddr = baseAddr.sprintf("0x%X", flashCtl.address);
    ui->lineEditAddress->setText(baseAddr);

    on_lineEditAddress_editingFinished();

    consoleInit();

    stimerInit();

    statusBarInit();

    timerElapsedInit();

//    mstimerInit();
    ui->pushButtonActive->setToolTip("apply");
//    ui->pushButtonActive->setIcon(qicon.fromTheme("media-record"));
    ui->pushButtonActive->setIcon(QIcon(":/image/disconnect.png"));
    ui->pushButtonActive->setIconSize(QSize(26, 26));
    availableDevs(rvPortInfo, &rvDevNum);
    fillPortInfo();
    on_comboBoxPort_activated(0);
    fillFreqInfo();
}

