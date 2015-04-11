#include "photobox_window.h"
#include <iostream>

PhotoboxWindow::PhotoboxWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::Photobox)
{
    ui->setupUi(this);

    ui->graphicsView->setScene(new QGraphicsScene);

    QObject::connect(ui->Cheese, SIGNAL(clicked()), this, SIGNAL(takePicture()));
}

PhotoboxWindow::~PhotoboxWindow()
{
    delete ui;
}

void PhotoboxWindow::showImage(QImage *image)
{
    auto view = ui->graphicsView;
    view->scene()->clear();
    view->scene()->addPixmap(QPixmap::fromImage(*image));
    view->fitInView(view->scene()->sceneRect(), Qt::KeepAspectRatio);
}
