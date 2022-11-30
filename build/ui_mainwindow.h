/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>
#include <console.h>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionExit;
    QAction *actionSaveAs;
    QAction *actionAbout;
    QAction *actionHelp;
    QAction *actionConfig;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QPushButton *pushButtonSave;
    QPushButton *pushButtonClear;
    QHBoxLayout *horizontalLayout;
    QGroupBox *groupBoxTool;
    QPushButton *pushButtonAuto;
    QPushButton *pushButtonConnect;
    QPushButton *pushButtonExit;
    QPushButton *pushButtonProgram;
    QPushButton *pushButtonVerify;
    QPushButton *pushButtonRead;
    QPushButton *pushButtonErase;
    QCheckBox *checkBoxConnect;
    QCheckBox *checkBoxErase;
    QCheckBox *checkBoxProgram;
    QCheckBox *checkBoxVerify;
    QLabel *label;
    QLineEdit *lineEditAddress;
    QProgressBar *progressBar;
    QLineEdit *lineEditLength;
    QLabel *labelLength;
    QGroupBox *groupBoxCosole;
    QGridLayout *gridLayout_2;
    QLabel *dataConsoleLable;
    QTextEdit *textEdit;
    QLabel *cmdConsoleLable;
    Console *console;
    QLineEdit *lineEditDir;
    QPushButton *pushButtonOpen;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuAbout;
    QMenu *menuTool;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1015, 711);
        QFont font;
        font.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font.setPointSize(12);
        MainWindow->setFont(font);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/image/logo.ico"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        MainWindow->setIconSize(QSize(256, 256));
        MainWindow->setAnimated(true);
        actionOpen = new QAction(MainWindow);
        actionOpen->setObjectName(QString::fromUtf8("actionOpen"));
        QFont font1;
        font1.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font1.setPointSize(10);
        font1.setUnderline(false);
        font1.setKerning(true);
        actionOpen->setFont(font1);
        actionSave = new QAction(MainWindow);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        QFont font2;
        font2.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font2.setPointSize(10);
        actionSave->setFont(font2);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionExit->setFont(font2);
        actionSaveAs = new QAction(MainWindow);
        actionSaveAs->setObjectName(QString::fromUtf8("actionSaveAs"));
        actionSaveAs->setFont(font2);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        actionAbout->setFont(font2);
        actionHelp = new QAction(MainWindow);
        actionHelp->setObjectName(QString::fromUtf8("actionHelp"));
        actionHelp->setFont(font2);
        actionConfig = new QAction(MainWindow);
        actionConfig->setObjectName(QString::fromUtf8("actionConfig"));
        actionConfig->setFont(font2);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        pushButtonSave = new QPushButton(centralwidget);
        pushButtonSave->setObjectName(QString::fromUtf8("pushButtonSave"));
        pushButtonSave->setFont(font);

        gridLayout->addWidget(pushButtonSave, 0, 2, 1, 1);

        pushButtonClear = new QPushButton(centralwidget);
        pushButtonClear->setObjectName(QString::fromUtf8("pushButtonClear"));
        pushButtonClear->setFont(font);

        gridLayout->addWidget(pushButtonClear, 0, 3, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        groupBoxTool = new QGroupBox(centralwidget);
        groupBoxTool->setObjectName(QString::fromUtf8("groupBoxTool"));
        pushButtonAuto = new QPushButton(groupBoxTool);
        pushButtonAuto->setObjectName(QString::fromUtf8("pushButtonAuto"));
        pushButtonAuto->setGeometry(QRect(30, 430, 111, 41));
        pushButtonAuto->setFont(font);
        pushButtonConnect = new QPushButton(groupBoxTool);
        pushButtonConnect->setObjectName(QString::fromUtf8("pushButtonConnect"));
        pushButtonConnect->setGeometry(QRect(30, 130, 111, 41));
        pushButtonConnect->setFont(font);
        pushButtonConnect->setContextMenuPolicy(Qt::DefaultContextMenu);
        pushButtonConnect->setAutoExclusive(false);
        pushButtonConnect->setAutoDefault(false);
        pushButtonConnect->setFlat(false);
        pushButtonExit = new QPushButton(groupBoxTool);
        pushButtonExit->setObjectName(QString::fromUtf8("pushButtonExit"));
        pushButtonExit->setGeometry(QRect(30, 490, 111, 41));
        pushButtonExit->setFont(font);
        pushButtonProgram = new QPushButton(groupBoxTool);
        pushButtonProgram->setObjectName(QString::fromUtf8("pushButtonProgram"));
        pushButtonProgram->setGeometry(QRect(30, 310, 111, 41));
        pushButtonProgram->setFont(font);
        pushButtonVerify = new QPushButton(groupBoxTool);
        pushButtonVerify->setObjectName(QString::fromUtf8("pushButtonVerify"));
        pushButtonVerify->setGeometry(QRect(30, 370, 111, 41));
        pushButtonVerify->setFont(font);
        pushButtonRead = new QPushButton(groupBoxTool);
        pushButtonRead->setObjectName(QString::fromUtf8("pushButtonRead"));
        pushButtonRead->setGeometry(QRect(30, 190, 111, 41));
        pushButtonRead->setFont(font);
        pushButtonErase = new QPushButton(groupBoxTool);
        pushButtonErase->setObjectName(QString::fromUtf8("pushButtonErase"));
        pushButtonErase->setGeometry(QRect(30, 250, 111, 41));
        pushButtonErase->setFont(font);
        checkBoxConnect = new QCheckBox(groupBoxTool);
        checkBoxConnect->setObjectName(QString::fromUtf8("checkBoxConnect"));
        checkBoxConnect->setEnabled(false);
        checkBoxConnect->setGeometry(QRect(150, 140, 21, 24));
        checkBoxErase = new QCheckBox(groupBoxTool);
        checkBoxErase->setObjectName(QString::fromUtf8("checkBoxErase"));
        checkBoxErase->setEnabled(true);
        checkBoxErase->setGeometry(QRect(150, 260, 21, 24));
        checkBoxErase->setChecked(true);
        checkBoxProgram = new QCheckBox(groupBoxTool);
        checkBoxProgram->setObjectName(QString::fromUtf8("checkBoxProgram"));
        checkBoxProgram->setGeometry(QRect(150, 320, 21, 24));
        checkBoxProgram->setChecked(true);
        checkBoxVerify = new QCheckBox(groupBoxTool);
        checkBoxVerify->setObjectName(QString::fromUtf8("checkBoxVerify"));
        checkBoxVerify->setGeometry(QRect(150, 380, 21, 24));
        checkBoxVerify->setChecked(true);
        label = new QLabel(groupBoxTool);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(150, 19, 81, 31));
        QFont font3;
        font3.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font3.setPointSize(11);
        label->setFont(font3);
        lineEditAddress = new QLineEdit(groupBoxTool);
        lineEditAddress->setObjectName(QString::fromUtf8("lineEditAddress"));
        lineEditAddress->setGeometry(QRect(30, 20, 111, 31));
        lineEditAddress->setFont(font);
        lineEditAddress->setFrame(true);
        progressBar = new QProgressBar(groupBoxTool);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setEnabled(true);
        progressBar->setGeometry(QRect(30, 560, 111, 27));
        progressBar->setFont(font3);
        progressBar->setValue(0);
        progressBar->setInvertedAppearance(false);
        lineEditLength = new QLineEdit(groupBoxTool);
        lineEditLength->setObjectName(QString::fromUtf8("lineEditLength"));
        lineEditLength->setEnabled(false);
        lineEditLength->setGeometry(QRect(30, 70, 111, 31));
        lineEditLength->setFont(font);
        lineEditLength->setFrame(true);
        lineEditLength->setDragEnabled(false);
        lineEditLength->setReadOnly(true);
        lineEditLength->setClearButtonEnabled(false);
        labelLength = new QLabel(groupBoxTool);
        labelLength->setObjectName(QString::fromUtf8("labelLength"));
        labelLength->setEnabled(false);
        labelLength->setGeometry(QRect(150, 70, 71, 31));
        labelLength->setFont(font3);

        horizontalLayout->addWidget(groupBoxTool);

        groupBoxCosole = new QGroupBox(centralwidget);
        groupBoxCosole->setObjectName(QString::fromUtf8("groupBoxCosole"));
        gridLayout_2 = new QGridLayout(groupBoxCosole);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        dataConsoleLable = new QLabel(groupBoxCosole);
        dataConsoleLable->setObjectName(QString::fromUtf8("dataConsoleLable"));
        QFont font4;
        font4.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font4.setPointSize(11);
        font4.setBold(false);
        font4.setItalic(false);
        font4.setKerning(true);
        dataConsoleLable->setFont(font4);

        gridLayout_2->addWidget(dataConsoleLable, 0, 0, 1, 1);

        textEdit = new QTextEdit(groupBoxCosole);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        QFont font5;
        font5.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font5.setPointSize(12);
        font5.setBold(false);
        font5.setItalic(false);
        textEdit->setFont(font5);
        textEdit->setAcceptDrops(true);
        textEdit->setFrameShape(QFrame::WinPanel);

        gridLayout_2->addWidget(textEdit, 1, 0, 1, 1);

        cmdConsoleLable = new QLabel(groupBoxCosole);
        cmdConsoleLable->setObjectName(QString::fromUtf8("cmdConsoleLable"));
        QFont font6;
        font6.setFamily(QString::fromUtf8("\345\256\213\344\275\223"));
        font6.setPointSize(11);
        font6.setBold(false);
        font6.setItalic(false);
        cmdConsoleLable->setFont(font6);

        gridLayout_2->addWidget(cmdConsoleLable, 2, 0, 1, 1);

        console = new Console(groupBoxCosole);
        console->setObjectName(QString::fromUtf8("console"));
        console->setFont(font5);
        console->setStyleSheet(QString::fromUtf8(""));
        console->setFrameShape(QFrame::WinPanel);

        gridLayout_2->addWidget(console, 3, 0, 1, 1);


        horizontalLayout->addWidget(groupBoxCosole);

        horizontalLayout->setStretch(0, 3);
        horizontalLayout->setStretch(1, 10);

        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 4);

        lineEditDir = new QLineEdit(centralwidget);
        lineEditDir->setObjectName(QString::fromUtf8("lineEditDir"));
        lineEditDir->setFont(font);

        gridLayout->addWidget(lineEditDir, 0, 0, 1, 1);

        pushButtonOpen = new QPushButton(centralwidget);
        pushButtonOpen->setObjectName(QString::fromUtf8("pushButtonOpen"));
        pushButtonOpen->setFont(font);

        gridLayout->addWidget(pushButtonOpen, 0, 1, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        lineEditDir->raise();
        pushButtonOpen->raise();
        pushButtonClear->raise();
        pushButtonSave->raise();
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1015, 26));
        menubar->setFont(font);
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuFile->setFont(font3);
        menuAbout = new QMenu(menubar);
        menuAbout->setObjectName(QString::fromUtf8("menuAbout"));
        menuAbout->setFont(font3);
        menuTool = new QMenu(menubar);
        menuTool->setObjectName(QString::fromUtf8("menuTool"));
        menuTool->setFont(font3);
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        statusbar->setFont(font3);
        statusbar->setLayoutDirection(Qt::LeftToRight);
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuTool->menuAction());
        menubar->addAction(menuAbout->menuAction());
        menuFile->addAction(actionOpen);
        menuFile->addAction(actionSave);
        menuFile->addAction(actionSaveAs);
        menuFile->addAction(actionExit);
        menuAbout->addAction(actionHelp);
        menuAbout->addAction(actionAbout);
        menuTool->addAction(actionConfig);

        retranslateUi(MainWindow);

        pushButtonConnect->setDefault(false);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "RvFlash\347\274\226\347\250\213\345\267\245\345\205\267", nullptr));
        actionOpen->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200", nullptr));
        actionSave->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230", nullptr));
