#include "photobox_window.h"
#include <iostream>
#include <QGraphicsPixmapItem>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>

PhotoboxWindow::PhotoboxWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Photobox),
      last_image(nullptr), preview(nullptr)
{
    ui->setupUi(this);

    ui->graphicsView->setScene(new QGraphicsScene);

    showFullScreen();


    auto view = ui->graphicsView;

    QObject::connect(ui->Cheese, SIGNAL(clicked()), this, SIGNAL(takePicture()));
}

PhotoboxWindow::~PhotoboxWindow()
{
    delete ui;
}

void PhotoboxWindow::showPreview(QImage *image)
{
    auto view = ui->graphicsView;
    if(preview == nullptr) {
        preview = new Pixmap(QPixmap::fromImage(*image));
        view->scene()->addItem(preview);
    } else {
        preview->setPixmap(QPixmap::fromImage(*image));
    }
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
