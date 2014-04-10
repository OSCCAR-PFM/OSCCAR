/*=========================================================================

  Program:   ParaView
  Module:    vtkKdTreeManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKdTreeManager - class used to generate KdTree from unstructured or
// structured data.
// .SECTION Description
// ParaView needs to build a KdTree when ordered compositing. The KdTree is
// either built using the all data in the pipeline when on structure data is
// present, or using the partitions provided by the structure data's extent
// translator. This class manages this logic. When structure data's extent
// translator is to be used, it simply uses vtkKdTreeGenerator. Otherwise, it
// lets the vtkPKdTree build the optimal partitioning for the data.

#ifndef __vtkKdTreeManager_h
#define __vtkKdTreeManager_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkPKdTree;
class vtkAlgorithm;
class vtkDataSet;
class vtkDataObject;
class vtkExtentTranslator;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkKdTreeManager : public vtkObject
{
public:
  static vtkKdTreeManager* New();
  vtkTypeMacro(vtkKdTreeManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add data objects.
  void AddDataObject(vtkDataObject*);
  void RemoveAllDataObjects();

  // Description:
  // Set the optional extent translator to use to get aid in building the
  // KdTree.
  void SetStructuredDataInformation(
    vtkExtentTranslator* translator,
    const int whole_extent[6],
    const double origin[3], const double spacing[3]);

  // Description:
  // Get/Set the KdTree managed by this manager.
  void SetKdTree(vtkPKdTree*);
  vtkGetObjectMacro(KdTree, vtkPKdTree);

  // Description:
  // Get/Set the number of pieces. 
  // Passed to the vtkKdTreeGenerator when SetStructuredDataInformation() is
  // used with non-empty translator.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

  // Description:
  // Rebuilds the KdTree.
  void GenerateKdTree();

//BTX
protected:
  vtkKdTreeManager();
  ~vtkKdTreeManager();

  void AddDataObjectToKdTree(vtkDataObject *data);
  void AddDataSetToKdTree(vtkDataSet *data);

  bool KdTreeInitialized;
  vtkPKdTree* KdTree;
  int NumberOfPieces;

  vtkSmartPointer<vtkExtentTranslator> ExtentTranslator;
  double Origin[3];
  double Spacing[3];
  int WholeExtent[6];

  vtkSetVector3Macro(Origin, double);
  vtkSetVector3Macro(Spacing, double);
  vtkSetVector6Macro(WholeExtent, int);

private:
  vtkKdTreeManager(const vtkKdTreeManager&); // Not implemented
  void operator=(const vtkKdTreeManager&); // Not implemented

  class vtkDataObjectSet;
  vtkDataObjectSet* DataObjects;

//ETX
};

#endif
