#ifndef PHOTOBOXWINDOW_H
#define PHOTOBOXWINDOW_H

#include <QMainWindow>
#include "ui_photobox.h"
#include "pixmap.hpp"

class PhotoboxWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PhotoboxWindow(QWidget *parent = 0);
    ~PhotoboxWindow();

signals:
    void takePicture();

public slots:
    void showPreview(QImage* image);
    void showImage(QImage* image);

private:
    Ui::Photobox* ui;

    Pixmap* last_image;
    Pixmap* preview;
};

#endif // PHOTOBOXWINDOW_H
