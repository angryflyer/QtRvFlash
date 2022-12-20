#include "usb_listener.h"
#include <QApplication>
#include <QDebug>

bool usb_listener::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    unsigned int msgType = msg->message;
    if(msgType == WM_DEVICECHANGE) {
        emit DeviceChangeCbk();
        if(msg->wParam == DBT_DEVICEARRIVAL)
        {
            qDebug("usb in");
            emit DevicePlugIn();         //触发信号
        }
        if(msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            qDebug("usb out");
            emit DevicePlugOut();         //触发信号
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

//触发信号
void usb_listener::EmitMySignal()
{
    emit DevicePlugIn();
}
