#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>

extern "C" {
#include <gphoto2/gphoto2.h>
}

class EOSCamera : public QObject
{
    Q_OBJECT

public:
    EOSCamera();
    ~EOSCamera();

    void testLoop();

    void takePicture();
    void takePreviewImage();

    void autoFocus();

signals:
    void newPreview(QImage image);
    void newImage(QImage image);

private:
    Camera	*canon;
    GPContext *canoncontext;
};

#endif // CAMERA_H
