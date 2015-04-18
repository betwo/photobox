#include "photobox_window.h"
#include <iostream>
#include <QGraphicsPixmapItem>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QKeyEvent>
#include <QGraphicsBlurEffect>
#include <QGraphicsDropShadowEffect>
#include <QtConcurrent/QtConcurrent>
#include <thread>
#include <chrono>
#include <QTextBlockFormat>
#include <QTextCursor>

PhotoboxWindow::PhotoboxWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Photobox),
      last_image(nullptr), preview(nullptr), time_left_text(nullptr)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);

    ui->graphicsView->setScene(new QGraphicsScene);
    ui->graphicsView->setBackgroundBrush(QBrush(Qt::black));

    shot_effect = new QGraphicsBlurEffect;

    showFullScreen();
}

PhotoboxWindow::~PhotoboxWindow()
{
    delete ui;
}

void PhotoboxWindow::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Space) {
        emit takePicture();
    }
}

QParallelAnimationGroup * PhotoboxWindow::addTextAnimation(const std::string& txt, double scale)
{
    if(text[txt] == nullptr) {
        auto t  = new QGraphicsTextItem(QString::fromStdString(txt));
        text[txt] = t;

        QFont serifFont("Arial", scale, QFont::Bold);
        t->setFont(serifFont);
        t->setDefaultTextColor(Qt::white);

        t->setPos(0, 250 - scale / 2.0);
        t->setTextWidth(ui->graphicsView->sceneRect().width());
        t->setTransformOriginPoint(QPointF(500, scale));

        auto e = new QGraphicsDropShadowEffect;
        e->setBlurRadius(40);
        e->setColor(Qt::black);
        e->setOffset(10, 10);
        t->setGraphicsEffect(e);

        QTextBlockFormat format;
        format.setAlignment(Qt::AlignCenter);
        QTextCursor cursor = t->textCursor();
        cursor.select(QTextCursor::Document);
        cursor.mergeBlockFormat(format);
        cursor.clearSelection();
        t->setTextCursor(cursor);

        ui->graphicsView->scene()->addItem(t);
    }

    text[txt]->setOpacity(0);
    text[txt]->show();


    QParallelAnimationGroup *animation = new QParallelAnimationGroup;

    // SCALE
    QPropertyAnimation* text_animation_scale = new QPropertyAnimation(text[txt], "scale");
    QObject::connect(text_animation_scale, SIGNAL(finished()), text_animation_scale, SLOT(deleteLater()));

    text_animation_scale->setDuration(400);
    text_animation_scale->setStartValue(0.0);
    text_animation_scale->setEndValue(1.0);
    text_animation_scale->setEasingCurve(QEasingCurve::InOutQuad);

    animation->addAnimation(text_animation_scale);


    // OPACITY
    QPropertyAnimation* text_animation_opacity = new QPropertyAnimation(text[txt], "opacity");
    QObject::connect(text_animation_opacity, SIGNAL(finished()), text_animation_opacity, SLOT(deleteLater()));

    text_animation_opacity->setDuration(400);
    text_animation_opacity->setStartValue(0.0);
    text_animation_opacity->setEndValue(1.0);
    text_animation_opacity->setEasingCurve(QEasingCurve::InOutQuad);

    animation->addAnimation(text_animation_opacity);

    return animation;
}

QParallelAnimationGroup* PhotoboxWindow::hideTextAnimation(const std::string &txt)
{
    QParallelAnimationGroup *animation = new QParallelAnimationGroup;

    // SCALE
    QPropertyAnimation* text_animation_scale = new QPropertyAnimation(text[txt], "scale");
    QObject::connect(text_animation_scale, SIGNAL(finished()), text_animation_scale, SLOT(deleteLater()));

    text_animation_scale->setDuration(400);
    text_animation_scale->setStartValue(1.0);
    text_animation_scale->setEndValue(0.0);
    text_animation_scale->setEasingCurve(QEasingCurve::InOutQuad);

    animation->addAnimation(text_animation_scale);


    // OPACITY
    QPropertyAnimation* text_animation_opacity = new QPropertyAnimation(text[txt], "opacity");
    QObject::connect(text_animation_opacity, SIGNAL(finished()), text_animation_opacity, SLOT(deleteLater()));

    text_animation_opacity->setDuration(400);
    text_animation_opacity->setStartValue(1.0);
    text_animation_opacity->setEndValue(0.0);
    text_animation_opacity->setEasingCurve(QEasingCurve::InOutQuad);

    animation->addAnimation(text_animation_opacity);

    return animation;
}

