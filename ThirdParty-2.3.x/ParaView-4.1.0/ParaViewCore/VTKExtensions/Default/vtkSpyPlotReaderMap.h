/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkSpyPlotReaderMap.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpyPlotReaderMap - Maps strings to vtkSpyPlotUniReaders
// .SECTION Description
// Extracted from vtkSpyPlotReader
//-----------------------------------------------------------------------------
//=============================================================================
#ifndef __vtkSpyPlotReaderMap_h
#define __vtkSpyPlotReaderMap_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkSystemIncludes.h"

#include <string>
#include <vector>
#include <map>

class vtkMultiProcessStream;
class vtkSpyPlotReader;
class vtkSpyPlotUniReader;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkSpyPlotReaderMap
{
public:
  typedef std::map<std::string, vtkSpyPlotUniReader*> MapOfStringToSPCTH;
  typedef std::vector<std::string> VectorOfStrings;
  MapOfStringToSPCTH Files;

  // Initialize the file-map. The filename can either be a case file or a spcth
  // file. In case of later, we detect fileseries automatically.
  // This method should ideally be called only on the 0th node to avoid reading
  // reads for meta-data on all the nodes.
  bool Initialize(const char *filename);

  void Clean(vtkSpyPlotUniReader* save);
  vtkSpyPlotUniReader* GetReader(MapOfStringToSPCTH::iterator& it, 
                                 vtkSpyPlotReader* parent);
  void TellReadersToCheck(vtkSpyPlotReader *parent);

  bool Save(vtkMultiProcessStream& stream);
  bool Load(vtkMultiProcessStream& stream);

private:
  // Description:
  // This does the updating of the meta data of the case file. Similar to
  // InitializeFromCaseFile, this method builds the vtkSpyPlotReaderMap using the
  // case file.
  bool InitializeFromSpyFile(const char*);
 
  // Description:
  // This does the updating of the meta data for a series, when no case file
  // provided. The main role of this method is to build the vtkSpyPlotReaderMap
  // based on the files.
  bool InitializeFromCaseFile(const char*);
};


#endif

// VTK-HeaderTest-Exclude: vtkSpyPlotReaderMap.h
