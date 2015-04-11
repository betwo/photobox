#include "photobox_window.h"
#include <QMainWindow>
#include <iostream>
#include "camera.h"
#include "director.h"
#include <QtConcurrent/QtConcurrentRun>

int main(int argc, char *argv[])
{
    EOSCamera camera;
    Director director(camera);

    QApplication app(argc, argv);
    PhotoboxWindow box;

    QtConcurrent::run([&director]() {
        director.run();
    });

    QObject::connect(&camera, SIGNAL(newPreview(QImage*)), &box, SLOT(showPreview(QImage*)));
    QObject::connect(&camera, SIGNAL(newImage(QImage*)), &box, SLOT(showImage(QImage*)));

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
    QObject::connect(&box, SIGNAL(takePicture()), &director, SLOT(takePicture()));

    box.show();

    app.exec();

    director.stop();

    return 0;
}

#include "photobox.moc"
