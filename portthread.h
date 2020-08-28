#ifndef PORTTHREAD_H
#define PORTTHREAD_H

#include <QObject>
#include <QThread>
#include <QtSerialPort>
#include <QTimer>

class portthread : public QThread
{
    Q_OBJECT
public:
    portthread();
protected:
    void run();
private:
    QTimer *t_2;
};

#endif // PORTTHREAD_H
