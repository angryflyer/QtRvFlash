#include <QFile>
#include <QFileDialog>
#include <QTextStream>
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
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "rvlink_ftd2xx.h"
#include "flashrom.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
//    , m_console(new Console)
{
    ui->setupUi(this);

    my_thread = new FlashThread(this);

    stimer = new QTimer(this);

    mstimer = new QTimer(this);

    timeDataLable = new QLabel(this);

    statusLabel = new QLabel(this);

    timerElapsedLable = new QLabel(this);

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

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8")); //设置编码

    RvFlashInit();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete my_thread;
    delete stimer;
    delete timeDataLable;
    delete statusLabel;
}

void MainWindow::on_pushButtonExit_clicked()
{
    MainWindow::close();
}

void MainWindow::on_pushButtonConnect_clicked()
{
    if(false == checkConnect())
    {
        return;
    }

    ft_open();
    process_reset();
    ft_close();

    QMessageBox::information(NULL,QStringLiteral("提示！"),QStringLiteral("连接成功！"),QStringLiteral("确定"));
}

void MainWindow::on_pushButtonRead_clicked()
{
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
//    ft_dev_init(waitfreq);
////    ReadDataAndCheckACK(addr, &data, 40, 64);
//    rv_read(addr, &data);
//    ft_close();
//    ui->console->LOGI( "read address=0x%x, read data=0x%llx\n", addr, data);
//}

void MainWindow::on_pushButtonErase_clicked()
{
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
    //ULONGLONG dwLegth = filesize64;
    char tempBuffer[mtpsize];
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

    //将fileName转为char类型存到flashCtl.fileName
    QByteArray filename = fileName.toLocal8Bit();
    flashCtl.fileName = filename.data();
    flashCtl.fileSize = file.size();//get file size/lenth
    flashCtl.currentfileSize = file.size();//backup file.size

    //filesize up to mtpsize
    if(flashCtl.fileSize > mtpsize)
    {
        QMessageBox::critical(NULL,QStringLiteral("警告！"),QStringLiteral("文件大小超过flash容量,超出数据将会丢失！"),QStringLiteral("确定"));
        flashCtl.fileSize = mtpsize;
        flashCtl.currentfileSize = mtpsize;
    }

    float lenth = (float)flashCtl.fileSize / 1024;
    ui->console->LOGI("fileName=%s\nfileSize=%.1fK (%dBYTES)\n", flashCtl.fileName, lenth, flashCtl.fileSize);

    //填充mtpbuffer，全0xff
    memset(flashCtl.mtpWriteBuffer, 0xff, mtpsize);//fill buffer with 0xff
    memset(tempBuffer, 0xff, mtpsize);//fill buffer with 0xff

    //读取数据
    QDataStream readDataStream(&file);
    statusLabel->setText(QStringLiteral("准备读取数据"));
    QApplication::processEvents();
    readDataStream.readRawData((char*)flashCtl.mtpWriteBuffer,flashCtl.fileSize);//读取数据到缓存
    statusLabel->setText(QStringLiteral("读取数据完毕"));
    QApplication::processEvents();

    memcpy(tempBuffer, (char*)flashCtl.mtpWriteBuffer, flashCtl.fileSize);
    QByteArray *tempByte = new QByteArray(tempBuffer,flashCtl.fileSize);
    QString *tempStr = new QString(tempByte->toHex().toUpper());

    ULONGLONG addrData = flashCtl.address;

    QString addrStr;

    //显示文件内容
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
            addrStr = "0x";//indicate hex string
            addrStr = addrStr.append(QString(QByteArray::number(addrData, 16).toUpper()));//add address information after "0x"
            addrStr = addrStr.append(": ");//add ":" after address
            tempStr->insert(i-2,addrStr);
            if(addrData < 0x10000000) i += 11;
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
    statusLabel->setText(QStringLiteral("显示数据完毕"));
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
        out.writeRawData((char*)flashCtl.readOutBuffer, mtplenth);
    }

    file.close();
}

void MainWindow::about()
{
    QMessageBox::information(NULL,QStringLiteral("关于"),QStringLiteral("名称：RvFlash\n版本：V1.1\n时间：2022-04-29"),QStringLiteral("确定"));
}

