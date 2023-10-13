// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPythonMacroSupervisor_h
#define pqPythonMacroSupervisor_h

#include "pqPythonModule.h"

#include "vtkParaViewDeprecation.h" // for deprecation

#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>

class QAction;

class PQPYTHON_EXPORT pqPythonMacroSupervisor : public QObject
{
  Q_OBJECT
public:
  pqPythonMacroSupervisor(QObject* p = nullptr);
  ~pqPythonMacroSupervisor() override;

  // Description:
  // Add a widget to be given macro actions.  QActions representing script macros
  // will be added to the widget.  This could be a QToolBar, QMenu, or other type
  // of widget.
  void addWidgetForRunMacros(QWidget* widget);

  // Description:
  // Add a widget to be given macro actions.  QActions representing script macros
  // will be added to the widget.  This could be a QToolBar, QMenu, or other type
  // of widget.
  void addWidgetForEditMacros(QWidget* widget);

  // Description:
  // Add a widget to be given macro actions.  QActions representing script macros
  // will be added to the widget.  This could be a QToolBar, QMenu, or other type
  // of widget.
  void addWidgetForDeleteMacros(QWidget* widget);

  // Description:
  // Lookup and return a macro action by filename.
  // If it does not exist, return null.
  QAction* getMacro(const QString& fileName);

  // Description:
  // Get macros from known macro directories (see getMacrosFilePaths)
  // In the returned map, the keys are filenames and values are macro names.
  static QMap<QString, QString> getStoredMacros();

  // Description:
  // Hide file by prepending a `.` to its name.
  // Hidden file in macro directory are not loaded.
  PARAVIEW_DEPRECATED_IN_5_12_0("Use hideFile instead.")
  static void removeStoredMacro(const QString& filename);

  // Description:
  // Hide file by prepending a `.` to its name.
  // Hidden file in macro directory are not loaded.
  static void hideFile(const QString& filename);

  // Description:
  // Get a macro name from the fileName
  static QString macroNameFromFileName(const QString& filename);

  // Description:
  // Get an icon name from the fileName
  // If no corresponding icon, return an empty string
  static QString iconNameFromFileName(const QString& filename);

  // Description:
  // Get a list a "*.py" files from macro directories.
  static QStringList getMacrosFilePaths();

  static QStringList getSupportedIconFormats()
  {
    return QStringList() << ".svg"
                         << ".png";
  }

Q_SIGNALS:

  // Description:
  // Emitted when a macro has been triggered.
  void executeScriptRequested(const QString& fileName);

  /**
   * Emitted when a macro has been added.
   */
  void onAddedMacro();

  // Description:
  // Emitted when a macro has to be edited
  void onEditMacro(const QString& filename);

public Q_SLOTS:

  // Description:
  // Add an action with the given name and filename.  If there is already
  // a macro with the given filename it's macroname will be updated to the
  // one given.  Macro names do not have to be unique.
  void addMacro(const QString& macroName, const QString& filename);
  void addMacro(const QString& filename);

  // Description:
  // Remove an action from the UI, with the given filename. Note, this does not
  // remove the macro from a future load, you must call hideFile yourself
  // (or manually remove the file from the settings dir).
  void removeMacro(const QString& filename);

  // Description:
  // Update Macro list widgets and actions...
  void updateMacroList();

protected Q_SLOTS:

  // Description:
  // If the sender is a QAction managed by this class, the filename will be
  // looked up and the signal requestExecuteScript will be emitted.
  void onMacroTriggered();

  // Description:
  // If the sender is a QAction managed by this class, the filename will be
  // moved (deleted), and the macro will be removed
  void onDeleteMacroTriggered();

  // Description:
  // If the sender is a QAction managed by this class, the macro file will be
  // open in a python edit
  void onEditMacroTriggered();

protected: // NOLINT(readability-redundant-access-specifiers)
  // Description:
  // Add a widget to be given macro actions.  QActions representing script macros
  // will be added to the widget.  This could be a QToolBar, QMenu, or other type
  // of widget.
  void addWidgetForMacros(QWidget* widget, int actionType); // 0:run, 1:edit, 2:delete

  // Description:
  // Removes all actions and re-adds actions for each macro stored.
  void resetActions();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif // ifndef pqPythonMacroSupervisor_h
