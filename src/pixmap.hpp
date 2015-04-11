#ifndef PIXMAP_HPP
#define PIXMAP_HPP

#include <QGraphicsPixmapItem>

class Pixmap : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)

public:
    explicit Pixmap(QGraphicsItem *parent = 0)
        : QGraphicsPixmapItem(parent)
    {

    }

    explicit Pixmap(const QPixmap &pixmap, QGraphicsItem *parent = 0)
        : QGraphicsPixmapItem(pixmap, parent)
    {

    }
};

#endif // PIXMAP_HPP

