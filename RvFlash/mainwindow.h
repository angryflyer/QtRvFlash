#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QLabel>
#include "flashthread.h"
#include "usb_listener.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

//class Console;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);    
    ~MainWindow();

protected:

    void stimerInit();

    void mstimerInit();

    void timerElapsedInit();

    void statusBarInit();

    void dataToConsole();

    bool checkStatus();

    bool checkConnect();

    bool checkCurrentFile();

    void consoleInit();

    bool configInit();

    void RvFlashInit();

private slots:

    void on_pushButtonConnect_clicked();

    void on_pushButtonRead_clicked();

    void on_pushButtonErase_clicked();

    void on_pushButtonProgram_clicked();

    void on_pushButtonVerify_clicked();

    void on_pushButtonRun_clicked();

    void on_pushButtonAuto_clicked();

    void on_pushButtonOpen_clicked();

    void on_pushButtonSave_clicked();

    void open();

    void save();

    void saveas();

    void about();

    void help();

    void config();

    void on_pushButtonClear_clicked();

    void on_lineEditAddress_editingFinished();

    void on_lineEditDir_editingFinished();

    void on_checkBoxErase_stateChanged(int arg1);

    void on_checkBoxProgram_stateChanged(int arg1);

    void on_checkBoxVerify_stateChanged(int arg1);

    void on_checkBoxRun_stateChanged(int arg1);

    void doSomething(void);

    void updateProgress(int p, float speed, const char* msg, int type);

    void timerUpdate();

    void timerElapsed();

    void consoleDebug(const QByteArray &data);

private:
    Ui::MainWindow *ui;
    QString currentFile;
    FlashThread* my_thread;
    QTimer *stimer;//S
    QTimer *mstimer;//ms
    QLabel *timeDataLable;
    QLabel *statusLabel;
    QLabel *timerElapsedLable;

    QTime baseTime;
    QString showStr;
    usb_listener *m_usb_listener = Q_NULLPTR;
//    Console *m_console = nullptr;
    char *write_buffer;
    char *read_buffer;
//    char *cmd_buffer;
    BYTE cmd_buffer_index = 0;
    bool cmd_debug = false;
//    bool ft_init = false;
};
#endif // MAINWINDOW_H
