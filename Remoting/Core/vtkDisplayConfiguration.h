// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkDisplayConfiguration
 * @brief display configuration container
 *
 * vtkDisplayConfiguration is a container used by vtkRemotingCoreConfiguration
 * to manage display configurations which are needed for CAVE and other
 * immersive displays.
 */

#ifndef vtkDisplayConfiguration_h
#define vtkDisplayConfiguration_h

#include "vtkObject.h"
#include "vtkRemotingCoreModule.h" // for exports
#include "vtkTuple.h"              // for vtkTuple
#include <memory>                  // for std::unique_ptr

class VTKREMOTINGCORE_EXPORT vtkDisplayConfiguration : public vtkObject
{
public:
  static vtkDisplayConfiguration* New();
  vtkTypeMacro(vtkDisplayConfiguration, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get whether to use window borders / frames are to be shown.
   */
  vtkGetMacro(ShowBorders, bool);

  /**
   * Returns true of each display should simply be full-screen. If so, display
   * geometry is ignored.
   */
  vtkGetMacro(FullScreen, bool);

  /**
   * Get eye separation.
   */
  vtkGetMacro(EyeSeparation, double);

  /**
   * Returns the count for display specified.
   */
  int GetNumberOfDisplays() const;

  ///@{
  /**
   * Returns environment string for the given display index.
   */
  const char* GetEnvironment(int index) const;
  ///@}

  ///@{
  /**
   * Returns geometry (screen space size and position) for the given display
   * index.
   */
  vtkTuple<int, 4> GetGeometry(int index) const;
  ///@}

  ///@{
  /**
   * Returns positions of screen corners (in physical coordinates) for the
   * given display.
   */
  vtkTuple<double, 3> GetLowerLeft(int index) const;
  vtkTuple<double, 3> GetLowerRight(int index) const;
  vtkTuple<double, 3> GetUpperRight(int index) const;
  ///@}

  ///@{
  /**
   * Returns whether given display index has lower left, lower right, and
   * upper right screen corners defined.
   */
  bool GetHasCorners(int index) const;
  ///@}

  ///@{
  /**
   * Only used on X windows systems. Indicates that the window should be
   * manageable by the window manager.  This means it can be covered by
   * other windows, and a taskbar item is available to bring it back to the
   * front.
   */
  bool GetCoverable(int index) const;
  ///@}

  ///@{
  /**
   * Allows user to select which displays show/hide things like scalar bar,
   * annotation, etc.
   */
  bool GetShow2DOverlays(int index) const;
  ///@}

  /**
   * Parses a PVX file to load display configuration information.
   */
  bool LoadPVX(const char* fname);

protected:
  vtkDisplayConfiguration();
  ~vtkDisplayConfiguration() override;

private:
  vtkDisplayConfiguration(const vtkDisplayConfiguration&) = delete;
  void operator=(const vtkDisplayConfiguration&) = delete;

  bool ShowBorders = false;
  bool Coverable = false;
  bool FullScreen = false;
  double EyeSeparation = 0.0;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

#endif
