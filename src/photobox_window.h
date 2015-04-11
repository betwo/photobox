#ifndef PHOTOBOXWINDOW_H
#define PHOTOBOXWINDOW_H

#include <QMainWindow>
#include "ui_photobox.h"
#include "pixmap.hpp"

class QGraphicsBlurEffect;

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

    void keyReleaseEvent(QKeyEvent* e);

private:
    void doTakePicture();

private:
    Ui::Photobox* ui;

    QGraphicsBlurEffect* shot_effect;

    Pixmap* last_image;
    Pixmap* preview;

    QGraphicsTextItem* text;
};

#endif // PHOTOBOXWINDOW_H
