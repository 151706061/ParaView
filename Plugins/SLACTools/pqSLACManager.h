// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#ifndef pqSLACManager_h
#define pqSLACManager_h

#include <QObject>

class QAction;

class pqPipelineSource;
class pqRenderView;
class pqServer;
class pqView;

// #define AUTO_FIND_TEMPORAL_RANGE

/// This singleton class manages the state associated with the packaged
/// visualizations provided by the SLAC tools.
class pqSLACManager : public QObject
{
  Q_OBJECT;

public:
  static pqSLACManager* instance();

  ~pqSLACManager() override;

  /// Get the action for the respective operation.
  QAction* actionDataLoadManager();
  QAction* actionShowEField();
  QAction* actionShowBField();
  QAction* actionShowParticles();
  QAction* actionSolidMesh();
  QAction* actionWireframeSolidMesh();
  QAction* actionWireframeAndBackMesh();
  QAction* actionPlotOverZ();
  QAction* actionToggleBackgroundBW();
  QAction* actionShowStandardViewpoint();
  QAction* actionTemporalResetRange();
  QAction* actionCurrentTimeResetRange();

  /// Convenience function for getting the current server.
  pqServer* getActiveServer();

  /// Get the window used for viewing the mesh.
  pqView* getMeshView();
  pqRenderView* getMeshRenderView();

  /// Get the window used for viewing plots.
  pqView* getPlotView();

  /// Get the reader objects.  Returns nullptr if that reader was never
  /// created.
  pqPipelineSource* getMeshReader();
  pqPipelineSource* getParticlesReader();

  /// Get plotting object.  Returns nullptr if that object was never created.
  pqPipelineSource* getPlotFilter();

  /// Get object that computes ranges over time.  Returns nullptr if that
  /// object was never created.
  pqPipelineSource* getTemporalRanges();

  /// Data loader calls this to notify manager
  void loaderCreatingPipeline();

  /// Convenience function for destroying a pipeline object and all of its
  /// consumers.
  static void destroyPipelineSourceAndConsumers(pqPipelineSource* source);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void showDataLoadManager();
  void checkActionEnabled();
  void onSourceCreated(pqPipelineSource* source);
  void showField(QString name);
  void showField(const char* name);
  void showEField();
  void showBField();
  void showParticles(bool show);
  void showSolidMesh();
  void showWireframeSolidMesh();
  void showWireframeAndBackMesh();
  void createPlotOverZ();
  void toggleBackgroundBW();
  void showStandardViewpoint();
  void resetRangeTemporal();
  void resetRangeCurrentTime();

protected:
  /// Finds a pipeline source with the given SM XML name.  If there is more than
  /// one, the first is returned.
  virtual pqPipelineSource* findPipelineSource(const char* SMName);

  /// Finds a view appropriate for the data of the source and port given,
  /// constrained to those views with the given type.
  virtual pqView* findView(pqPipelineSource* source, int port, const QString& viewType);

  /// Updates the plot view (if it is created) with the field shown in the
  /// mesh view.
  virtual void updatePlotField();

  /// Information about currently shown field.
  QString CurrentFieldName;
  bool CurrentFieldRangeKnown;
  double CurrentFieldRange[2];
  double CurrentFieldAverage;

  /// If true, scale the fields by the values in the current timestep.  If
  /// false and the entire field range is known, scale by the range over all
  /// timesteps.  If the range over all timesteps is not known, fall back to
  /// using only the range known.
  bool ScaleFieldsByCurrentTimeStep;

private:
  pqSLACManager(QObject* p);

  class pqInternal;
  pqInternal* Internal;

  Q_DISABLE_COPY(pqSLACManager)
};

#endif // pqSLACManager_h
