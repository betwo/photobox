#ifndef ARDUINOBUTTON_H
#define ARDUINOBUTTON_H

#include <QObject>
#include <boost/asio.hpp>

class ArduinoButton : public QObject
{
    Q_OBJECT

public:
    ArduinoButton();
    ~ArduinoButton();

    void run();
    void stop();

signals:
    void buttonPressed();

private:
    boost::asio::io_service io;
    boost::asio::serial_port port;

    bool running;
};

#endif // ARDUINOBUTTON_H
