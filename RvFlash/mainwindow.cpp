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

    write_buffer = new char[bufsize];
    read_buffer  = new char[bufsize];

    flashCtl.writeBufferAddr = (ULONGLONG*)write_buffer;
    flashCtl.readBufferAddr  = (ULONGLONG*)read_buffer;

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

//    QMessageBox::information(NULL,QStringLiteral("提示！"),QStringLiteral("连接成功！"),QStringLiteral("确定"));
    QMessageBox::information(NULL,QStringLiteral("Notice！"),QStringLiteral("Connect succeeded！"),QStringLiteral("Confirm"));
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

    //将fileName转为char类型存到flashCtl.fileName
    QByteArray filename = fileName.toLocal8Bit();
    flashCtl.fileName = filename.data();
    flashCtl.fileSize = file.size();//get file size/lenth
    flashCtl.currentfileSize = file.size();//backup file.size

    //filesize up to mtpsize
    if(flashCtl.fileSize > mtpsize)
    {
//        QMessageBox::critical(NULL,QStringLiteral("警告！"),QStringLiteral("仅显示前64KB数据！"),QStringLiteral("确定"));
        QMessageBox::critical(NULL,QStringLiteral("Warning！"),QStringLiteral("Only display 64KB data！"),QStringLiteral("Confirm"));
        display_size = mtpsize;
    } else
    {
        display_size = flashCtl.fileSize;
    }

    float lenth = (float)flashCtl.fileSize / 1024;
    ui->console->LOGI("fileName=%s\nfileSize=%.1fK (%dBYTES)\n", flashCtl.fileName, lenth, flashCtl.fileSize);

    //读取数据
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
    QMessageBox::information(NULL,QStringLiteral("About"),QStringLiteral("Name   ：RvFlash\nVersion：V1.2\nTime    ：2022-12-13"),QStringLiteral("Confirm"));
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

    //将fileName转为char类型存到flashCtl.fileName
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

    //读取数据
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
    timerElapsedLable->setText(QStringLiteral("Time consuming："));

}


void MainWindow::timerElapsed()
{
//    QString timeconsumption = QStringLiteral("耗时：");
    QString timeconsumption = QStringLiteral("Time consuming：");

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

    if(flashCtl.proj == 0)
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
            QMessageBox::warning(NULL,QStringLiteral("Warning！"),QStringLiteral("Size over limit！"),QStringLiteral("Confirm"));
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

    ft_status = ft_dev_init(waitfreq);
    ft_status &= ft_close();

    //try to recover when connect fail
    while(false == ft_status)
    {
        ft_status = ft_dev_init(waitfreq);
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
        flashCtl.connectStatus = (flashCtl.proj == 0) ? chipConnect() : true;
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

    //填充tempBuffer，全0xff
    memset(tempBuffer, 0xff, mtpsize);//fill buffer with 0xff

    //读取数据
    QDataStream readDataStream(&file);
    readDataStream.readRawData((char*)tempBuffer,fileSize);//读取数据到缓存

    for (int i = 0; i < fileSize; i++) {
       if (std::strncmp(&tempBuffer[i], "SPEED=", 6) == 0) {
         waitfreq = std::atoi(&tempBuffer[i] + 6);
       } else if (std::strncmp(&tempBuffer[i], "WAITTIME=", 9) == 0) {
         waitlevel = std::atoi(&tempBuffer[i] + 9);
       } else if (std::strncmp(&tempBuffer[i], "PROJ=", 5) == 0) {
         flashCtl.proj = std::atoi(&tempBuffer[i] + 5);
       }
     }

    ui->console->LOGI(QString(QStringLiteral("Config Speed     @ %dKHz\n")).toStdString().data(),waitfreq);//rvlink/test io clk frequency
    ui->console->LOGI(QString(QStringLiteral("Config Gap       @ %dus\n")).toStdString().data(),waitlevel);//stable time of read data
    if(flashCtl.proj == 0)
    {
       configMsg = "Config PROJ      @ PEP\n";
    }
    else
    {
       configMsg = "Config PROJ      @ 506\n";
    }
    ui->console->LOGI(configMsg);//stable time of read data

    if(flashCtl.proj == 0)
    {
        ui->pushButtonConnect->setEnabled(true);
        ui->pushButtonRead->setEnabled(true);
        ui->pushButtonErase->setEnabled(true);
        ui->checkBoxErase->setEnabled(true);
        ui->checkBoxErase->setChecked(true);
        ui->checkBoxVerify->setEnabled(true);
        ui->checkBoxVerify->setChecked(true);
        flashCtl.autoEraseFlag = true;
        flashCtl.autoVerifyFlag = true;
    }
    else
    {
        ui->pushButtonConnect->setEnabled(false);
        ui->pushButtonRead->setEnabled(false);
        ui->pushButtonErase->setEnabled(false);
        ui->checkBoxErase->setEnabled(false);
        ui->checkBoxErase->setChecked(false);
        ui->checkBoxVerify->setEnabled(true);
        ui->checkBoxVerify->setChecked(false);
        flashCtl.autoEraseFlag = false;
        flashCtl.autoVerifyFlag = false;
    }

    return true;
}

void MainWindow::RvFlashInit()
{   
    QString baseAddr;

    configInit();

    if(flashCtl.proj == 1)
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
}
