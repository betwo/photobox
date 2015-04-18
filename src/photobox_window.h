#ifndef PHOTOBOXWINDOW_H
#define PHOTOBOXWINDOW_H

#include <QMainWindow>
#include "ui_photobox.h"
#include "pixmap.hpp"
#include <QTimer>

class QGraphicsBlurEffect;
class QParallelAnimationGroup;

class PhotoboxWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PhotoboxWindow(QWidget *parent = 0);
    ~PhotoboxWindow();

signals:
    void endPictureTakingAnimations();
    void takePicture();

public slots:
    void showPreview(QImage* image);
    void showImage(QImage* image);

    void keyReleaseEvent(QKeyEvent* e);

    void startPictureTakingAnimations();
    void done();
    void updateTime();

private:
    QParallelAnimationGroup * addTextAnimation(const std::string &text, double scale = 80);
    QParallelAnimationGroup * hideTextAnimation(const std::string &text);

private:
    Ui::Photobox* ui;

    QGraphicsBlurEffect* shot_effect;

    Pixmap* last_image;
    Pixmap* preview;

    std::map<std::string, QGraphicsTextItem*> text;

    int64_t show_time;

    u_int64_t start_time;
    QTimer time_left_timer;
    QGraphicsTextItem* time_left_text;
};

#endif // PHOTOBOXWINDOW_H
