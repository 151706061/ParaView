// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCoordinateFramePropertyWidget.h"
#include "ui_pqCoordinateFramePropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqPointPickingHelper.h"
#include "pqRenderView.h"
#include "pqWidgetUtilities.h"

#include "vtkCamera.h"
#include "vtkSMDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVector.h"

#include <QComboBox>
#include <QMenu>

namespace
{
// Divide the vector by its length (if non-zero), returning true if non-zero.
template <typename T, std::size_t N>
bool normalize(std::array<T, N>& vec)
{
  T mag = 0.;
  for (std::size_t ii = 0; ii < N; ++ii)
  {
    mag += vec[ii] * vec[ii];
  }
  mag = std::sqrt(mag);
  if (mag < 1e-8)
  {
    return false;
  }
  for (std::size_t ii = 0; ii < N; ++ii)
  {
    vec[ii] /= mag;
  }
  return true;
}

// Avoid any dimension having an extent of 0;
// ensure that each dimension has some thickness and then scale.
static void pqAdjustBounds(vtkBoundingBox& bbox, double scaleFactor)
{
  double max_length = bbox.GetMaxLength();
  max_length = max_length > 0 ? max_length * 0.05 : 1;
  double min_point[3], max_point[3];
  bbox.GetMinPoint(min_point[0], min_point[1], min_point[2]);
  bbox.GetMaxPoint(max_point[0], max_point[1], max_point[2]);
  for (int cc = 0; cc < 3; cc++)
  {
    if (bbox.GetLength(cc) == 0)
    {
      min_point[cc] -= max_length;
      max_point[cc] += max_length;
    }

    double mid = (min_point[cc] + max_point[cc]) / 2.0;
    min_point[cc] = mid + scaleFactor * (min_point[cc] - mid);
    max_point[cc] = mid + scaleFactor * (max_point[cc] - mid);
  }
  bbox.SetMinPoint(min_point);
  bbox.SetMaxPoint(max_point);
}
}

