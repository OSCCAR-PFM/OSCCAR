/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPGenericIOReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPGenericIOReader.h -- Read GenericIO formatted data
//
// .SECTION Description
//  Creates a vtkUnstructuredGrid instance from a GenericIO file.

#ifndef VTKPGENERICIOREADER_H_
#define VTKPGENERICIOREADER_H_

// VTK includes
#include "vtkUnstructuredGridAlgorithm.h" // Base class
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

// Forward Declarations
class vtkCallbackCommand;
class vtkDataArray;
class vtkDataArraySelection;
class vtkGenericIOMetaData;
class vtkInformation;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkStdString;
class vtkStringArray;
class vtkUnstructuredGrid;

// GenericIO Forward Declarations
namespace gio {
  class GenericIOReader;
}

enum IOType {
  IOTYPEMPI,
  IOTYPEPOSIX
};

enum BlockAssignment {
  ROUND_ROBIN,
  RCB
};


class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPGenericIOReader :
  public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPGenericIOReader *New();
  vtkTypeMacro(vtkPGenericIOReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the cosmology particle binary file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(XAxisVariableName);
  vtkGetStringMacro(XAxisVariableName);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(YAxisVariableName);
  vtkGetStringMacro(YAxisVariableName);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(ZAxisVariableName);
  vtkGetStringMacro(ZAxisVariableName);

  // Description:
  // Set/Get the underlying IO method the reader will employ, i.e., MPI or POSIX.
  vtkSetMacro(GenericIOType,int);
  vtkGetMacro(GenericIOType,int);

  // Description:
  // Set/Get the underlying block-assignment strategy to use, i.e., ROUND_ROBIN,
  // or RCB.
  vtkSetMacro(BlockAssignment,int);
  vtkGetMacro(BlockAssignment,int);

  // Description:
  // Set/Get the RankInQuery. Used in combination with SetQueryRankNeighbors(1)
  // tells the reader to render only the data of the RankInQuery and its
  // neighbors.
  vtkSetMacro(RankInQuery,int);
  vtkGetMacro(RankInQuery,int);

  // Description:
  // Set/Get whether the reader should read/render only the data of the
  // user-supplied rank, via SetRankInQuery(),
  vtkSetMacro(QueryRankNeighbors,int);
  vtkGetMacro(QueryRankNeighbors,int);

  // Description:
  // Returns the list of arrays used to select the variables to be used
  // for the x,y and z axis.
  vtkGetObjectMacro(ArrayList,vtkStringArray);

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection,vtkDataArraySelection);

  // Description:
  // Set/Get a multiprocess-controller for reading in parallel.
  // By default this parameter is set to NULL by the constructor.
  vtkSetMacro(Controller,vtkMultiProcessController*);
  vtkGetMacro(Controller,vtkMultiProcessController*);

  // Description:
  // Returns the number of arrays in the file, i.e., the number of columns.
  int GetNumberOfPointArrays();

  // Description:
  // Returns the name of the ith array.
  const char* GetPointArrayName(int i);

  // Description:
  // Returns the status of the array corresponding to the given name.
  int GetPointArrayStatus(const char* name);

  // Description:
  // Sets the status of the
  void SetPointArrayStatus(const char* name, int status);

protected:
  vtkPGenericIOReader();
  virtual ~vtkPGenericIOReader();

  // Pipeline methods
  virtual int RequestInformation(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int RequestData(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  // Description:
  // Loads the GenericIO metadata from the file.
  void LoadMetaData();

  // Description:
  // This method checks if the internal reader parameters have changed.
  // Namely, if the I/O method or filename have changed, the method returns
  // true.
  bool ReaderParametersChanged();

  // Description:
  // Returns the internal reader instance according to IOType.
  gio::GenericIOReader* GetInternalReader();


  // Description:
  // Return the point from the raw data.
  void GetPointFromRawData(
          std::string xName,
          std::string yName,
          std::string zName,
          vtkIdType idx,
          double pnt[3]);

  // Description:
  // Loads the variable with the given name
  void LoadRawVariableData(std::string varName);

  // Description:
  // Loads the Raw data
  void LoadRawData();

  // Description:
  // Loads the particle coordinates
  void LoadCoordinates(vtkUnstructuredGrid *grid);

  // Description:
  // Loads the particle data arrays
  void LoadData(vtkUnstructuredGrid *grid);

  // Description:
  // Finds the neighbors of the user-supplied rank
  void FindRankNeighbors();

  // Descriptions
  // Call-back registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject *caller,unsigned long eid,
    void *clientdata,void *calldata );

  char *XAxisVariableName;
  char *YAxisVariableName;
  char *ZAxisVariableName;

  char *FileName;
  int GenericIOType;
  int BlockAssignment;

  int QueryRankNeighbors;
  int RankInQuery;

  bool BuildMetaData;


  vtkMultiProcessController* Controller;

  vtkStringArray *ArrayList;
  vtkDataArraySelection* PointDataArraySelection;
  vtkCallbackCommand* SelectionObserver;

// BTX
  gio::GenericIOReader* Reader;
  vtkGenericIOMetaData* MetaData;
// ETX

  int RequestInfoCounter;
  int RequestDataCounter;
private:
  vtkPGenericIOReader(const vtkPGenericIOReader&); // Not implemented.
  void operator=(const vtkPGenericIOReader&); // Not implemented.
};

#endif /* VTKPGENERICIOREADER_H_ */
