#include "arduino_button.h"

#include <QtConcurrent/QtConcurrentRun>

using namespace::boost::asio;

// Base serial settings
namespace {
serial_port_base::baud_rate BAUD(9600);
serial_port_base::flow_control FLOW( serial_port_base::flow_control::none );
serial_port_base::parity PARITY( serial_port_base::parity::none );
serial_port_base::stop_bits STOP( serial_port_base::stop_bits::one );
serial_port_base::character_size CHARSIZE(8U);
}

ArduinoButton::ArduinoButton()
    : port( io, "/dev/ttyACM0" ), running(false)
{
    // Setup port - base settings
    port.set_option( BAUD );
    port.set_option( FLOW );
    port.set_option( PARITY );
    port.set_option( STOP );
    port.set_option( CHARSIZE );
}

ArduinoButton::~ArduinoButton()
{
    stop();
}

void ArduinoButton::run()
{
    running = true;

    QtConcurrent::run([this]() {
        char button_state;
        int num;

        while(running){
            num = read(port,buffer(&button_state,1));
            if(button_state == '1') {
                emit buttonPressed();
            }
        }
    });
}

void ArduinoButton::stop()
{
    running = false;
}

#include "moc_arduino_button.cpp"
