// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "vtkPVInformationKeys.h"

#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationStringKey.h"

vtkInformationKeyMacro(vtkPVInformationKeys, TIME_LABEL_ANNOTATION, String);
vtkInformationKeyRestrictedMacro(vtkPVInformationKeys, WHOLE_BOUNDING_BOX, DoubleVector, 6);
