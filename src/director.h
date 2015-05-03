#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <QObject>
#include <mutex>
#include <condition_variable>

class EOSCamera;

class Director : public QObject
{
    Q_OBJECT

public:
    Director(EOSCamera& cam);

    void run();
    void stop();

private:
    void setPreview(bool p);

public slots:
    void takePicture();

signals:
    void doneTakingPicture();

private:
    EOSCamera& cam;

    std::mutex mutex;
    std::condition_variable cond_picture_possible;
    std::condition_variable cond_preview_possible;

    bool running;

    bool is_preview_running;
    bool is_picture_requested;
};

#endif // DIRECTOR_H
