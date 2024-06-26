// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPipelineBrowserWidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqExtractor.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqOutputPort.h"
#include "pqPipelineAnnotationFilterModel.h"
#include "pqPipelineModel.h"
#include "pqPipelineModelSelectionAdaptor.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkPVGeneralSettings.h"
#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMScalarBarWidgetRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewProxy.h"

#include <QDebug>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>

#include <cassert>

//-----------------------------------------------------------------------------
pqPipelineBrowserWidget::pqPipelineBrowserWidget(QWidget* parentObject)
  : Superclass(parentObject)
  , PipelineModel(
      new pqPipelineModel(*pqApplicationCore::instance()->getServerManagerModel(), this))
  , FilteredPipelineModel(new pqPipelineAnnotationFilterModel(this))
  , ContextMenu(new QMenu(this))
{
  this->configureModel();

  // Initialize pqFlatTreeView.
  this->Superclass::setModel(this->FilteredPipelineModel);
  this->getHeader()->hide();
  this->getHeader()->moveSection(1, 0);
  this->installEventFilter(this);
  this->setSelectionMode(pqFlatTreeView::ExtendedSelection);
  this->setContextMenuPolicy(Qt::DefaultContextMenu);

  // Connect internal handlers
  QObject::connect(
    this, SIGNAL(clicked(const QModelIndex&)), this, SLOT(handleIndexClicked(const QModelIndex&)));
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setActiveView(pqView*)));

  new pqPipelineModelSelectionAdaptor(this->getSelectionModel());
}

//-----------------------------------------------------------------------------
pqPipelineBrowserWidget::~pqPipelineBrowserWidget() = default;

//-----------------------------------------------------------------------------
void pqPipelineBrowserWidget::configureModel()
{
  this->FilteredPipelineModel->setSourceModel(this->PipelineModel);

  // Connect the model to the ServerManager model.
  pqServerManagerModel* smModel = pqApplicationCore::instance()->getServerManagerModel();

  // We connect to `preServerAdded` instead of `serverAdded` signal.
  // This makes it possible for the pqPipelineModel to become aware of a new
  // server connection before the
  // vtkSMParaViewPipelineController::InitializeSession is called by
  // pqServerManagerModel. Thus if any proxies are created during that call, the
  // pqPipelineModel knows which session they belong to.
  QObject::connect(
    smModel, SIGNAL(preServerAdded(pqServer*)), this->PipelineModel, SLOT(addServer(pqServer*)));
  QObject::connect(
    smModel, SIGNAL(serverRemoved(pqServer*)), this->PipelineModel, SLOT(removeServer(pqServer*)));
  QObject::connect(smModel, SIGNAL(sourceAdded(pqPipelineSource*)), this->PipelineModel,
    SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smModel, SIGNAL(sourceRemoved(pqPipelineSource*)), this->PipelineModel,
    SLOT(removeSource(pqPipelineSource*)));
  QObject::connect(smModel, SIGNAL(connectionAdded(pqPipelineSource*, pqPipelineSource*, int)),
    this->PipelineModel, SLOT(addConnection(pqPipelineSource*, pqPipelineSource*, int)));
  QObject::connect(smModel, SIGNAL(connectionRemoved(pqPipelineSource*, pqPipelineSource*, int)),
    this->PipelineModel, SLOT(removeConnection(pqPipelineSource*, pqPipelineSource*, int)));

  // monitor extractor related signals.
  QObject::connect(smModel, SIGNAL(extractorAdded(pqExtractor*)), this->PipelineModel,
    SLOT(addExtractor(pqExtractor*)));
  QObject::connect(smModel, SIGNAL(extractorRemoved(pqExtractor*)), this->PipelineModel,
    SLOT(removeExtractor(pqExtractor*)));
  QObject::connect(smModel, SIGNAL(connectionAdded(pqServerManagerModelItem*, pqExtractor*)),
    this->PipelineModel, SLOT(addConnection(pqServerManagerModelItem*, pqExtractor*)));
  QObject::connect(smModel, SIGNAL(connectionRemoved(pqServerManagerModelItem*, pqExtractor*)),
    this->PipelineModel, SLOT(removeConnection(pqServerManagerModelItem*, pqExtractor*)));

  // Use the tree view's font as the base for the model's modified
  // font.
  QFont modifiedFont = this->font();
  modifiedFont.setBold(true);
  this->PipelineModel->setModifiedFont(modifiedFont);

  // Make sure the tree items get expanded when new descendents
  // are added.
  QObject::connect(this->PipelineModel, SIGNAL(firstChildAdded(const QModelIndex&)), this,
    SLOT(expandWithModelIndexTranslation(const QModelIndex&)));

  QObject::connect(this->PipelineModel, SIGNAL(childWithChildrenAdded(const QModelIndex&)), this,
    SLOT(expandWithModelIndexTranslation(const QModelIndex&)));
}

