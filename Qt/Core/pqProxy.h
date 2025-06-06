// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * \file pqProxy.h
 *
 * \date 11/16/2005
 */

#ifndef pqProxy_h
#define pqProxy_h

#include "pqServerManagerModelItem.h"
#include <QPointer>

class pqProxyInternal;
class pqServer;
class vtkPVXMLElement;
class vtkSMProxy;
class vtkSMSessionProxyManager;

/**
 * This class represents any registered Server Manager proxy.
 * It keeps essential information to locate the proxy as well as additional
 * metadata such as user-specified label.
 */
class PQCORE_EXPORT pqProxy : public pqServerManagerModelItem
{
  Q_OBJECT
public:
  /**
   * The modification state of this proxy
   */
  enum ModifiedState
  {
    UNINITIALIZED,
    MODIFIED,
    UNMODIFIED
  };

  pqProxy(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = nullptr);
  ~pqProxy() override;

  /**
   * Get the server on which this proxy exists.
   */
  pqServer* getServer() const;

  /**
   * This is a convenience method. It re-registers the underlying proxy with
   * the requested new name under the same group. Then it unregisters the proxy
   * from the group with the old name. This operation is understood as renaming
   * the proxy, since as a consequence, this pqProxy's \c SMName changes.
   */
  void rename(const QString& newname);

  /**
   * Get the name with which this proxy is registered on the server manager. A
   * proxy can be registered with more than one name on the Server Manager.
   * This is the name/group which this pqProxy stands for.
   */
  const QString& getSMName();
  const QString& getSMGroup();

  /**
   * Get the vtkSMProxy this object stands for.
   * This can never be nullptr. A pqProxy always represents one and only one
   * Server Manager proxy.
   */
  vtkSMProxy* getProxy() const;

  /**
   * \brief
   *   Gets whether or not the source has been modified.
   * \return
   *   True if the source has been modified.
   */
  ModifiedState modifiedState() const { return this->Modified; }

  /**
   * \brief
   *   Sets whether or not the source has been modified.
   * \param modified True if the source has been modified.
   */
  void setModifiedState(ModifiedState modified);

  /**
   * Returns the hints for this proxy, if any. May returns nullptr if no hints
   * are defined.
   */
  vtkPVXMLElement* getHints() const;

  /**
   * Returns a list of all helper proxies.
   */
  QList<vtkSMProxy*> getHelperProxies() const;

  /**
   * Returns a list of all the helper proxies added with a given key.
   */
  QList<vtkSMProxy*> getHelperProxies(const QString& key) const;

  /**
   * Returns the keys for helper proxies.
   */
  QList<QString> getHelperKeys() const;

  /**
   * Concept of helper proxies:
   * A pqProxy is created for every important vtkSMProxy registered. Many a times,
   * there may be other proxies associated with that proxy, eg. lookup table proxies,
   * implicit function proxies may be associated with a filter/source proxy.
   * The GUI can create "associated" proxies and add them as helper proxies.
   * Helper proxies get registered under special groups, so that they are
   * undo/redo-able, and state save-restore-able. The pqProxy makes sure that
   * the helper proxies are unregistered when the main proxy is unregistered.
   */
  virtual void addHelperProxy(const QString& key, vtkSMProxy*);
  void removeHelperProxy(const QString& key, vtkSMProxy*);

  /**
   * Updates the internal datastructures using the proxies currently registered
   * under the group that would be used for helper proxies. This makes it
   * possible to locate helper proxies created from Python.
   */
  void updateHelperProxies() const;

  /**
   * Returns the proxy manager by calling this->getProxy()->GetProxyManager();
   */
  vtkSMSessionProxyManager* proxyManager() const;

  /**
   * Returns a pqProxy instance, of any, whose helper proxy is the *aproxy*.
   * This is not the fastest implementation, so beware of that.
   * If found, the key is set to the helper key.
   */
  static pqProxy* findProxyWithHelper(vtkSMProxy* aproxy, QString& key);

  /**
   * Returns a pqProxy found under current pqServer and matching given vtkSMProxy.
   * Returns nullptr if no such pqproxy is found.
   */
  static pqProxy* findProxy(vtkSMProxy* aproxy);

  /**
   * Return whether or not the user has modified the GUI name of the source.
   * This is needed when reading in a dataset with Catalyst channel information
   * so that we can make sure that we don't overwrite any QUI name the user
   * has already modified. The situation where this could happen is if the
   * user loads a file, changes the GUI name, and then  hits the Apply button.
   * In this situation we don't want to change the GUI name to the Catalyst
   * channel name but every other situation we do.
   */
  bool userModifiedSMName() { return this->UserModifiedSMName; }

Q_SIGNALS:
  /**
   * Fired when the name of the proxy is changed.
   */
  void nameChanged(pqServerManagerModelItem*);

  /**
   * Fired when the modified status changes for the proxy.
   */
  void modifiedStateChanged(pqServerManagerModelItem*);

protected:
  friend class pqServerManagerModel;

  /**
   * Make this pqProxy take on a new identity. This is following case:
   * Proxy A registered as (gA, nA), then is again registered as (gA, nA2).
   * pqServerManagerModel does not create a new pqProxy for (gA, nA2).
   * However, if (gA, nA) is now unregistered, the same old instance of pqProxy
   * which represented (gA, nA) will now "take on a new identity" and
   * represent proxy (gA, nA2). This method will trigger the
   * nameChanged() signal.
   */
  void setSMName(const QString& new_name);

  // Use this method to initialize the pqObject state using the
  // underlying vtkSMProxy. This needs to be done only once,
  // after the object has been created.
  virtual void initialize();

  // Method used to update the internal structure without affecting
  // the ProxyManager proxy registration
  virtual void addInternalHelperProxy(const QString& key, vtkSMProxy*) const;
  virtual void removeInternalHelperProxy(const QString& key, vtkSMProxy*) const;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  // Used to monitor helper proxy registration when created on other clients
  void onProxyRegistered(const QString&, const QString&, vtkSMProxy*);
  void onProxyUnRegistered(const QString&, const QString&, vtkSMProxy*);

private:
  QPointer<pqServer> Server; ///< Stores the parent server.
  QString SMName;
  QString SMGroup;
  pqProxyInternal* Internal;
  ModifiedState Modified;
  bool UserModifiedSMName;
};

#endif
