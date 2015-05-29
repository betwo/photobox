#include "photobox_window.h"
#include <QMainWindow>
#include <iostream>
#include "camera.h"
#include "director.h"
#include <QtConcurrent/QtConcurrentRun>
#include "arduino_button.h"
#include <thread>

#define USE_BUTTON 1

int main(int argc, char *argv[])
{

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

    QObject::connect(&camera, SIGNAL(newPreview(QImage)), &box, SLOT(showPreview(QImage)));
    QObject::connect(&camera, SIGNAL(newImage(QImage)), &box, SLOT(showImage(QImage)));

    QObject::connect(&box, SIGNAL(takePicture()), &box, SLOT(startPictureTakingAnimations()));

    QObject::connect(&box, SIGNAL(endPictureTakingAnimations()), &director, SLOT(takePicture()), Qt::QueuedConnection);
    QObject::connect(&director, SIGNAL(doneTakingPicture()), &box, SLOT(allowTakingPicture()));

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));


#if USE_BUTTON
    ArduinoButton button;
    QObject::connect(&button, SIGNAL(buttonPressed()), &box, SLOT(startPictureTakingAnimations()));

    button.run();
#endif

    box.show();

    app.exec();

#if USE_BUTTON
    button.stop();
#endif

    director.stop();
    director_thread.quit();

    return 0;
}

#include "photobox.moc"
