#include "photobox_window.h"
#include <QMainWindow>
#include <iostream>
#include "camera.h"
#include "director.h"
#include <QtConcurrent/QtConcurrentRun>
#include "arduino_button.h"
#include <thread>

int main(int argc, char *argv[])
{
    ArduinoButton button;
    button.run();

//    while(1) {
//        std::cout << ".." << std::endl;
//        std::this_thread::sleep_for(std::chrono::microseconds(1000000));
//    }
//    return 0;

    EOSCamera camera;

    QThread director_thread;
    Director director(camera);
    director.moveToThread(&director_thread);


    QApplication app(argc, argv);
    PhotoboxWindow box;

    QtConcurrent::run([&director]() {
        director.run();
    });
    director_thread.start();

    QObject::connect(&camera, SIGNAL(newPreview(QImage*)), &box, SLOT(showPreview(QImage*)));
    QObject::connect(&camera, SIGNAL(newImage(QImage*)), &box, SLOT(showImage(QImage*)));

    QObject::connect(&box, SIGNAL(takePicture()), &box, SLOT(startPictureTakingAnimations()));
    QObject::connect(&button, SIGNAL(buttonPressed()), &box, SLOT(startPictureTakingAnimations()));

    QObject::connect(&box, SIGNAL(endPictureTakingAnimations()), &director, SLOT(takePicture()), Qt::QueuedConnection);
//    QObject::connect(&button, SIGNAL(buttonPressed()), &director, SLOT(takePicture()), Qt::DirectConnection);



    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));


    button.run();
    box.show();

    app.exec();

    button.stop();
    director.stop();
    director_thread.quit();

    return 0;
}

#include "photobox.moc"
