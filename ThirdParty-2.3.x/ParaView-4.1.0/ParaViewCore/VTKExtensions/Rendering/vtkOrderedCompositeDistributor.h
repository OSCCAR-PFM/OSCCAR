/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedCompositeDistributor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2005 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkOrderedCompositeDistributor
//
// .SECTION Description
//
// This class distributes data for use with ordered compositing (i.e. with
// IceT).  The composite distributor takes the same vtkPKdTree
// that IceT and will use that to distribute the data.
//
// Input poly data will be converted back to poly data on the output.
//
// This class also has an optional pass through mode to make it easy to
// turn ordered compositing on and off.
//

#ifndef __vtkOrderedCompositeDistributor_h
#define __vtkOrderedCompositeDistributor_h

#include "vtkPointSetAlgorithm.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkBSPCuts;
class vtkDataSet;
class vtkDataSetSurfaceFilter;
class vtkDistributedDataFilter;
class vtkMultiProcessController;
class vtkPKdTree;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkOrderedCompositeDistributor : public vtkPointSetAlgorithm
{
public:
  vtkTypeMacro(vtkOrderedCompositeDistributor, vtkPointSetAlgorithm);
  static vtkOrderedCompositeDistributor *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set the vtkPKdTree to distribute with.
  virtual void SetPKdTree(vtkPKdTree *);
  vtkGetObjectMacro(PKdTree, vtkPKdTree);

  // Description:
  // Set/get the controller to distribute with.
  virtual void SetController(vtkMultiProcessController *);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // When on, data is passed through without compositing.
  vtkSetMacro(PassThrough, bool);
  vtkGetMacro(PassThrough, bool);
  vtkBooleanMacro(PassThrough, bool);

  // Description:
  // When non-null, the output will be converted to the given type.
  vtkSetStringMacro(OutputType);
  vtkGetStringMacro(OutputType);

protected:
  vtkOrderedCompositeDistributor();
  ~vtkOrderedCompositeDistributor();

  char *OutputType;
  bool PassThrough;
  vtkPKdTree *PKdTree;
  vtkMultiProcessController *Controller;
 
  int FillInputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkOrderedCompositeDistributor(const vtkOrderedCompositeDistributor &);  // Not implemented.
  void operator=(const vtkOrderedCompositeDistributor &);  // Not implemented.
};

#endif //__vtkOrderedCompositeDistributor_h
