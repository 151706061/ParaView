// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPMultiResolutionGenericIOReader
 *
 * This reader handles multiple GenericIO files that are different resolutions
 * of the same dataset.  This reader aggregates these and allows streaming
 * different resolutions on different parts of the dataset.  It has the
 * concept of a resolution level with 0 being the lowest resolution and the
 * resolution increases as the level number increases.
 */

#ifndef vtkPMultiResolutionGenericIOReader_h
#define vtkPMultiResolutionGenericIOReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkStringArray;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPMultiResolutionGenericIOReader
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPMultiResolutionGenericIOReader* New();
  vtkTypeMacro(vtkPMultiResolutionGenericIOReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual bool CanReadFile(const char* fileName);

  ///@{
  /**
   * Sets/Gets the filename to be read by this reader
   */
  void SetFileName(const char* fname);
  vtkGetStringMacro(FileName);
  ///@}

  void SetXAxisVariableName(const char* arg);
  vtkGetStringMacro(XAxisVariableName);
  void SetYAxisVariableName(const char* arg);
  vtkGetStringMacro(YAxisVariableName);
  void SetZAxisVariableName(const char* arg);
  vtkGetStringMacro(ZAxisVariableName);

  vtkStringArray* GetArrayList();
  /**
   * This function inserts the given file as a resolution level on this reader.
   * NOTE: 0 is lowest resolution and the resolution should increase from
   * there.
   */
  bool InsertLevel(const char* fileName, int level);

  /**
   * Gets the number of resolution levels known by this reader
   */
  int GetNumberOfLevels() const;
  /**
   * Gets the filename for the given resolution level
   */
  const char* GetFileNameForLevel(int level) const;
  /**
   * Clears all resolution levels
   */
  void RemoveAllLevels();

  ///@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  ///@}

  /**
   * Returns the number of arrays in the file
   */
  int GetNumberOfPointArrays();

  /**
   * Returns the name of the ith array
   */
  const char* GetPointArrayName(int i);

  /**
   * Returns the status of the array corresponding to the given name.
   */
  int GetPointArrayStatus(const char* name);

  /**
   * Sets the status of the array corresponding to the given name.
   */
  void SetPointArrayStatus(const char* name, int status);

protected:
  vtkPMultiResolutionGenericIOReader();
  ~vtkPMultiResolutionGenericIOReader();

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName;
  char* XAxisVariableName;
  char* YAxisVariableName;
  char* ZAxisVariableName;

  vtkDataArraySelection* PointDataArraySelection;
  vtkCallbackCommand* SelectionObserver;

private:
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  class vtkInternal;
  vtkInternal* Internal;

  vtkPMultiResolutionGenericIOReader(const vtkPMultiResolutionGenericIOReader&) = delete;
  void operator=(const vtkPMultiResolutionGenericIOReader&) = delete;
};

#endif