pqCoordinateFramePropertyWidget::pqCoordinateFramePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
      "representations", "CoordinateFrameWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::CoordinateFramePropertyWidget ui;
  ui.setupUi(this);
  pqWidgetUtilities::formatChildTooltips(this);

  if (this->widgetProxy()->GetProperty("LockedAxis"))
  {
    vtkSMProperty* lockAxisInfo = this->widgetProxy()->GetProperty("LockedAxisInfo");
    this->addPropertyLink(this, "lockedAxis", SIGNAL(lockedAxisChangedByUser(int)), lockAxisInfo);
    QObject::connect(ui.lockAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
      &pqCoordinateFramePropertyWidget::currentIndexChangedLockAxis);
  }
  else
  {
    qCritical("Missing required property for 'LockedAxis' to function.");
    ui.lockAxis->hide();
  }

  if (vtkSMProperty* origin = smgroup->GetProperty("Origin"))
  {
    this->addPropertyLink(ui.originX, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 0);
    this->addPropertyLink(ui.originY, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 1);
    this->addPropertyLink(ui.originZ, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 2);
    auto* originInfo = dynamic_cast<vtkSMDoubleVectorProperty*>(smgroup->GetProperty("OriginInfo"));
    if (originInfo)
    {
      this->addPropertyLink(
        ui.originX, "text2", SIGNAL(textChangedAndEditingFinished()), originInfo, 0);
      this->addPropertyLink(
        ui.originY, "text2", SIGNAL(textChangedAndEditingFinished()), originInfo, 1);
      this->addPropertyLink(
        ui.originZ, "text2", SIGNAL(textChangedAndEditingFinished()), originInfo, 2);
    }
    ui.labelOrigin->setText(QCoreApplication::translate("ServerManagerXML", origin->GetXMLLabel()));
    QString tooltip = this->getTooltip(origin);
    ui.originX->setToolTip(tooltip);
    ui.originY->setToolTip(tooltip);
    ui.originZ->setToolTip(tooltip);
    ui.labelOrigin->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Origin'.");
  }

  if (vtkSMProperty* xAxis = smgroup->GetProperty("XAxis"))
  {
    this->addPropertyLink(ui.xAxisX, "text2", SIGNAL(blank()), xAxis, 0);
    this->addPropertyLink(ui.xAxisY, "text2", SIGNAL(blank()), xAxis, 1);
    this->addPropertyLink(ui.xAxisZ, "text2", SIGNAL(blank()), xAxis, 2);
    QObject::connect(
      ui.acceptXAxis, &QToolButton::clicked, this, &pqCoordinateFramePropertyWidget::setUserXAxis);
    auto* xAxisInfo = dynamic_cast<vtkSMDoubleVectorProperty*>(smgroup->GetProperty("XAxisInfo"));
    if (xAxisInfo)
    {
      this->addPropertyLink(ui.xAxisX, "text2", SIGNAL(blank()), xAxisInfo, 0);
      this->addPropertyLink(ui.xAxisY, "text2", SIGNAL(blank()), xAxisInfo, 1);
      this->addPropertyLink(ui.xAxisZ, "text2", SIGNAL(blank()), xAxisInfo, 2);
    }
    ui.labelXAxis->setText(QCoreApplication::translate("ServerManagerXML", xAxis->GetXMLLabel()));
    QString tooltip = this->getTooltip(xAxis);
    ui.xAxisX->setToolTip(tooltip);
    ui.xAxisY->setToolTip(tooltip);
    ui.xAxisZ->setToolTip(tooltip);
    ui.labelXAxis->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'XAxis'.");
  }

  if (vtkSMProperty* yAxis = smgroup->GetProperty("YAxis"))
  {
    this->addPropertyLink(ui.yAxisX, "text2", SIGNAL(blank()), yAxis, 0);
    this->addPropertyLink(ui.yAxisY, "text2", SIGNAL(blank()), yAxis, 1);
    this->addPropertyLink(ui.yAxisZ, "text2", SIGNAL(blank()), yAxis, 2);
    QObject::connect(
      ui.acceptYAxis, &QToolButton::clicked, this, &pqCoordinateFramePropertyWidget::setUserYAxis);
    auto* yAxisInfo = dynamic_cast<vtkSMDoubleVectorProperty*>(smgroup->GetProperty("XAxisInfo"));
    if (yAxisInfo)
    {
      this->addPropertyLink(ui.yAxisX, "text2", SIGNAL(blank()), yAxisInfo, 0);
      this->addPropertyLink(ui.yAxisY, "text2", SIGNAL(blank()), yAxisInfo, 1);
      this->addPropertyLink(ui.yAxisZ, "text2", SIGNAL(blank()), yAxisInfo, 2);
    }
    ui.labelYAxis->setText(QCoreApplication::translate("ServerManagerXML", yAxis->GetXMLLabel()));
    QString tooltip = this->getTooltip(yAxis);
    ui.yAxisX->setToolTip(tooltip);
    ui.yAxisY->setToolTip(tooltip);
    ui.yAxisZ->setToolTip(tooltip);
    ui.labelYAxis->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'YAxis'.");
  }

  if (vtkSMProperty* zAxis = smgroup->GetProperty("ZAxis"))
  {
    this->addPropertyLink(ui.zAxisX, "text2", SIGNAL(blank()), zAxis, 0);
    this->addPropertyLink(ui.zAxisY, "text2", SIGNAL(blank()), zAxis, 1);
    this->addPropertyLink(ui.zAxisZ, "text2", SIGNAL(blank()), zAxis, 2);
    QObject::connect(
      ui.acceptZAxis, &QToolButton::clicked, this, &pqCoordinateFramePropertyWidget::setUserZAxis);
    auto* zAxisInfo = dynamic_cast<vtkSMDoubleVectorProperty*>(smgroup->GetProperty("XAxisInfo"));
    if (zAxisInfo)
    {
      this->addPropertyLink(ui.zAxisX, "text2", SIGNAL(blank()), zAxisInfo, 0);
      this->addPropertyLink(ui.zAxisY, "text2", SIGNAL(blank()), zAxisInfo, 1);
      this->addPropertyLink(ui.zAxisZ, "text2", SIGNAL(blank()), zAxisInfo, 2);
    }
    ui.labelZAxis->setText(QCoreApplication::translate("ServerManagerXML", zAxis->GetXMLLabel()));
    QString tooltip = this->getTooltip(zAxis);
    ui.zAxisX->setToolTip(tooltip);
    ui.zAxisY->setToolTip(tooltip);
    ui.zAxisZ->setToolTip(tooltip);
    ui.labelZAxis->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'XAxis'.");
  }

  // link a few buttons
  this->connect(ui.actionAlignToWorldX, SIGNAL(triggered()), SLOT(useXNormal()));
  this->connect(ui.actionAlignToWorldY, SIGNAL(triggered()), SLOT(useYNormal()));
  this->connect(ui.actionAlignToWorldZ, SIGNAL(triggered()), SLOT(useZNormal()));
  this->connect(ui.actionAxisAlignWorld, SIGNAL(triggered()), SLOT(resetToWorldXYZ()));
  this->connect(ui.actionAlignToCameraOut, SIGNAL(triggered()), SLOT(useCameraNormal()));
  this->connect(ui.actionAlignCameraToAxis, SIGNAL(triggered()), SLOT(resetCameraToNormal()));
  ui.actionRecenterOnBounds->setEnabled(false);
  this->connect(ui.actionRecenterOnBounds, SIGNAL(triggered()), SLOT(resetToDataBounds()));

  auto actionMenu = new QMenu(tr("Coordinate frame utilities"));
  actionMenu->addAction(ui.actionAlignToWorldX);
  actionMenu->addAction(ui.actionAlignToWorldY);
  actionMenu->addAction(ui.actionAlignToWorldZ);
  actionMenu->addAction(ui.actionAxisAlignWorld);
  actionMenu->addAction(ui.actionAlignToCameraOut);
  actionMenu->addAction(ui.actionAlignCameraToAxis);
  // actionMenu->addAction(ui.actionRecenterOnBounds);
  ui.actionButton->setMenu(actionMenu);

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  using PickOption = pqPointPickingHelper::PickOption;

  // picking origin point actions
  pqPointPickingHelper* pickPointHelper =
    new pqPointPickingHelper(QKeySequence(tr("P")), false, this, PickOption::Coordinates);
  pickPointHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickPointHelper, SIGNAL(pick(double, double, double)), SLOT(setOrigin(double, double, double)));

  pqPointPickingHelper* pickPointHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this, PickOption::Coordinates);
  pickPointHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickPointHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setOrigin(double, double, double)));

  // picking normal actions
  pqPointPickingHelper* pickNormalHelper =
    new pqPointPickingHelper(QKeySequence(tr("N")), false, this, PickOption::Normal);
  pickNormalHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickNormalHelper, SIGNAL(pick(double, double, double)),
    SLOT(setNormal(double, double, double)));

  pqPointPickingHelper* pickNormalHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+N")), true, this, PickOption::Normal);
  pickNormalHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickNormalHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setNormal(double, double, double)));

  // picking direction actions
  pqPointPickingHelper* pickDirectionHelper =
    new pqPointPickingHelper(QKeySequence(tr("T")), false, this, PickOption::Coordinates);
  pickDirectionHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickDirectionHelper, SIGNAL(pick(double, double, double)),
    SLOT(setDirection(double, double, double)));

  pqPointPickingHelper* pickDirectionHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+T")), true, this, PickOption::Coordinates);
  pickDirectionHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickDirectionHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setDirection(double, double, double)));

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::dataUpdated, this,
    &pqCoordinateFramePropertyWidget::placeWidget);
}

