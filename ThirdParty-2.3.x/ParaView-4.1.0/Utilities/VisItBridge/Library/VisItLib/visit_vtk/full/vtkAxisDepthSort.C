/*****************************************************************************
*
* Copyright (c) 2000 - 2010, Lawrence Livermore National Security, LLC
* Produced at the Lawrence Livermore National Laboratory
* LLNL-CODE-400124
* All rights reserved.
*
* This file is  part of VisIt. For  details, see https://visit.llnl.gov/.  The
* full copyright notice is contained in the file COPYRIGHT located at the root
* of the VisIt distribution or at http://www.llnl.gov/visit/copyright.html.
*
* Redistribution  and  use  in  source  and  binary  forms,  with  or  without
* modification, are permitted provided that the following conditions are met:
*
*  - Redistributions of  source code must  retain the above  copyright notice,
*    this list of conditions and the disclaimer below.
*  - Redistributions in binary form must reproduce the above copyright notice,
*    this  list of  conditions  and  the  disclaimer (as noted below)  in  the
*    documentation and/or other materials provided with the distribution.
*  - Neither the name of  the LLNS/LLNL nor the names of  its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR  IMPLIED WARRANTIES, INCLUDING,  BUT NOT  LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR  PURPOSE
* ARE  DISCLAIMED. IN  NO EVENT  SHALL LAWRENCE  LIVERMORE NATIONAL  SECURITY,
* LLC, THE  U.S.  DEPARTMENT OF  ENERGY  OR  CONTRIBUTORS BE  LIABLE  FOR  ANY
* DIRECT,  INDIRECT,   INCIDENTAL,   SPECIAL,   EXEMPLARY,  OR   CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR
* SERVICES; LOSS OF  USE, DATA, OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER
* CAUSED  AND  ON  ANY  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT
* LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY  WAY
* OUT OF THE  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
* DAMAGE.
*
*****************************************************************************/

// ************************************************************************* //
//                            vtkAxisDepthSort.C                             //
// ************************************************************************* //

#include <vtkAxisDepthSort.h>

#include <stdlib.h>

#include <vtkCellData.h>
#include <vtkGenericCell.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkExecutive.h>

static int CoordSorter(const void *, const void *);

typedef struct 
{
    float    coord;
    int      cell_id;
} coord_cell_id_pair;

vtkStandardNewMacro(vtkAxisDepthSort);


// ****************************************************************************
//  Modifications:
//
//    Hank Childs, Fri Mar 11 08:26:34 PST 2005
//    Fix memory leak.
//
// ****************************************************************************

vtkAxisDepthSort::vtkAxisDepthSort()
{
    this->SetNumberOfOutputPorts(6);
}


vtkPolyData *
vtkAxisDepthSort::GetPlusXOutput(vtkInformationVector* outputVector)
{
    return this->GetOutput(0, outputVector);
}


vtkPolyData *
vtkAxisDepthSort::GetMinusXOutput(vtkInformationVector* outputVector)
{
    return this->GetOutput(1, outputVector);
}


vtkPolyData *
vtkAxisDepthSort::GetPlusYOutput(vtkInformationVector* outputVector)
{
    return this->GetOutput(2, outputVector);
}


vtkPolyData *
vtkAxisDepthSort::GetMinusYOutput(vtkInformationVector* outputVector)
{
    return this->GetOutput(3, outputVector);
}


vtkPolyData *
vtkAxisDepthSort::GetPlusZOutput(vtkInformationVector* outputVector)
{
    return this->GetOutput(4, outputVector);
}


vtkPolyData *
vtkAxisDepthSort::GetMinusZOutput(vtkInformationVector* outputVector)
{
    return this->GetOutput(5, outputVector);
}

vtkPolyData *
vtkAxisDepthSort::GetOutput(int port, vtkInformationVector* outputVector)
{
    vtkInformation *outInfo = outputVector->GetInformationObject(port);
    vtkPolyData *output = vtkPolyData::SafeDownCast(
            outInfo->Get(vtkDataObject::DATA_OBJECT()));
    return output;
}

