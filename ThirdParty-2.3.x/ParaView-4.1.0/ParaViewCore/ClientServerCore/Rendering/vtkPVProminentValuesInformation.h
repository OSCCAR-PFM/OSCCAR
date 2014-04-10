/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProminentValuesInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProminentValuesInformation - Prominent values a data array takes on.
// .SECTION Description
// This vtkPVInformation subclass provides a way for clients to discover
// whether a specific remote vtkAbstractArray instance behaves like a
// discrete set or a continuum (for each component of its tuples as well
// as for its tuples as a whole).
//
// If the array behaves discretely (which we define to be: takes on fewer
// than 33 distinct values over more than 99.9% of its entries to within a
// given confidence that dictates the number of samples required), then
// the prominent values are also made available.
//
// This class uses vtkAbstractArray::GetProminentComponentValues().

#ifndef __vtkPVProminentValuesInformation_h
#define __vtkPVProminentValuesInformation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkAbstractArray;
class vtkClientServerStream;
class vtkCompositeDataSet;
class vtkDataObject;
class vtkStdString;
class vtkStringArray;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVProminentValuesInformation : public vtkPVInformation
{
public:
  static vtkPVProminentValuesInformation* New();
  vtkTypeMacro(vtkPVProminentValuesInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the output port whose dataset should be queried.
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);

  // Description:
  // Set/get array's association
  vtkSetStringMacro(FieldAssociation);
  vtkGetStringMacro(FieldAssociation);

  // Description:
  // Set/get array's name
  vtkSetStringMacro(FieldName);
  vtkGetStringMacro(FieldName);

  // Description:
  // Changing the number of components clears the ranges back to the default.
  void SetNumberOfComponents(int numComps);
  vtkGetMacro(NumberOfComponents, int);

  // Description:
  // Set/get the minimum fraction of the array that should be composed of
  // a value (between 0 and 1) in order for it to be considered prominent.
  //
  // Setting this to one indicates that an array must have every value be
  // identical in order to have any considered prominent.
  vtkSetClampMacro(Fraction,double,0.,1.);
  vtkGetMacro(Fraction,double);

  // Description:
  // Set/get the maximum uncertainty allowed in the detection of prominent values.
  // The uncertainty is the probability of prominent values going undetected.
  // Setting this to zero forces the entire array to be inspected.
  vtkSetClampMacro(Uncertainty,double,0.,1.);
  vtkGetMacro(Uncertainty,double);

  // Description:
  // Returns 1 if the array can be combined.
  // It must have the same name and number of components.
  int Compare(vtkPVProminentValuesInformation* info);

  // Description:
  // Copy information from an \a other object.
  void DeepCopy(vtkPVProminentValuesInformation* other);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Transfer information about a single vtkAbstractArray's prominent values into this object.
  //
  // This is called *after* CopyFromObject has determined the number of components available;
  // this method relies on this->NumberOfComponents being valid.
  virtual void CopyDistinctValuesFromObject(vtkAbstractArray*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation* other);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Push/pop parameters controlling which array to sample onto/off of the stream.
  virtual void CopyParametersToStream(vtkMultiProcessStream&);
  virtual void CopyParametersFromStream(vtkMultiProcessStream&);

  // Description:
  // Remove all parameter information.
  //
  // You must copy/set parameter values before adding data or copying data from an object.
  void InitializeParameters();

  // Description:
  // Remove all gathered information (but not parameters). Next add will behave like a copy.
  void Initialize();

  // Description:
  // Merge another list of prominent values.
  void AddDistinctValues(vtkPVProminentValuesInformation*);

  // Description:
  // Returns either NULL (array component appears to be continuous) or
  // a pointer to a vtkAbstractArray (array component appears to be discrete)
  // containing a sorted list of all distinct prominent values encountered in
  // the array component.
  //
  // Passing -1 as the component will return information about distinct tuple values
  // as opposed to distinct component values.
  vtkAbstractArray* GetProminentComponentValues(int component);

protected:
  vtkPVProminentValuesInformation();
  ~vtkPVProminentValuesInformation();

  void DeepCopyParameters(vtkPVProminentValuesInformation* other);
  void CopyFromCompositeDataSet(vtkCompositeDataSet*);
  void CopyFromLeafDataObject(vtkDataObject*);

  /// Information parameters
  //@{
  int PortNumber;
  int NumberOfComponents;
  char* FieldName;
  char* FieldAssociation;
  double Fraction;
  double Uncertainty;
  //@}

  /// Information results
  //@{
  //BTX
  class vtkInternalDistinctValues;
  vtkInternalDistinctValues* DistinctValues;
  //ETX
  //@}

  vtkPVProminentValuesInformation(const vtkPVProminentValuesInformation&); // Not implemented
  void operator=(const vtkPVProminentValuesInformation&); // Not implemented
};

#endif