pqCoordinateFramePropertyWidget::~pqCoordinateFramePropertyWidget() = default;

void pqCoordinateFramePropertyWidget::placeWidget()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    return;
  }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
  pqAdjustBounds(bbox, scaleFactor);
  double bds[6];
  bbox.GetBounds(bds);
  vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bds, 6);
  wdgProxy->UpdateVTKObjects();
}

int pqCoordinateFramePropertyWidget::getLockedAxis() const
{
  auto* combo = this->findChild<QComboBox*>("lockAxis");
  int lockAxis = combo->currentIndex() - 1;
  return lockAxis;
}

void pqCoordinateFramePropertyWidget::setLockedAxis(int lockedAxis)
{
  auto* combo = this->findChild<QComboBox*>("lockAxis");
  int prior = combo->currentIndex() - 1;
  if (lockedAxis != prior)
  {
    combo->setCurrentIndex(lockedAxis + 1);
  }
}

void pqCoordinateFramePropertyWidget::resetToWorldXYZ()
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  wdgProxy->InvokeCommand("ResetAxes");
  wdgProxy->UpdatePropertyInformation();
  Q_EMIT this->changeAvailable();
  this->render();
}

void pqCoordinateFramePropertyWidget::resetToDataBounds()
{
  vtkBoundingBox bbox = this->dataBounds();

  if (bbox.IsValid())
  {
    vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
    double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
    pqAdjustBounds(bbox, scaleFactor);
    double origin[3];
    bbox.GetCenter(origin);
    vtkSMPropertyHelper(wdgProxy, "Origin").Set(origin, 3);
    wdgProxy->UpdateVTKObjects();
    Q_EMIT this->changeAvailable();
    this->render();
  }
}