int
vtkAxisDepthSort::RequestData( vtkInformation* vtkNotUsed(request),
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector)
{
    int   i;
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    
    vtkPolyData *input = vtkPolyData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    int ncells = input->GetNumberOfCells();

    coord_cell_id_pair *pairs = new coord_cell_id_pair[ncells];

    //
    // Create some automatic variables that we will use when generating the
    // outputs.
    //
    vtkPointData *inPD       = input->GetPointData();
    vtkCellData  *inCD       = input->GetCellData();
    int           doCellData = (inCD->GetNumberOfArrays() > 0 ? true : false);
    vtkPointData *outPD      = NULL;
    vtkCellData  *outCD      = NULL;
    vtkIdType     npts       = 0;
    vtkIdType    *cellPts    = 0;
    input->BuildCells();
 
    //
    // Start off by getting the center of the cell for each cell.
    //
    float *loc = new float[ncells*3];
    double   *bnds = NULL;
    vtkGenericCell *cell = vtkGenericCell::New();
    for (i = 0 ; i < ncells ; i++)
    {
        input->GetCell(i, cell);
        bnds = cell->GetBounds();
        loc[3*i]   = (bnds[0]+bnds[1])/2.;
        loc[3*i+1] = (bnds[2]+bnds[3])/2.;
        loc[3*i+2] = (bnds[4]+bnds[5])/2.;
    }
    cell->Delete();

    //
    // Create an array to sort in X and use qsort to sort it.
    //
    for (i = 0 ; i < ncells ; i++)
    {
        pairs[i].coord   = loc[3*i];
        pairs[i].cell_id = i;
    }
    qsort(pairs, ncells, sizeof(coord_cell_id_pair), CoordSorter);

    //
    // Now create the PlusX/MinusX output.
    //
    vtkPolyData *minusX = GetMinusXOutput(outputVector);
    minusX->SetPoints(input->GetPoints());
    minusX->Allocate(input);
    outPD = minusX->GetPointData();
    outCD = minusX->GetCellData();
    outPD->PassData(inPD);
    if (doCellData)
        outCD->CopyAllocate(inCD);
    for (i = 0 ; i < ncells ; i++)
    {
        int cell = pairs[i].cell_id;
        input->GetCellPoints(cell, npts, cellPts);
        minusX->InsertNextCell(input->GetCellType(cell), npts, cellPts);
        if (doCellData)
            outCD->CopyData(inCD, i, cell);
    }

    vtkPolyData *plusX = GetPlusXOutput(outputVector);
    plusX->SetPoints(input->GetPoints());
    plusX->Allocate(input);
    outPD = plusX->GetPointData();
    outCD = plusX->GetCellData();
    outPD->PassData(inPD);
    if (doCellData)
        outCD->CopyAllocate(inCD);
    for (i = ncells-1 ; i >= 0 ; i--)
    {
        int cell = pairs[i].cell_id;
        input->GetCellPoints(cell, npts, cellPts);
        plusX->InsertNextCell(input->GetCellType(cell), npts, cellPts);
        if (doCellData)
            outCD->CopyData(inCD, i, cell);
    }

    //
    // Create an array to sort in Y and use qsort to sort it.
    //
    for (i = 0 ; i < ncells ; i++)
    {
        pairs[i].coord   = loc[3*i+1];
        pairs[i].cell_id = i;
    }
    qsort(pairs, ncells, sizeof(coord_cell_id_pair), CoordSorter);

    //
    // Now create the PlusY/MinusY output.
    //
    vtkPolyData *minusY = GetMinusYOutput(outputVector);
    minusY->SetPoints(input->GetPoints());
    minusY->Allocate(input);
    outPD = minusY->GetPointData();
    outCD = minusY->GetCellData();
    outPD->PassData(inPD);
    if (doCellData)
        outCD->CopyAllocate(inCD);
    for (i = 0 ; i < ncells ; i++)
    {
        int cell = pairs[i].cell_id;
        input->GetCellPoints(cell, npts, cellPts);
        minusY->InsertNextCell(input->GetCellType(cell), npts, cellPts);
        if (doCellData)
            outCD->CopyData(inCD, i, cell);
    }

    vtkPolyData *plusY = GetPlusYOutput(outputVector);
    plusY->SetPoints(input->GetPoints());
    plusY->Allocate(input);
    outPD = plusY->GetPointData();
    outCD = plusY->GetCellData();
    outPD->PassData(inPD);
    if (doCellData)
        outCD->CopyAllocate(inCD);
    for (i = ncells-1 ; i >= 0 ; i--)
    {
        int cell = pairs[i].cell_id;
        input->GetCellPoints(cell, npts, cellPts);
        plusY->InsertNextCell(input->GetCellType(cell), npts, cellPts);
        if (doCellData)
            outCD->CopyData(inCD, i, cell);
    }

    //
    // Create an array to sort in Z and use qsort to sort it.
    //
    for (i = 0 ; i < ncells ; i++)
    {
        pairs[i].coord   = loc[3*i+2];
        pairs[i].cell_id = i;
    }
    qsort(pairs, ncells, sizeof(coord_cell_id_pair), CoordSorter);

    //
    // Now create the PlusZ/MinusZ output.
    //
    vtkPolyData *minusZ = GetMinusZOutput(outputVector);
    minusZ->SetPoints(input->GetPoints());
    minusZ->Allocate(input);
    outPD = minusZ->GetPointData();
    outCD = minusZ->GetCellData();
    outPD->PassData(inPD);
    if (doCellData)
        outCD->CopyAllocate(inCD);
    for (i = 0 ; i < ncells ; i++)
    {
        int cell = pairs[i].cell_id;
        input->GetCellPoints(cell, npts, cellPts);
        minusZ->InsertNextCell(input->GetCellType(cell), npts, cellPts);
        if (doCellData)
            outCD->CopyData(inCD, i, cell);
    }

    vtkPolyData *plusZ = GetPlusZOutput(outputVector);
    plusZ->SetPoints(input->GetPoints());
    plusZ->Allocate(input);
    outPD = plusZ->GetPointData();
    outCD = plusZ->GetCellData();
    outPD->PassData(inPD);
    if (doCellData)
        outCD->CopyAllocate(inCD);
    for (i = ncells-1 ; i >= 0 ; i--)
    {
        int cell = pairs[i].cell_id;
        input->GetCellPoints(cell, npts, cellPts);
        plusZ->InsertNextCell(input->GetCellType(cell), npts, cellPts);
        if (doCellData)
            outCD->CopyData(inCD, i, cell);
    }

    //
    // Clean up memory.
    //
    delete [] loc;
    delete [] pairs;
    return 1;
}


int
CoordSorter(const void *arg1, const void *arg2)
{
    const coord_cell_id_pair *c1 = (const coord_cell_id_pair *) arg1;
    const coord_cell_id_pair *c2 = (const coord_cell_id_pair *) arg2;

    if (c1[0].coord > c2[0].coord)
    {
        return 1;
    }
    else if (c1[0].coord < c2[0].coord)
    {
        return -1;
    }

    return 0;
}


