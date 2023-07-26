// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqManageLinksReaction.h"

#include "pqCoreUtilities.h"
#include "pqLinksManager.h"

//-----------------------------------------------------------------------------
void pqManageLinksReaction::manageLinks()
{
  pqLinksManager dialog(pqCoreUtilities::mainWidget());
  dialog.setWindowTitle(tr("Link Manager"));
  dialog.setObjectName("pqLinksManager");
  dialog.exec();
}
