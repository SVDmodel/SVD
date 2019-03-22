#ifndef CUSTOM3DINPUTHANDLER_H
#define CUSTOM3DINPUTHANDLER_H


#include <QtDataVisualization/Q3DInputHandler>

  using namespace QtDataVisualization;

  class Custom3dInputHandler : public Q3DInputHandler
  {
      Q_OBJECT
  public:
      explicit Custom3dInputHandler(QObject *parent = nullptr);

      virtual void mouseMoveEvent(QMouseEvent *event, const QPoint &mousePos);
      virtual void mousePressEvent(QMouseEvent *event, const QPoint &mousePos);
      virtual void wheelEvent(QWheelEvent *event);
  };

#endif // CUSTOM3DINPUTHANDLER_H
