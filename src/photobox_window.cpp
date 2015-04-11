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
      last_image(nullptr), preview(nullptr), text(nullptr)
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
        doTakePicture();
    }
}

void PhotoboxWindow::doTakePicture()
{
    ui->graphicsView->viewport()->update();

    if(text == nullptr) {
        text = new QGraphicsTextItem("Cheese");

        QFont serifFont("Arial", 80, QFont::Bold);
        text->setFont(serifFont);
        text->setDefaultTextColor(Qt::white);

        text->setPos(0, 250);
        text->setTextWidth(ui->graphicsView->sceneRect().width());
        text->setTransformOriginPoint(QPointF(500, 75));

        auto e = new QGraphicsDropShadowEffect;
        e->setBlurRadius(40);
        e->setColor(Qt::black);
        e->setOffset(10, 10);
        text->setGraphicsEffect(e);

        QTextBlockFormat format;
        format.setAlignment(Qt::AlignCenter);
        QTextCursor cursor = text->textCursor();
        cursor.select(QTextCursor::Document);
        cursor.mergeBlockFormat(format);
        cursor.clearSelection();
        text->setTextCursor(cursor);

        ui->graphicsView->scene()->addItem(text);
    }


    // BLUR
    QPropertyAnimation* blur_animation = new QPropertyAnimation(shot_effect, "blurRadius");
    blur_animation->setDuration(3000);
    blur_animation->setStartValue(0.0);
    blur_animation->setEndValue(30.0);

    blur_animation->setEasingCurve(QEasingCurve::OutQuad);

    blur_animation->start();

    QObject::connect(blur_animation, SIGNAL(finished()), blur_animation, SLOT(deleteLater()));

    // SCALE
    QPropertyAnimation* text_animation_scale = new QPropertyAnimation(text, "scale");
    text_animation_scale->setDuration(400);
    text_animation_scale->setStartValue(0.0);
    text_animation_scale->setEndValue(1.0);
    text_animation_scale->setEasingCurve(QEasingCurve::InOutQuad);

    text_animation_scale->start();

    QObject::connect(text_animation_scale, SIGNAL(finished()), text_animation_scale, SLOT(deleteLater()));

    // OPACITY
    QPropertyAnimation* text_animation_opacity = new QPropertyAnimation(text, "opacity");
    text_animation_opacity->setDuration(400);
    text_animation_opacity->setStartValue(0.0);
    text_animation_opacity->setEndValue(1.0);
    text_animation_opacity->setEasingCurve(QEasingCurve::InOutQuad);

    text_animation_opacity->start();


    text->show();

    QObject::connect(text_animation_opacity, SIGNAL(finished()), text_animation_opacity, SLOT(deleteLater()));

    QtConcurrent::run([this]() {
        emit takePicture();
    });
}

void PhotoboxWindow::showPreview(QImage *image)
{
    auto view = ui->graphicsView;
    if(preview == nullptr) {
        preview = new Pixmap(QPixmap::fromImage(*image));
        view->scene()->addItem(preview);
        shot_effect->setBlurHints(QGraphicsBlurEffect::AnimationHint | QGraphicsBlurEffect::QualityHint);
        preview->setGraphicsEffect(shot_effect);

    } else {
        preview->setPixmap(QPixmap::fromImage(*image));
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

    if(text) {
        text->hide();
    }

    double scale = preview->pixmap().width() / (double) last_image->pixmap().width(); //0.2;
    last_image->setScale(scale);
    last_image->setPos(0,0);

    QPropertyAnimation* animation = new QPropertyAnimation(last_image, "scale");
    animation->setDuration(1000);
    animation->setStartValue(last_image->scale());
    animation->setEndValue(0.0);

    animation->setEasingCurve(QEasingCurve::OutBounce);

    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    timer->setInterval(3000);

    QObject::connect(timer, SIGNAL(timeout()), animation, SLOT(start()));
    QObject::connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    QObject::connect(animation, SIGNAL(finished()), animation, SLOT(deleteLater()));

    timer->start();

    view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
}