void MainWindow::help()
{
    QDesktopServices helpDoc;
    QString strDoc = QString(qApp->applicationDirPath()).append(QStringLiteral("/doc/RvFlash使用说明.pdf"));
    if (!helpDoc.openUrl(QUrl::fromLocalFile(strDoc))) {
        QMessageBox::information(NULL,QStringLiteral("警告！"),QStringLiteral("无法打开帮助文档!"),QStringLiteral("确定"));
    }
}

void MainWindow::config()
{
    if(configInit()) {
        QMessageBox::information(NULL,QStringLiteral("通知"),QStringLiteral("配置成功!"),QStringLiteral("确定"));
    } else {
        QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("配置失败，没有找到配置文件!\n请检查目录下RvFlash.ini！"),QStringLiteral("确定"));
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
    ui->console->LOGI(QString(QStringLiteral("设置起始地址 @ 0x%llx\n")).toStdString().data(), flashCtl.address);

//    ULONGLONG sector = 0;
//    DWORD sectorlocat = 0;
//    ULONGLONG dwLegth = flashCtl.fileSize;
//    sector = ceil((float)dwLegth / (8 * 1024));
//    sectorlocat = (float)(flashCtl.address - STATION_SDIO_MTP_ADDR) / mtpsecsize;
//    sectorlocat = sectorlocat + 1;
//    fprintf(stderr,  "flashCtl.address @ 0x%llx sector & %lld sectorlocat @ %ld\n", flashCtl.address, sector, sectorlocat);
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

void MainWindow::doSomething(void)
{
    my_thread->start();
}

void MainWindow::updateProgress(int p, const char* msg, int type)
{
    static time_t ltime;
    ltime = time(NULL);
    if(threadstart == (type & 0x3))
    ui->console->LOGI(msg, asctime(localtime(&ltime)));

    ui->progressBar->setValue(p);

    ltime = time(NULL);
    if(threadend == (type & 0x3))
    ui->console->LOGI(msg, asctime(localtime(&ltime)));
    //ui->console->LOGI(msg);
    if((threadstart | writetype) == type)  statusLabel->setText(QStringLiteral("正在写入"));
    if((threadend | writetype) == type)  statusLabel->setText(QStringLiteral("写入完成"));
    if((threadworking | writeerror) == type)
    {
        statusLabel->setText(QStringLiteral("写入错误"));
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("写入错误！"),QStringLiteral("确定"));
    }

    if((threadstart | readtype) == type)  statusLabel->setText(QStringLiteral("正在读取"));
    if((threadend | readtype) == type)  statusLabel->setText(QStringLiteral("读取完成"));
    if((threadworking | readerror) == type)
    {
        statusLabel->setText(QStringLiteral("校验错误"));
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("校验错误！"),QStringLiteral("确定"));
    }

    if((threadstart | readouttype) == type)  statusLabel->setText(QStringLiteral("正在读取"));
    if((threadend | readouttype) == type)
    {
        statusLabel->setText(QStringLiteral("读取完成"));
        dataToConsole();
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
        QMessageBox::information(NULL,QStringLiteral("通知！"),QStringLiteral("读取完成！"),QStringLiteral("确定"));
    }

    if((threadworking | readouterror) == type)
    {
        statusLabel->setText(QStringLiteral("读出错误"));
        //ui->console->LOGI("p = %d\n", p);
        ui->progressBar->setValue(p);
        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("读出错误！"),QStringLiteral("确定"));
    }

    if((threadstart | verifytype) == type) statusLabel->setText(QStringLiteral("正在校验"));
    if((threadend | verifytype) == type)  statusLabel->setText(QStringLiteral("校验完成"));
    if((threadend | verifyerror) == type)
    {
        statusLabel->setText(QStringLiteral("校验错误"));
        QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("校验错误！！！"),QStringLiteral("确定"));
    }

    if((threadstart | erasetype) == type) statusLabel->setText(QStringLiteral("正在擦除"));
    if((threadend | erasetype) == type)  statusLabel->setText(QStringLiteral("擦除完成"));

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

    timerElapsedLable->setText(QStringLiteral("耗时："));
}


