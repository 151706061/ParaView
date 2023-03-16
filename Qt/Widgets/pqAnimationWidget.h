/*=========================================================================

   Program: ParaView
   Module:    pqAnimationWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef pqAnimationWidget_h
#define pqAnimationWidget_h

#include "pqWidgetsModule.h"

#include "vtkParaViewDeprecation.h"

#include <QAbstractScrollArea>
#include <QStandardItemModel>

class QGraphicsView;
class QHeaderView;
class pqAnimationModel;
class pqAnimationTrack;

class PARAVIEW_DEPRECATED_IN_5_12_0(
  "See `pqTimeManagerWidget` for new design.") PQWIDGETS_EXPORT pqAnimationWidget
  : public QAbstractScrollArea
{
  Q_OBJECT;
  using Superclass = QAbstractScrollArea;

public:
  pqAnimationWidget(QWidget* p = nullptr);
  ~pqAnimationWidget() override = default;

  pqAnimationModel* animationModel() const;

  /**
   * Enabled header is used to show if the track is enabled.
   */
  QHeaderView* enabledHeader() const;

  QHeaderView* createDeleteHeader() const;
  QWidget* createDeleteWidget() const;

Q_SIGNALS:
  // emitted when a track is double clicked on
  void trackSelected(pqAnimationTrack*);
  void deleteTrackClicked(pqAnimationTrack*);
  void createTrackClicked();
  // emitted when the timeline offset is changed
  void timelineOffsetChanged(int);

  /**
   * request enable/disabling of the track.
   */
  void enableTrackClicked(pqAnimationTrack*);

protected Q_SLOTS:
  void updateSizes();
  void headerDblClicked(int);
  void headerDeleteClicked(int);
  void headerEnabledClicked(int which);

protected: // NOLINT(readability-redundant-access-specifiers)
  void updateGeometries();
  void updateScrollBars();
  void updateWidgetPosition();
  void scrollContentsBy(int dx, int dy) override;
  bool event(QEvent* e) override;
  void resizeEvent(QResizeEvent* e) override;
  void showEvent(QShowEvent* e) override;
  void wheelEvent(QWheelEvent* e) override;

private:
  QGraphicsView* View;
  QHeaderView* CreateDeleteHeader;
  QHeaderView* EnabledHeader;
  QStandardItemModel CreateDeleteModel;
  QHeaderView* Header;
  QWidget* CreateDeleteWidget;
  pqAnimationModel* Model;
};

#endif // pqAnimationWidget_h
