#ifndef PHOTOBOXWINDOW_H
#define PHOTOBOXWINDOW_H

#include <QMainWindow>
#include "ui_photobox.h"

class PhotoboxWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PhotoboxWindow(QWidget *parent = 0);
    ~PhotoboxWindow();

signals:
    void takePicture();

public slots:
    void showImage(QImage* image);

private:
    Ui::Photobox* ui;
};

#endif // PHOTOBOXWINDOW_H