void PhotoboxWindow::startPictureTakingAnimations()
{
    ui->graphicsView->viewport()->update();



    //    text->hide();

    QSequentialAnimationGroup *sequence = new QSequentialAnimationGroup;

    QParallelAnimationGroup *start = new QParallelAnimationGroup;

    // BLUR
    //    QPropertyAnimation* blur_animation = new QPropertyAnimation(shot_effect, "blurRadius");
    //    QObject::connect(blur_animation, SIGNAL(finished()), blur_animation, SLOT(deleteLater()));

    //    blur_animation->setDuration(1000);
    //    blur_animation->setStartValue(0.0);
    //    blur_animation->setEndValue(30.0);
    //    blur_animation->setEasingCurve(QEasingCurve::OutQuad);

    //    start->addAnimation(blur_animation);

    //    sequence->addAnimation(start);


    sequence->addAnimation(addTextAnimation("3", 360));
    sequence->addPause(1000);
    sequence->addAnimation(hideTextAnimation("3"));
    sequence->addPause(100);
    sequence->addAnimation(addTextAnimation("2", 360));
    sequence->addPause(1000);
    sequence->addAnimation(hideTextAnimation("2"));
    sequence->addPause(100);
    sequence->addAnimation(addTextAnimation("1", 360));
    sequence->addPause(1000);
    sequence->addAnimation(hideTextAnimation("1"));
    sequence->addPause(100);
    sequence->addAnimation(addTextAnimation("Bitte lächeln!", 120));

    sequence->start();

    QObject::connect(sequence, SIGNAL(finished()), this, SIGNAL(endPictureTakingAnimations()));


    //    QtConcurrent::run([this]() {
    //        emit takePicture();
    //    });
}

void PhotoboxWindow::showPreview(QImage *image)
{
    auto view = ui->graphicsView;
    if(preview == nullptr) {
        preview = new Pixmap(QPixmap::fromImage(image->mirrored(true, false)));
        view->scene()->addItem(preview);
        shot_effect->setBlurHints(QGraphicsBlurEffect::AnimationHint | QGraphicsBlurEffect::QualityHint);
        preview->setGraphicsEffect(shot_effect);

    } else {
        preview->setPixmap(QPixmap::fromImage(image->mirrored(true, false)));
    }

    shot_effect->setBlurRadius(0);

    view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
}


void PhotoboxWindow::showImage(QImage *image)
{
    std::cout << "show image of size " << image->width() << "x" << image->height() << std::endl;
    auto view = ui->graphicsView;
    if(last_image == nullptr) {
        last_image = new Pixmap(QPixmap::fromImage(*image));
        view->scene()->addItem(last_image);
    } else {
        last_image->setPixmap(QPixmap::fromImage(*image));
    }

    for(auto t : text) {
        t.second->hide();
    }

    double scale = preview->pixmap().width() / (double) last_image->pixmap().width(); //0.2;
    last_image->setScale(scale);
    last_image->setPos(0,0);

    QPropertyAnimation* animation = new QPropertyAnimation(last_image, "scale");
    animation->setDuration(1000);
    animation->setStartValue(last_image->scale());
    animation->setEndValue(0.0);

    animation->setEasingCurve(QEasingCurve::OutBounce);

    show_time = 8000;

    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    timer->setInterval(show_time);

    QObject::connect(timer, SIGNAL(timeout()), animation, SLOT(start()));
    QObject::connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    QObject::connect(animation, SIGNAL(finished()), animation, SLOT(deleteLater()));

    timer->start();

    view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);

    if(time_left_text == nullptr) {
        time_left_text = new QGraphicsTextItem("Time left");

        QFont serifFont("Arial", 64, QFont::Bold);
        time_left_text->setFont(serifFont);
        time_left_text->setDefaultTextColor(Qt::white);

        time_left_text->setPos(0, 0);

        ui->graphicsView->scene()->addItem(time_left_text);
    }

    time_left_timer.start(100);
    time_left_timer.setSingleShot(false);

    start_time = QDateTime::currentMSecsSinceEpoch();
    QObject::connect(&time_left_timer, SIGNAL(timeout()), this, SLOT(updateTime()));

    QObject::connect(animation, SIGNAL(finished()), this, SLOT(done()));
}

void PhotoboxWindow::updateTime()
{
    uint64_t now  = QDateTime::currentMSecsSinceEpoch();
    int64_t dt = now - start_time;

    double seconds = (show_time - dt) / 1000.0;

    if(seconds >= 0) {
        time_left_text->setPlainText(QString::number((int) std::round(seconds)));
    } else {
        time_left_text->hide();
    }
}

void PhotoboxWindow::done()
{
    time_left_timer.stop();
    time_left_text->hide();
}
