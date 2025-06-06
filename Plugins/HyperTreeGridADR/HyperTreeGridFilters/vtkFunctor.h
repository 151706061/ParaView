// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkFunctor_h
#define vtkFunctor_h

#include "vtkFiltersHyperTreeGridADRModule.h" // For export macro

#include <cmath>

#define vtkFunctorCompMacro                                                                        \
  template <class FunctorT>                                                                        \
  inline bool operator==(const FunctorT&) const                                                    \
  {                                                                                                \
    return false;                                                                                  \
  }                                                                                                \
  template <class FunctorT>                                                                        \
  inline bool operator!=(const FunctorT& f) const                                                  \
  {                                                                                                \
    return !(*this == f);                                                                          \
  }

#define vtkDefaultSpecializationFunctorCompMacro(functor)                                          \
  inline bool operator==(const functor&) const                                                     \
  {                                                                                                \
    return true;                                                                                   \
  }

/**
 * @class   vtkIdentityFunctor
 * @brief   Functor f(x) = x
 */

struct VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkIdentityFunctor
{
  double operator()(double x) { return x; }
  vtkFunctorCompMacro;
  vtkDefaultSpecializationFunctorCompMacro(vtkIdentityFunctor);
};

/**
 * @class   vtkSquareFunctor
 * @brief   Functor f(x) = x^2
 */
struct VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkSquareFunctor
{
  double operator()(double x) { return x * x; }
  vtkFunctorCompMacro;
  vtkDefaultSpecializationFunctorCompMacro(vtkSquareFunctor);
};

/**
 * @class   vtkLogFunctor
 * @brief   Functor f(x) = log(x)
 */
struct VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkLogFunctor
{
  double operator()(double x) { return std::log(x); }
  vtkFunctorCompMacro;
  vtkDefaultSpecializationFunctorCompMacro(vtkLogFunctor);
};

/**
 * @class   vtkInverseFunctor
 * @brief   Functor f(x) = 1/x
 */
struct VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkInverseFunctor
{
  double operator()(double x) { return 1.0 / x; }
  vtkFunctorCompMacro;
  vtkDefaultSpecializationFunctorCompMacro(vtkInverseFunctor);
};

/**
 * @class   vtkEntropyFunctor
 * @brief   Functor f(x) = x log(x)
 */
struct VTKFILTERSHYPERTREEGRIDADR_EXPORT vtkEntropyFunctor
{
  double operator()(double x) { return x * std::log(x); }
  vtkFunctorCompMacro;
  vtkDefaultSpecializationFunctorCompMacro(vtkEntropyFunctor);
};

#undef vtkFunctorCompMacro
#undef vtkDefaultSpecializationFunctorCompMacro

#endif

// VTK-HeaderTest-Exclude: vtkFunctor.h
