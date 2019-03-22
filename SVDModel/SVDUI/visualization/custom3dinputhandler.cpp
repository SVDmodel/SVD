#include "custom3dinputhandler.h"


#include <QtDataVisualization/Q3DCamera>

Custom3dInputHandler::Custom3dInputHandler(QObject *parent) :
  Q3DInputHandler(parent)
{
}

void Custom3dInputHandler::mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos)
{
  Q_UNUSED(event)
    //setInputPosition(mousePos);
    Q3DInputHandler::mouseMoveEvent(event, mousePos);
}

void Custom3dInputHandler::mousePressEvent(QMouseEvent *event, const QPoint &mousePos)
{
    if (event->button() != Qt::LeftButton) {
        Q3DInputHandler::mousePressEvent(event, mousePos);
        return;
    }


    //setInputPosition(mousePos);
    scene()->setGraphPositionQuery(mousePos);
    Q3DInputHandler::mousePressEvent(event, mousePos);
}

void Custom3dInputHandler::wheelEvent(QWheelEvent *event)
{
  Q3DInputHandler::wheelEvent(event);
  return;

    // Adjust zoom level based on what zoom range we're in.
  int zoomLevel = scene()->activeCamera()->zoomLevel();
  if (zoomLevel > 100)
      zoomLevel += event->angleDelta().y() / 12;
  else if (zoomLevel > 50)
      zoomLevel += event->angleDelta().y() / 60;
  else
      zoomLevel += event->angleDelta().y() / 120;
  if (zoomLevel > 500)
      zoomLevel = 500;
  else if (zoomLevel < 10)
      zoomLevel = 10;

  scene()->activeCamera()->setZoomLevel(zoomLevel);
}
