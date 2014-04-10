/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef __vtkVRInteractorStyleFactory_h
#define __vtkVRInteractorStyleFactory_h

#include "vtkObject.h"

#include <string>
#include <vector>

class vtkVRInteractorStyle;

class vtkVRInteractorStyleFactory : public vtkObject
{
public:
  static vtkVRInteractorStyleFactory *New();
  vtkTypeMacro(vtkVRInteractorStyleFactory, vtkObject)
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get the singleton instance of this class
  static vtkVRInteractorStyleFactory *GetInstance();

  // Description:
  // Get the list of interactor style classes
  std::vector<std::string> GetInteractorStyleClassNames();

  // Description:
  // Get a list of action descriptions for the styles (e.g. "Grab", "Track",
  // etc).
  std::vector<std::string> GetInteractorStyleDescriptions();

  // Description:
  // Get the action description for a style classname
  std::string GetDescriptionFromClassName(const std::string &className);

  // Description:
  // Create a new interactor style instance. The input string
  // must be in the vector returned by GetInteractorStyleClassNames().
  vtkVRInteractorStyle *NewInteractorStyleFromClassName(const std::string &);

  // Description:
  // Create a new interactor style instance. The input string
  // must be in the vector returned by GetInteractorStyleDescriptions().
  vtkVRInteractorStyle *NewInteractorStyleFromDescription(const std::string &);

  friend class pqVRStarter;

protected:
  vtkVRInteractorStyleFactory();
  ~vtkVRInteractorStyleFactory();

  static void SetInstance(vtkVRInteractorStyleFactory *);
  static vtkVRInteractorStyleFactory *Instance;

  std::vector<std::string> InteractorStyleClassNames;
  std::vector<std::string> InteractorStyleDescriptions;

};

#endif // __vtkVRInteractorStyleFactory_h
