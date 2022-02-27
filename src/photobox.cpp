#include "photobox_window.h"
#include <QMainWindow>
#include <iostream>
#include "camera.h"
#include "director.h"
#include <QtConcurrent/QtConcurrentRun>
#include "arduino_button.h"
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define USE_BUTTON 0

static bool is_writable_directory(const std::string& path)
{
    struct stat path_stat;
    stat(path.c_str(), &path_stat);
    return S_ISDIR(path_stat.st_mode) && (access(path.c_str(), W_OK) == 0);
}

int main(int argc, char *argv[])
{
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " output-directory" << "\nWhere the output-directory argument must be a valid directory." << std::endl;
        return 1;
    }

    std::string output_dir = argv[1];
    if(!is_writable_directory(output_dir)){
        std::cerr << output_dir << " is not a writable directory." << std::endl;
        return 1;
    }
    if(output_dir.back() != '/') {
        output_dir += "/";
    }

    EOSCamera camera(output_dir);

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
