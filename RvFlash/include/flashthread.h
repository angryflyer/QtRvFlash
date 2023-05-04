#pragma once
#include <QThread>
#include "flashrom.h"

enum threadStatus
{
    threadstart,
    threadworking,
    threadend
};

class MainWindow;
class FlashThread : public QThread
{
    Q_OBJECT

public:

    FlashThread(MainWindow* creator, QObject* parent = NULL);

signals:
    void progress(int p, float speed, const char* msg, int type);

protected:

    void run() override;

    void flashErase();

    BOOL flashWrite();

    BOOL flashWriteBurst();

    BOOL flashReadOut();

    BOOL flashRead();

    BOOL flashVerify();

    void flashAuto();

    MainWindow* m_thread_creator;
};

