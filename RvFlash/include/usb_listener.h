#ifndef USB_LISTENER_H
#define USB_LISTENER_H

#include <QWidget>
#ifdef _WIN32
#include <windows.h>
#endif
#include <QAbstractNativeEventFilter>
#include <dbt.h>

class usb_listener:public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    void EmitMySignal();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);

signals:
    void DeviceChangeCbk();
    void DevicePlugIn();
    void DevicePlugOut();
};

#endif // USB_LISTENER_H