void MainWindow::timerElapsed()
{
    QString timeconsumption = QStringLiteral("耗时：");

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
    char tempBuffer[mtpsize];
    ULONGLONG addrData = flashCtl.address;
    memcpy(tempBuffer, (char*)flashCtl.readOutBuffer, mtpsize);
    QByteArray *tempByte = new QByteArray(tempBuffer, mtpsize);
    QString *tempStr = new QString(tempByte->toHex().toUpper());
    QString addrStr;
    //显示文件内容
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
            addrStr = "0x";//indicate hex string
            addrStr = addrStr.append(QString(QByteArray::number(addrData, 16).toUpper()));//add address information after "0x"
            addrStr = addrStr.append(": ");//add ":" after address
            tempStr->insert(i-2,addrStr);
            if(addrData < 0x10000000) i += 11;
            else i += 12;
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
//    ULONGLONG mtpoffset = 0;

    //recover filesize value
    flashCtl.fileSize = flashCtl.currentfileSize;

    dwLegth = filesize64 + ceil((float)(mtpoffset) / 8);
    dwLegth = (dwLegth * 8);//calculate requred space = offset + filesize

    if(0 != (flashCtl.address % 8))//align with 8bytes
    {
        QMessageBox::critical(NULL,QStringLiteral("错误！"),QStringLiteral("地址需按8字节对齐！"),QStringLiteral("确定"));
        return false;
    }

    if(dwLegth > STATION_SDIO_MTP_WIDTH_IN_BYTE)
    {
        flashCtl.fileSize = mtplenth;
        if((true == flashCtl.writeFlag) || (true == flashCtl.autoFlag))
        {
            QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("文件超出flash容量限制,超出部分将会忽略！"),QStringLiteral("确定"));
        }
        //return false;
    }
//    else
//    {
//        flashCtl.fileSize = flashCtl.currentfileSize;
//    }

    return true;
}

bool MainWindow::checkConnect()
{
    BOOL ft_status = true;
    BYTE ft_connect_count = 0;
    static time_t ltime;

    ft_status = ft_dev_init(waitfreq);
    ft_status |= ft_close();

    //try to recover when connect fail
    while(false == ft_status)
    {
        ft_status = ft_dev_init(waitfreq);
        ft_status |= ft_close();
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
                QMessageBox::information(NULL,QStringLiteral("通知！"),QStringLiteral("RvFlash重连成功！"),QStringLiteral("确定"));
                //重置状态栏和进度条
                statusLabel->setText(QStringLiteral("*已恢复连接*"));
                ui->progressBar->setValue(0);
                flashctl_reset();
            }
        }
        if(ft_connect_count == 10)
        {
            QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("RvFlash连接失败,请重新连接！"),QStringLiteral("确定"));
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
        flashCtl.connectStatus = chipConnect();
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
        QMessageBox::warning(NULL,QStringLiteral("警告！"),QStringLiteral("请加载程序文件！！！"),QStringLiteral("确定"));
        return false;
    }
    return true;
}

void MainWindow::consoleInit()
{
    QFont qf;
    qf.setPointSize(12);
    //qf.setFamily("sans-serif");
    qf.setFamily("Source Code Pro");//字体
    qf.setBold(true);
    //qf.setPixelSize(18);//文字像素大小
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
}

bool MainWindow::configInit()
{
    char tempBuffer[mtpsize];
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

    //填充tempBuffer，全0xff
    memset(tempBuffer, 0xff, mtpsize);//fill buffer with 0xff

    //读取数据
    QDataStream readDataStream(&file);
    readDataStream.readRawData((char*)tempBuffer,fileSize);//读取数据到缓存

    for (int i = 0; i < fileSize; i++) {
       if (std::strncmp(&tempBuffer[i], "SPEED=", 6) == 0) {
         waitfreq = std::atoi(&tempBuffer[i] + 6);
       } else if (std::strncmp(&tempBuffer[i], "WAITTIME=", 9) == 0) {
         waitlevel = std::atoi(&tempBuffer[i]+9);
       }
     }

    ui->console->LOGI(QString(QStringLiteral("设置通信频率 @ %dKHz\n")).toStdString().data(),waitfreq);//rvlink/test io clk frequency
    ui->console->LOGI(QString(QStringLiteral("设置等待时间 @ %dus\n")).toStdString().data(),waitlevel);//stable time of read data
    return true;
}

void MainWindow::RvFlashInit()
{
    on_lineEditAddress_editingFinished();

    consoleInit();

    stimerInit();

    statusBarInit();

    timerElapsedInit();

//    mstimerInit();

    configInit();
}