//-----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setActiveView(pqView* view)
{
  this->PipelineModel->setView(view);
}

//-----------------------------------------------------------------------------
bool pqPipelineBrowserWidget::eventFilter(QObject* object, QEvent* eventArg)
{
  if (object == this && eventArg->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(eventArg);
    if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
    {
      Q_EMIT this->deleteKey();
    }
  }

  return this->Superclass::eventFilter(object, eventArg);
}

//----------------------------------------------------------------------------
bool pqPipelineBrowserWidget::viewportEvent(QEvent* evt)
{
  if (evt->type() == QEvent::FontChange)
  {
    // Pass the changed font to the model otherwise it doesn't use
    // correct font for modified items.
    QFont modifiedFont = this->font();
    modifiedFont.setBold(true);
    this->PipelineModel->setModifiedFont(modifiedFont);
  }
  return this->Superclass::viewportEvent(evt);
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::handleIndexClicked(const QModelIndex& index_)
{
  // we make sure we are only clicking on an eye
  if (index_.column() == 1)
  {
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;

    // Get object relative to pqPipelineModel
    const pqPipelineModel* model = this->getPipelineModel(index_);
    QModelIndex index = this->pipelineModelIndex(index_);

    // We need to obtain the source to give the undo element some sensible name.
    pqServerManagerModelItem* smModelItem = model->getItemFor(index);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(smModelItem);
    pqOutputPort* port =
      source ? source->getOutputPort(0) : qobject_cast<pqOutputPort*>(smModelItem);
    if (port)
    {
      pqView* activeView = pqActiveObjects::instance().activeView();
      vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : nullptr;
      bool cur_state = (viewProxy == nullptr
          ? false
          : (controller->GetVisibility(port->getSourceProxy(), port->getPortNumber(), viewProxy)));

      bool new_visibility_state = !cur_state;
      bool is_selected = false;
      QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
      Q_FOREACH (QModelIndex selIndex_, indexes)
      {
        // Convert index to pqPipelineModel
        QModelIndex selIndex = this->pipelineModelIndex(selIndex_);

        if (selIndex.row() == index.row() && selIndex.parent() == index.parent())
        {
          is_selected = true;
          break;
        }
      }
      if (is_selected)
      {
        this->setVisibility(new_visibility_state, indexes);
      }
      else
      {
        // although there's a selected group of objects, the user clicked on the
        // eye for some other item. In that case, we only affect the clicked
        // item.
        QModelIndexList indexes2;
        indexes2 << index;
        this->setVisibility(new_visibility_state, indexes2);
        // change the selection to the item, if we just made it visible.
        if (new_visibility_state)
        {
          QModelIndex itemIndex = this->getModel()->index(index_.row(), 0, index_.parent());
          this->getSelectionModel()->setCurrentIndex(
            itemIndex, QItemSelectionModel::ClearAndSelect);
        }
      }
    }
    else if (auto extractor = qobject_cast<pqExtractor*>(smModelItem))
    {
      // toggle enabled state for the extractor.
      auto activeView = pqActiveObjects::instance().activeView();
      extractor->toggleEnabledState(activeView);
    }
  }
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setSelectionVisibility(bool visible)
{
  QModelIndexList indexes = this->getSelectionModel()->selectedIndexes();
  this->setVisibility(visible, indexes);
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setVisibility(bool visible, const QModelIndexList& indexes)
{
  bool begun_undo_set = false;

  Q_FOREACH (QModelIndex index_, indexes)
  {
    // Get object relative to pqPipelineModel
    const pqPipelineModel* model = this->getPipelineModel(index_);
    QModelIndex index = this->pipelineModelIndex(index_);

    pqServerManagerModelItem* smModelItem = model->getItemFor(index);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(smModelItem);
    pqOutputPort* port =
      source ? source->getOutputPort(0) : qobject_cast<pqOutputPort*>(smModelItem);

    if (port)
    {
      if (!begun_undo_set)
      {
        begun_undo_set = true;
        if (indexes.size() == 1)
        {
          source = port->getSource();
          BEGIN_UNDO_SET((visible ? tr("Show %1").arg(source->getSMName())
                                  : tr("Hide %1").arg(source->getSMName())));
        }
        else
        {
          BEGIN_UNDO_SET((visible ? tr("Show Selected") : tr("Hide Selected")));
        }
      }
      pqPipelineBrowserWidget::setVisibility(visible, port);
    }
  }
  if (begun_undo_set)
  {
    END_UNDO_SET();
  }
  if (pqView* view = pqActiveObjects::instance().activeView())
  {
    if (view->getNumberOfVisibleDataRepresentations() == 1 && visible &&
      vtkPVGeneralSettings::GetInstance()->GetResetDisplayEmptyViews())
    {
      view->resetDisplay();
    }
    pqActiveObjects::instance().activeView()->render();
  }
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::contextMenuEvent(QContextMenuEvent* e)
{
  this->setFocus(Qt::OtherFocusReason);

  this->ContextMenu->exec(this->mapToGlobal(e->pos()));
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setVisibility(bool visible, pqOutputPort* port)
{
  if (port)
  {
    auto& activeObjects = pqActiveObjects::instance();
    vtkNew<vtkSMParaViewPipelineControllerWithRendering> controller;
    pqView* activeView = activeObjects.activeView();
    vtkSMViewProxy* viewProxy = activeView ? activeView->getViewProxy() : nullptr;
    int scalarBarMode = vtkPVGeneralSettings::GetInstance()->GetScalarBarMode();

    if (pqLiveInsituManager::isInsituServer(port->getServer()))
    {
      // we don't need to add an extract for writer parameters proxies.
      if (!pqLiveInsituManager::isWriterParametersProxy(port->getSourceProxy()))
      {
        pqLiveInsituVisualizationManager* mgr =
          pqLiveInsituManager::managerFromInsitu(port->getServer());
        if (mgr && mgr->addExtract(port))
        {
          // refresh the pipeline browser icon.
        }
      }
    }
    else
    {
      if (visible)
      {
        // Make sure the given port is selected specially if we are in
        // multi-server / catalyst configuration type
        activeObjects.setActivePort(port);
      }

      auto activeLayout = activeObjects.activeLayout();
      const auto location = activeObjects.activeLayoutLocation();

      vtkSMProxy* reprProxy = controller->SetVisibility(
        port->getSourceProxy(), port->getPortNumber(), viewProxy, visible);
      if (visible && viewProxy == nullptr && reprProxy)
      {
        // this implies that the controller would have created a new view.
        // let's get that view so we toggle scalar bar visibility in that view
        // and also add it to layout.
        viewProxy = vtkSMViewProxy::FindView(reprProxy);
        controller->AssignViewToLayout(viewProxy, activeLayout, location);
        activeView =
          pqApplicationCore::instance()->getServerManagerModel()->findItem<pqView*>(viewProxy);
      }

      // assign to layout, in case a new view is created.
      // update scalar bars: show new ones if needed. Hiding of scalar bars is
      // taken care of by vtkSMParaViewPipelineControllerWithRendering (I still
      // wonder if that's the best thing to do).
      const QString scalarBarError =
        tr("You might have added a new scalar bar mode, you need to do something "
           "here, skipping.");
      if (scalarBarMode != vtkPVGeneralSettings::MANUAL_SCALAR_BARS)
      {
        // This gets executed if scalar bar mode is
        // AUTOMATICALLY_HIDE_SCALAR_BARS or AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS
        if (visible && vtkSMColorMapEditorHelper::GetUsingScalarColoring(reprProxy))
        {
          const int stickyVisible =
            vtkSMColorMapEditorHelper::IsScalarBarStickyVisible(reprProxy, viewProxy);
          if (stickyVisible != -1)
          {
            vtkSMColorMapEditorHelper::SetScalarBarVisibility(reprProxy, viewProxy, stickyVisible);
          }
          else if (scalarBarMode == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
          {
            vtkSMColorMapEditorHelper::SetScalarBarVisibility(reprProxy, viewProxy, true);
          }
          else if (scalarBarMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS)
          {
            vtkSMColorMapEditorHelper::SetScalarBarVisibility(reprProxy, viewProxy, false);
          }
          else
          {
            qCritical() << scalarBarError << "\n";
          }
        }
        if (visible && vtkSMColorMapEditorHelper::GetAnyBlockUsingScalarColoring(reprProxy))
        {
          const auto selectors = vtkSMColorMapEditorHelper::GetColorArraysBlockSelectors(reprProxy);
          const auto stickyVisibles = vtkSMColorMapEditorHelper::IsBlocksScalarBarStickyVisible(
            reprProxy, viewProxy, selectors);
          if (std::any_of(stickyVisibles.begin(), stickyVisibles.end(),
                [](int stickyVisible) { return stickyVisible != -1; }))
          {
            for (size_t i = 0; i < selectors.size(); ++i)
            {
              if (stickyVisibles[i] != -1)
              {
                vtkSMColorMapEditorHelper::SetBlockScalarBarVisibility(
                  reprProxy, viewProxy, selectors[i], stickyVisibles[i]);
              }
              else if (scalarBarMode ==
                vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
              {
                vtkSMColorMapEditorHelper::SetBlockScalarBarVisibility(
                  reprProxy, viewProxy, selectors[i], true);
              }
              else if (scalarBarMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS)
              {
                vtkSMColorMapEditorHelper::SetBlockScalarBarVisibility(
                  reprProxy, viewProxy, selectors[i], false);
              }
              else
              {
                qCritical() << scalarBarError << "\n";
              }
            }
          }
          else if (scalarBarMode == vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS)
          {
            vtkSMColorMapEditorHelper::SetBlocksScalarBarVisibility(
              reprProxy, viewProxy, selectors, true);
          }
          else if (scalarBarMode == vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS)
          {
            vtkSMColorMapEditorHelper::SetBlocksScalarBarVisibility(
              reprProxy, viewProxy, selectors, false);
          }
          else
          {
            qCritical() << scalarBarError << "\n";
          }
        }
      }

      // BUG #20521. Toggle interaction mode for 3D views; @sa pqApplyBehavior.
      auto rview = qobject_cast<pqRenderView*>(activeView);
      if (visible && rview != nullptr && rview->getNumberOfVisibleDataRepresentations() == 1)
      {
        rview->updateInteractionMode(port);
      }
    }
  }
}

//----------------------------------------------------------------------------
QMenu* pqPipelineBrowserWidget::contextMenu() const
{
  return this->ContextMenu;
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setAnnotationFilterMatching(bool matching)
{
  this->FilteredPipelineModel->setAnnotationFilterMatching(matching);
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::enableAnnotationFilter(const QString& annotationKey)
{
  this->FilteredPipelineModel->enableAnnotationFilter(annotationKey);
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::disableAnnotationFilter()
{
  this->FilteredPipelineModel->disableAnnotationFilter();
}
//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::enableSessionFilter(vtkSession* session)
{
  this->FilteredPipelineModel->enableSessionFilter(session);
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::disableSessionFilter()
{
  this->FilteredPipelineModel->disableSessionFilter();
}

//----------------------------------------------------------------------------
QModelIndex pqPipelineBrowserWidget::pipelineModelIndex(const QModelIndex& index) const
{
  if (qobject_cast<const pqPipelineModel*>(index.model()))
  {
    return index;
  }
  const QSortFilterProxyModel* filterModel =
    qobject_cast<const QSortFilterProxyModel*>(index.model());
  assert("Invalid model used inside index" && filterModel);

  // Make a recursive call to support unknown filter depth
  return this->pipelineModelIndex(filterModel->mapToSource(index));
}

//----------------------------------------------------------------------------
const pqPipelineModel* pqPipelineBrowserWidget::getPipelineModel(const QModelIndex& index) const
{
  if (const pqPipelineModel* model = qobject_cast<const pqPipelineModel*>(index.model()))
  {
    return model;
  }

  const QSortFilterProxyModel* filterModel =
    qobject_cast<const QSortFilterProxyModel*>(index.model());
  assert("Invalid model used inside index" && filterModel);

  // Make a recusrive call to support unknown filter depth
  return this->getPipelineModel(filterModel->mapToSource(index));
}

//----------------------------------------------------------------------------
void pqPipelineBrowserWidget::expandWithModelIndexTranslation(const QModelIndex& index)
{
  this->expand(this->FilteredPipelineModel->mapFromSource(index));
}

//-----------------------------------------------------------------------------
void pqPipelineBrowserWidget::setModel(pqPipelineModel* model)
{
  if (!model)
    return;

  delete this->PipelineModel;
  this->PipelineModel = model;
  this->PipelineModel->setParent(this);

  this->configureModel();
}
