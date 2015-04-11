#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <QObject>
#include <mutex>

class EOSCamera;

class Director : public QObject
{
    Q_OBJECT

public:
    Director(EOSCamera& cam);

    void run();
    void stop();

public slots:
    void takePicture();

private:
    EOSCamera& cam;

    std::mutex mutex;
    bool running;
};

#endif // DIRECTOR_H