void pqCoordinateFramePropertyWidget::resetCameraToNormal()
{
  if (pqRenderView* renView = qobject_cast<pqRenderView*>(this->view()))
  {
    vtkCamera* camera = renView->getRenderViewProxy()->GetActiveCamera();
    vtkSMProxy* wdgProxy = this->widgetProxy();
    vtkVector3d ax[3];
    vtkVector3d up;
    vtkVector3d dr;
    camera->GetDirectionOfProjection(dr.GetData());
    camera->GetViewUp(up.GetData());
    vtkSMPropertyHelper(wdgProxy, "XAxis").Get(ax[0].GetData(), 3);
    vtkSMPropertyHelper(wdgProxy, "YAxis").Get(ax[1].GetData(), 3);
    vtkSMPropertyHelper(wdgProxy, "ZAxis").Get(ax[2].GetData(), 3);
    // Find axis closest to current camera direction.
    int bestAxis = -1;
    bool flip = false;
    double bestProjection = -1.0;
    for (int ii = 0; ii < 3; ++ii)
    {
      double projection = ax[ii].Dot(dr);
      if (std::abs(projection) > bestProjection)
      {
        bestAxis = ii;
        bestProjection = std::abs(projection);
        // Flip the axis by default so we look toward it rather than along it
        // (assuming the base point is in the view frustum).
        flip = (std::abs(projection) > projection);
      }
    }
    dr = flip ? -ax[bestAxis] : ax[bestAxis];
    // The up vector should be aligned with the next coordinate axis in the cycle:
    int bestUp = (bestAxis + 1) % 3;
    // Flip the coordinate axis to best align with the previous up vector,
    // then assign it to the camera up-vector:
    up = (ax[bestUp].Dot(up) < 0.0) ? -ax[bestUp] : ax[bestUp];
    renView->resetViewDirection(dr[0], dr[1], dr[2], up[0], up[1], up[2]);
    renView->render();
  }
}