#if QT_CONFIG(shortcut)
        actionSave->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        actionSaveAs->setText(QCoreApplication::translate("MainWindow", "\345\217\246\345\255\230\344\270\272", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "\345\205\263\344\272\216", nullptr));
        actionHelp->setText(QCoreApplication::translate("MainWindow", "\344\275\277\347\224\250\350\257\264\346\230\216", nullptr));
#if QT_CONFIG(tooltip)
        actionHelp->setToolTip(QCoreApplication::translate("MainWindow", "\344\275\277\347\224\250\350\257\264\346\230\216", nullptr));
#endif // QT_CONFIG(tooltip)
        actionConfig->setText(QCoreApplication::translate("MainWindow", "\351\205\215\347\275\256", nullptr));
        pushButtonSave->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230", nullptr));
#if QT_CONFIG(tooltip)
        pushButtonClear->setToolTip(QCoreApplication::translate("MainWindow", "<html><head/><body><p>\346\270\205\351\231\244log\346\227\245\345\277\227</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        pushButtonClear->setText(QCoreApplication::translate("MainWindow", "\346\270\205\351\231\244", nullptr));
        pushButtonAuto->setText(QCoreApplication::translate("MainWindow", "\350\207\252\345\212\250", nullptr));
        pushButtonConnect->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245", nullptr));
        pushButtonExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        pushButtonProgram->setText(QCoreApplication::translate("MainWindow", "\347\274\226\347\250\213", nullptr));
        pushButtonVerify->setText(QCoreApplication::translate("MainWindow", "\346\240\241\351\252\214", nullptr));
        pushButtonRead->setText(QCoreApplication::translate("MainWindow", "\350\257\273\345\207\272", nullptr));
        pushButtonErase->setText(QCoreApplication::translate("MainWindow", "\346\223\246\351\231\244", nullptr));
        checkBoxConnect->setText(QString());
        checkBoxErase->setText(QString());
        checkBoxProgram->setText(QString());
        checkBoxVerify->setText(QString());
        label->setText(QCoreApplication::translate("MainWindow", "\350\265\267\345\247\213\345\234\260\345\235\200", nullptr));
        lineEditAddress->setText(QCoreApplication::translate("MainWindow", "0x5000000", nullptr));
        lineEditLength->setText(QCoreApplication::translate("MainWindow", "0x500FFFF", nullptr));
        labelLength->setText(QCoreApplication::translate("MainWindow", "\346\210\252\346\255\242\345\234\260\345\235\200", nullptr));
        groupBoxCosole->setTitle(QString());
        dataConsoleLable->setText(QCoreApplication::translate("MainWindow", "\346\225\260\346\215\256\346\230\276\347\244\272", nullptr));
        textEdit->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'\345\256\213\344\275\223'; font-size:12pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-family:'Arial';\"><br /></p></body></html>", nullptr));
        cmdConsoleLable->setText(QCoreApplication::translate("MainWindow", "\346\216\247\345\210\266\345\217\260", nullptr));
        pushButtonOpen->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        menuAbout->setTitle(QCoreApplication::translate("MainWindow", "\345\270\256\345\212\251", nullptr));
        menuTool->setTitle(QCoreApplication::translate("MainWindow", "\345\267\245\345\205\267", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