void pqCoordinateFramePropertyWidget::useCameraNormal()
{
  vtkSMRenderViewProxy* viewProxy =
    this->view() ? vtkSMRenderViewProxy::SafeDownCast(this->view()->getProxy()) : nullptr;
  if (viewProxy)
  {
    vtkCamera* camera = viewProxy->GetActiveCamera();

    double camera_normal[3];
    camera->GetViewPlaneNormal(camera_normal);
    camera_normal[0] = -camera_normal[0];
    camera_normal[1] = -camera_normal[1];
    camera_normal[2] = -camera_normal[2];
    this->setNormal(camera_normal[0], camera_normal[1], camera_normal[2]);
  }
}

void pqCoordinateFramePropertyWidget::setUserXAxis()
{
  this->setUserAxis(0);
}

void pqCoordinateFramePropertyWidget::setUserYAxis()
{
  this->setUserAxis(1);
}

void pqCoordinateFramePropertyWidget::setUserZAxis()
{
  this->setUserAxis(2);
}

void pqCoordinateFramePropertyWidget::setUserAxis(int axisIndex)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  std::array<double, 3> axis;
  switch (axisIndex)
  {
    default: // fall through
    case 0:
      axis[0] = this->findChild<pqDoubleLineEdit*>("xAxisX")->text().toDouble();
      axis[1] = this->findChild<pqDoubleLineEdit*>("xAxisY")->text().toDouble();
      axis[2] = this->findChild<pqDoubleLineEdit*>("xAxisZ")->text().toDouble();
      normalize(axis);
      vtkSMPropertyHelper(wdgProxy, "XAxis").Set(axis.data(), 3);
      break;
    case 1:
      axis[0] = this->findChild<pqDoubleLineEdit*>("yAxisX")->text().toDouble();
      axis[1] = this->findChild<pqDoubleLineEdit*>("yAxisY")->text().toDouble();
      axis[2] = this->findChild<pqDoubleLineEdit*>("yAxisZ")->text().toDouble();
      normalize(axis);
      vtkSMPropertyHelper(wdgProxy, "YAxis").Set(axis.data(), 3);
      break;
    case 2:
      axis[0] = this->findChild<pqDoubleLineEdit*>("zAxisX")->text().toDouble();
      axis[1] = this->findChild<pqDoubleLineEdit*>("zAxisY")->text().toDouble();
      axis[2] = this->findChild<pqDoubleLineEdit*>("zAxisZ")->text().toDouble();
      normalize(axis);
      vtkSMPropertyHelper(wdgProxy, "ZAxis").Set(axis.data(), 3);
      break;
  }
  wdgProxy->UpdateVTKObjects();
  wdgProxy->UpdatePropertyInformation();
  Q_EMIT this->changeAvailable();
  this->render();
}

void pqCoordinateFramePropertyWidget::setNormal(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double n[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "Normal").Set(n, 3);
  wdgProxy->UpdateVTKObjects();
  wdgProxy->UpdatePropertyInformation();
  Q_EMIT this->changeAvailable();
  this->render();
}

void pqCoordinateFramePropertyWidget::setDirection(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double n[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "Direction").Set(n, 3);
  wdgProxy->UpdateVTKObjects();
  wdgProxy->UpdatePropertyInformation();
  Q_EMIT this->changeAvailable();
  this->render();
}

void pqCoordinateFramePropertyWidget::setOrigin(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double o[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "Origin").Set(o, 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

void pqCoordinateFramePropertyWidget::currentIndexChangedLockAxis(int lockAxis)
{
  --lockAxis; // Integer property values start at -1 (Axis::NONE), not 0.
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  if (auto* lockProp = dynamic_cast<vtkSMIntVectorProperty*>(wdgProxy->GetProperty("LockedAxis")))
  {
    lockProp->SetImmediateUpdate(true);
    lockProp->SetElements1(lockAxis);
    this->lockedAxisChangedByUser(lockAxis);
  }
}
