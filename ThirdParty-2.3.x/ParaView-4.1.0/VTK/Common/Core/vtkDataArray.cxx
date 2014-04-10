/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArray.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkDataArrayIteratorMacro.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"
#include "vtkLookupTable.h"
#include "vtkLongArray.h"
#include "vtkMath.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkTypedDataArrayIterator.h"
#include "vtkTypeTraits.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <algorithm> // for min(), max()

vtkInformationKeyRestrictedMacro(vtkDataArray, COMPONENT_RANGE, DoubleVector, 2);
vtkInformationKeyRestrictedMacro(vtkDataArray, L2_NORM_RANGE, DoubleVector, 2);


//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkDataArray::vtkDataArray()
{
  this->LookupTable = NULL;
  this->Range[0] = 0;
  this->Range[1] = 0;
}

//----------------------------------------------------------------------------
vtkDataArray::~vtkDataArray()
{
  if ( this->LookupTable )
    {
    this->LookupTable->Delete();
    }
  this->SetName(0);
}

//----------------------------------------------------------------------------
template <class InputIterator>
void vtkDeepCopySwitchOnOutput(InputIterator begin, InputIterator end,
                               vtkDataArray *outputArray)
{
  switch (outputArray->GetDataType())
    {
    vtkDataArrayIteratorMacro(outputArray,
                              std::copy(begin, end, vtkDABegin));

    default:
      vtkGenericWarningMacro("Unsupported data type "
                             << outputArray->GetDataTypeAsString() << "!");
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::DeepCopy(vtkAbstractArray* aa)
{
  if ( aa == NULL )
    {
    return;
    }

  vtkDataArray *da = vtkDataArray::FastDownCast(aa);
  if (da == NULL)
    {
    vtkErrorMacro(<< "Input array is not a vtkDataArray ("
                  << aa->GetClassName() << ")");
    return;
    }

  this->DeepCopy(da);
}

//----------------------------------------------------------------------------
//Normally subclasses will do this when the input and output type of the
//DeepCopy are the same. When they are not the same, then we use the
//templated code below.
void vtkDataArray::DeepCopy(vtkDataArray *da)
{
  // Match the behavior of the old AttributeData
  if ( da == NULL )
    {
    return;
    }

  if ( this != da )
    {
    this->Superclass::DeepCopy( da ); // copy Information object

    vtkIdType numTuples = da->GetNumberOfTuples();
    this->NumberOfComponents = da->NumberOfComponents;
    this->SetNumberOfTuples(numTuples);

    if (numTuples > 0)
      {
      switch (da->GetDataType())
        {
        vtkDataArrayIteratorMacro(
          da, vtkDeepCopySwitchOnOutput(vtkDABegin, vtkDAEnd, this)
          );

        case VTK_BIT:
          {//bit not supported, using generic double API
          for (vtkIdType i=0; i < numTuples; i++)
            {
            this->SetTuple(i, da->GetTuple(i));
            }
          break;
          }

        default:
          vtkErrorMacro("Unsupported data type " << da->GetDataType() << "!");
        }
      }

    this->SetLookupTable(0);
    if (da->LookupTable)
      {
      this->LookupTable = da->LookupTable->NewInstance();
      this->LookupTable->DeepCopy(da->LookupTable);
      }
    }

  this->Squeeze();
}

//----------------------------------------------------------------------------
// These can be overridden for more efficiency
double vtkDataArray::GetComponent(vtkIdType i, int j)
{
  double *tuple=new double[this->NumberOfComponents], c;

  this->GetTuple(i,tuple);
  c =  tuple[j];
  delete [] tuple;

  return c;
}

//----------------------------------------------------------------------------
void vtkDataArray::SetComponent(vtkIdType i, int j, double c)
{
  double *tuple=new double[this->NumberOfComponents];

  if ( i < this->GetNumberOfTuples() )
    {
    this->GetTuple(i,tuple);
    }
  else
    {
    for (int k=0; k<this->NumberOfComponents; k++)
      {
      tuple[k] = 0.0;
      }
    }

  tuple[j] = c;
  this->SetTuple(i,tuple);

  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertComponent(vtkIdType i, int j, double c)
{
  double *tuple=new double[this->NumberOfComponents];

  if ( i < this->GetNumberOfTuples() )
    {
    this->GetTuple(i,tuple);
    }
  else
    {
    for (int k=0; k<this->NumberOfComponents; k++)
      {
      tuple[k] = 0.0;
      }
    }

  tuple[j] = c;
  this->InsertTuple(i,tuple);

  delete [] tuple;
}

//----------------------------------------------------------------------------
void vtkDataArray::GetData(vtkIdType tupleMin, vtkIdType tupleMax, int compMin,
                           int compMax, vtkDoubleArray* data)
{
  int i;
  vtkIdType j;
  int numComp=this->GetNumberOfComponents();
  double *tuple=new double[numComp];
  double *ptr=data->WritePointer(0,(tupleMax-tupleMin+1)*(compMax-compMin+1));

  for (j=tupleMin; j <= tupleMax; j++)
    {
    this->GetTuple(j,tuple);
    for (i=compMin; i <= compMax; i++)
      {
      *ptr++ = tuple[i];
      }
    }
  delete [] tuple;
}

//--------------------------------------------------------------------------
template <class T>
inline void vtkDataArrayRoundIfNecessary(double val, T* retVal)
{
  val = std::max(val, static_cast<double>(vtkTypeTraits<T>::Min()));
  val = std::min(val, static_cast<double>(vtkTypeTraits<T>::Max()));
  *retVal = static_cast<T>((val>=0.0)?(val + 0.5):(val - 0.5));
}

//--------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
inline void vtkDataArrayRoundIfNecessary(double val, double* retVal)
{
  *retVal = val;
}

//--------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
inline void vtkDataArrayRoundIfNecessary(double val, float* retVal)
{
  *retVal = static_cast<float>(val);
}

//--------------------------------------------------------------------------
template <class Scalar, class Iterator>
void vtkDataArrayInterpolateTuple(Iterator from, Scalar *to, int numComp,
  vtkIdType* ids, vtkIdType numIds, double* weights)
{
  for(int i=0; i < numComp; ++i)
    {
    double c = 0;
    for(vtkIdType j=0; j < numIds; ++j)
      {
      c += weights[j] * static_cast<double>(from[ids[j]*numComp+i]);
      }
    // Round integer types. Don't round floating point types.
    vtkDataArrayRoundIfNecessary(c, to);
    ++to;
    }
}

//----------------------------------------------------------------------------
// Interpolate array value from other array value given the
// indices and associated interpolation weights.
// This method assumes that the two arrays are of the same time.
void vtkDataArray::InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
  vtkAbstractArray* source,  double* weights)
{
  if (this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Cannot InterpolateValue from array of type "
      << source->GetDataTypeAsString());
    return;
    }

  vtkDataArray* fromData = vtkDataArray::FastDownCast(source);
  if (fromData)
    {
    int numComp = fromData->GetNumberOfComponents();
    vtkIdType j, numIds=ptIndices->GetNumberOfIds();
    vtkIdType *ids=ptIndices->GetPointer(0);
    vtkIdType idx= i*numComp;
    double c;

    // Note that we must call WriteVoidPointer before GetVoidPointer
    // in case WriteVoidPointer reallocates memory and fromData ==
    // this. The vtkBitArray implementation doesn't use pointers, so skip
    // the resizing in this case.
    void* vto = fromData->GetDataType() != VTK_BIT ?
          this->WriteVoidPointer(idx, numComp) : 0;

    switch (fromData->GetDataType())
      {
    case VTK_BIT:
        {
        vtkBitArray *from=static_cast<vtkBitArray *>(fromData);
        vtkBitArray *to=static_cast<vtkBitArray *>(this);
        for (int k=0; k<numComp; k++)
          {
          for (c=0, j=0; j<numIds; j++)
            {
            c += weights[j]*from->GetValue(ids[j]*numComp+k);
            }
          to->InsertValue(idx+k, static_cast<int>(c));
          }
        }
      break;
        vtkDataArrayIteratorMacro(fromData,
          vtkDataArrayInterpolateTuple(vtkDABegin,
                                       static_cast<vtkDAValueType*>(vto),
                                       numComp, ids, numIds, weights)
      );
    default:
      vtkErrorMacro("Unsupported data type " << fromData->GetDataType()
        << " during interpolation!");
      }
    }
}


//----------------------------------------------------------------------------
template <class Scalar, class Iterator>
void vtkDataArrayInterpolateTuple(Iterator from1, Iterator from2, Scalar* to,
  int numComp, double t)
{
  const double oneMinusT = 1.0 - t;
  while (numComp-- > 0)
    {
    *(to++) = oneMinusT * (*(from1++)) + t * (*(from2++));
    }
}

//----------------------------------------------------------------------------
// Interpolate value from the two values, p1 and p2, and an
// interpolation factor, t. The interpolation factor ranges from (0,1),
// with t=0 located at p1. This method assumes that the three arrays are of
// the same type. p1 is value at index id1 in fromArray1, while, p2 is
// value at index id2 in fromArray2.
void vtkDataArray::InterpolateTuple(vtkIdType i,
  vtkIdType id1, vtkAbstractArray* source1,
  vtkIdType id2, vtkAbstractArray* source2, double t)
{
  int type = this->GetDataType();

  if (type != source1->GetDataType() || type != source2->GetDataType())
    {
    vtkErrorMacro("All arrays to InterpolateValue must be of same type.");
    return;
    }

  int k, numComp = source1->GetNumberOfComponents();
  double c;
  vtkIdType loc = i * numComp;

  switch (type)
    {
    case VTK_BIT:
      {
      vtkBitArray *from1 = static_cast<vtkBitArray *>(source1);
      vtkBitArray *from2 = static_cast<vtkBitArray *>(source2);
      vtkBitArray *to = static_cast<vtkBitArray *>(this);
      for (k=0; k<numComp; k++)
        {
        c = from1->GetValue(id1) + t * (from2->GetValue(id2) - from1->GetValue(id1));
        to->InsertValue(loc + k, static_cast<int>(c));
        }
      }
      break;
      // Note that we must call WriteVoidPointer before GetVoidPointer/creating
      // iterators in case WriteVoidPointer reallocates memory and
      // fromData1==this or fromData2==this.
    vtkTemplateMacro(
      // If either of the source arrays are mapped, use iterators. Otherwise,
      // void pointers are safe.
      if (source1->HasStandardMemoryLayout() &&
          source2->HasStandardMemoryLayout())
        {
        // Use pointers:
        void *vto = this->WriteVoidPointer(loc, numComp);
        void *vfrom1 = source1->GetVoidPointer(id1 * numComp);
        void *vfrom2 = source2->GetVoidPointer(id2 * numComp);
        vtkDataArrayInterpolateTuple<VTK_TT>(static_cast<VTK_TT*>(vfrom1),
                                             static_cast<VTK_TT*>(vfrom2),
                                             static_cast<VTK_TT*>(vto),
                                             numComp, t);
        }
      else
        {
        vtkTypedDataArray<VTK_TT> *tfrom1 =
            vtkTypedDataArray<VTK_TT>::FastDownCast(source1);
        vtkTypedDataArray<VTK_TT> *tfrom2 =
            vtkTypedDataArray<VTK_TT>::FastDownCast(source2);
        if (!tfrom1 || !tfrom2)
          {
          vtkErrorMacro(<<"Cannot call this function with non-standard arrays "
                        "unless all arrays are vtkTypedDataArray subclasses.");
          return;
          }
        VTK_TT *vto = static_cast<VTK_TT*>(
            this->WriteVoidPointer(loc, numComp));
        vtkDataArrayInterpolateTuple<VTK_TT>(
            vtkTypedDataArrayIterator<VTK_TT>(tfrom1, id1 * numComp),
            vtkTypedDataArrayIterator<VTK_TT>(tfrom2, id2 * numComp),
            vto, numComp, t);
        }
      );
    default:
      vtkErrorMacro("Unsupported data type " << type
                    << " during interpolation!");
    }

}

//----------------------------------------------------------------------------
void vtkDataArray::CreateDefaultLookupTable()
{
  if ( this->LookupTable )
    {
    this->LookupTable->UnRegister(this);
    }
  this->LookupTable = vtkLookupTable::New();
  // make sure it is built
  // otherwise problems with InsertScalar trying to map through
  // non built lut
  this->LookupTable->Build();
}

//----------------------------------------------------------------------------
void vtkDataArray::SetLookupTable(vtkLookupTable* lut)
{
  if ( this->LookupTable != lut )
    {
    if ( this->LookupTable )
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    if ( this->LookupTable )
      {
      this->LookupTable->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
double* vtkDataArray::GetTupleN(vtkIdType i, int n)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != n)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != " << n);
    }
  return this->GetTuple(i);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetTuple1(vtkIdType i)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  return *(this->GetTuple(i));
}

//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple2(vtkIdType i)
{
  return this->GetTupleN(i, 2);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple3(vtkIdType i)
{
  return this->GetTupleN(i, 3);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple4(vtkIdType i)
{
  return this->GetTupleN(i, 4);
}
//----------------------------------------------------------------------------
double* vtkDataArray::GetTuple9(vtkIdType i)
{
  return this->GetTupleN(i, 9);
}

//----------------------------------------------------------------------------
void vtkDataArray::SetTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  this->SetTuple(i, &value);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple3(vtkIdType i, double val0, double val1,
                             double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple4(vtkIdType i, double val0, double val1,
                             double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->SetTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::SetTuple9(vtkIdType i, double val0, double val1,
                             double val2, double val3, double val4,
                             double val5, double val6, double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->SetTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple1(vtkIdType i, double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  this->InsertTuple(i, &value);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple2(vtkIdType i, double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple3(vtkIdType i, double val0, double val1,
                                double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple4(vtkIdType i, double val0, double val1,
                                double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertTuple(i, tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertTuple9(vtkIdType i, double val0, double val1,
                                double val2,  double val3, double val4,
                                double val5, double val6,double val7, double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->InsertTuple(i, tuple);
}

//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple1(double value)
{
  int numComp = this->GetNumberOfComponents();
  if (numComp != 1)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 1");
    }
  this->InsertNextTuple(&value);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple2(double val0, double val1)
{
  double tuple[2];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 2)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 2");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple3(double val0, double val1,
                                    double val2)
{
  double tuple[3];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 3)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 3");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple4(double val0, double val1,
                                    double val2, double val3)
{
  double tuple[4];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 4)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 4");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  this->InsertNextTuple(tuple);
}
//----------------------------------------------------------------------------
void vtkDataArray::InsertNextTuple9(double val0, double val1,
                                    double val2,  double val3, double val4,
                                    double val5, double val6,double val7,
                                    double val8)
{
  double tuple[9];
  int numComp = this->GetNumberOfComponents();
  if (numComp != 9)
    {
    vtkErrorMacro("The number of components do not match the number requested: "
                  << numComp << " != 9");
    }
  tuple[0] = val0;
  tuple[1] = val1;
  tuple[2] = val2;
  tuple[3] = val3;
  tuple[4] = val4;
  tuple[5] = val5;
  tuple[6] = val6;
  tuple[7] = val7;
  tuple[8] = val8;
  this->InsertNextTuple(tuple);
}

//----------------------------------------------------------------------------
unsigned long vtkDataArray::GetActualMemorySize()
{
  vtkIdType numPrims;
  double size;
  // The allocated array may be larger than the number of primitives used.
  //numPrims = this->GetNumberOfTuples() * this->GetNumberOfComponents();
  numPrims = this->GetSize();

  size = vtkDataArray::GetDataTypeSize(this->GetDataType());

  // kilobytes
  return static_cast<unsigned long>(ceil((size*static_cast<double>(numPrims)
                                           )/1024.0));
}

//----------------------------------------------------------------------------
vtkDataArray* vtkDataArray::CreateDataArray(int dataType)
{
  vtkAbstractArray* aa = vtkAbstractArray::CreateArray(dataType);
  vtkDataArray* da = vtkDataArray::SafeDownCast(aa);
  if (!da && aa)
    {
    // Requested array is not a vtkDataArray. Delete the allocated array.
    aa->Delete();
    }
  return da;
}

namespace {
//----------------------------------------------------------------------------
typedef vtkIdType* IdIterator;

//----------------------------------------------------------------------------
template <class InputIterator, class OutputIterator>
void vtkDataArrayGetTuplesTemplate2(IdIterator ids, IdIterator idsEnd,
                                    InputIterator inIter,
                                    OutputIterator outIter,
                                    int numComps)
{
  InputIterator inPt;
  while (ids != idsEnd)
    {
    inPt = inIter + (*(ids++) * numComps);
    outIter = std::copy(inPt, inPt + numComps, outIter);
    }
}

//----------------------------------------------------------------------------
template <class InputIterator>
void vtkDataArrayGetTuplesTemplate1(IdIterator ids, IdIterator idsEnd,
                                    InputIterator inIter,
                                    vtkDataArray *outArray,
                                    int numComps)
{
  switch (outArray->GetDataType())
    {
    vtkDataArrayIteratorMacro(outArray,
      vtkDataArrayGetTuplesTemplate2(ids, idsEnd, inIter, vtkDABegin, numComps)
      );
    default:
      vtkGenericWarningMacro("vtkDataArray::GetTuples: "
                             "Unsupported output type.");
      return;
    }
}

} // end anon namespace


//----------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdList *ptIds, vtkAbstractArray *aa)
{
  vtkDataArray *outArray = vtkDataArray::FastDownCast(aa);
  if (!outArray)
    {
    vtkWarningMacro("Input is not a vtkDataArray.");
    return;
    }

  if ((outArray->GetNumberOfComponents() != this->GetNumberOfComponents()))
    {
    vtkWarningMacro("Number of components for input and output do not match");
    return;
    }

  IdIterator ids = ptIds->GetPointer(0);
  IdIterator idsEnd = ptIds->GetPointer(ptIds->GetNumberOfIds());

  switch (this->GetDataType())
    {
    vtkDataArrayIteratorMacro(this,
      vtkDataArrayGetTuplesTemplate1(ids, idsEnd, vtkDABegin, outArray,
                                     this->NumberOfComponents)
      );
    default: // Fallback to the double interface
      vtkIdType num=ptIds->GetNumberOfIds();
      for (vtkIdType i=0; i<num; i++)
        {
        outArray->SetTuple(i, this->GetTuple(ptIds->GetId(i)));
        }
      break;
    }
}

//----------------------------------------------------------------------------
template <class IT, class OT>
void vtkCopyTuples(IT* input, OT* output, int nComp,
                   vtkIdType p1, vtkIdType p2)
{
  vtkIdType i;
  int j;
  vtkIdType num=p2-p1+1;
  for (i=0; i<num; i++)
    {
    for (j=0; j<nComp; j++)
      {
      output[i*nComp+j] = static_cast<OT>(input[(p1+i)*nComp+j]);
      }
    }
}

//----------------------------------------------------------------------------
template <class IT>
void vtkCopyTuples1(IT* input, vtkDataArray* output,
                    vtkIdType p1, vtkIdType p2)
{
  switch (output->GetDataType())
    {
    vtkTemplateMacro(vtkCopyTuples( input,
                                    static_cast<VTK_TT *>(output->GetVoidPointer(0)),
                                    output->GetNumberOfComponents(), p1, p2) );

    default:
      vtkGenericWarningMacro("Sanity check failed: Unsupported data type "
                             << output->GetDataType() << ".");
      return;
    }
}


//----------------------------------------------------------------------------
void vtkDataArray::GetTuples(vtkIdType p1, vtkIdType p2, vtkAbstractArray *aa)
{
  vtkDataArray* da = vtkDataArray::SafeDownCast(aa);
  if (!da)
    {
    vtkWarningMacro("Input is not a vtkDataArray.");
    return;
    }

  if ((da->GetNumberOfComponents() != this->GetNumberOfComponents()))
    {
    vtkWarningMacro("Number of components for input and output do not match");
    return;
    }

  switch (this->GetDataType())
    {
    vtkTemplateMacro(vtkCopyTuples1( static_cast<VTK_TT *>(this->GetVoidPointer(0)), da,
                                     p1, p2 ) );
    // This is not supported by the template macro.
    // Switch to using the double interface.
    case VTK_BIT:
      {
      vtkIdType num=p2-p1+1;
      for (vtkIdType i=0; i<num; i++)
        {
        da->SetTuple(i,this->GetTuple(p1+i));
        }
      }
      break;
    default:
      vtkErrorMacro("Sanity check failed: Unsupported data type "
                    << this->GetDataType() << ".");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::FillComponent(int j, double c)
{
  if (j < 0 || j >= this->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Specified component " << j << " is not in [0, "
    << this->GetNumberOfComponents() << ")" );
    return;
    }

  vtkIdType i;

  for (i = 0; i < this->GetNumberOfTuples(); i++)
    {
    this->SetComponent(i, j, c);
    }
}


//----------------------------------------------------------------------------
void vtkDataArray::CopyComponent(int j, vtkDataArray *from,
                                 int fromComponent)
{
  if (this->GetNumberOfTuples() != from->GetNumberOfTuples())
    {
    vtkErrorMacro(<< "Number of tuples in 'from' ("
    << from->GetNumberOfTuples() << ") and 'to' ("
    << this->GetNumberOfTuples() << ") do not match.");
    return;
    }

  if (j < 0 || j >= this->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Specified component " << j << " in 'to' array is not in [0, "
    << this->GetNumberOfComponents() << ")" );
    return;
    }

  if (fromComponent < 0 || fromComponent >= from->GetNumberOfComponents())
    {
    vtkErrorMacro(<< "Specified component " << fromComponent << " in 'from' array is not in [0, "
    << from->GetNumberOfComponents() << ")" );
    return;
    }

  vtkIdType i;
  for (i = 0; i < this->GetNumberOfTuples(); i++)
    {
    this->SetComponent(i, j, from->GetComponent(i, fromComponent));
    }
}

//----------------------------------------------------------------------------
double vtkDataArray::GetMaxNorm()
{
  vtkIdType i;
  double norm, maxNorm;
  int nComponents = this->GetNumberOfComponents();

  maxNorm = 0.0;
  for (i=0; i<this->GetNumberOfTuples(); i++)
    {
    norm = vtkMath::Norm(this->GetTuple(i), nComponents);
    if ( norm > maxNorm )
      {
      maxNorm = norm;
      }
    }

  return maxNorm;
}

//----------------------------------------------------------------------------
int vtkDataArray::CopyInformation(vtkInformation* infoFrom, int deep)
{
  // Copy everything + give base classes a chance to
  // Exclude keys which they don't want copied.
  this->Superclass::CopyInformation(infoFrom,deep);

  // Remove any keys we own that are not to be copied here.
  vtkInformation *myInfo=this->GetInformation();
  // Range:
  if (myInfo->Has( L2_NORM_RANGE() ))
    {
    myInfo->Remove( L2_NORM_RANGE() );
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkDataArray::ComputeRange(double range[2], int comp)
{
  if ( comp >= this->NumberOfComponents )
    { // Ignore requests for nonexistent components.
    return;
    }

  // If we got component -1 on a vector array, compute vector magnitude.
  if (comp < 0 && this->NumberOfComponents == 1)
    {
    comp = 0;
    }

  vtkInformation* info = this->GetInformation();
  vtkInformationDoubleVectorKey* rkey;
  if ( comp < 0 )
    {
    rkey = L2_NORM_RANGE();
    }
  else
    {
    vtkInformationVector* infoVec;
    if ( ! info->Has( PER_COMPONENT() ) )
      {
      infoVec = vtkInformationVector::New();
      info->Set( PER_COMPONENT(), infoVec );
      infoVec->FastDelete();
      }
    else
      {
      infoVec = info->Get( PER_COMPONENT() );
      }
    int vlen = infoVec->GetNumberOfInformationObjects();
    if ( vlen < this->NumberOfComponents )
      {
      infoVec->SetNumberOfInformationObjects( this->NumberOfComponents );
      double rtmp[2];
      rtmp[0] = VTK_DOUBLE_MAX;
      rtmp[1] = VTK_DOUBLE_MIN;
      // Since the MTime() of these new keys will be newer than this->MTime(), we must
      // be sure that their ranges are marked "invalid" so that we know they must be
      // computed.
      for ( int i = vlen; i < this->NumberOfComponents; ++i )
        {
        infoVec->GetInformationObject( i )->Set( COMPONENT_RANGE(), rtmp, 2 );
        }
      }
    info = infoVec->GetInformationObject( comp );
    rkey = COMPONENT_RANGE();
    }

  if ( info->Has( rkey ) )
    {
    if ( this->GetMTime() <= info->GetMTime() )
      {
      info->Get( rkey, range );
      if ( range[0] != VTK_DOUBLE_MAX && range[1] != VTK_DOUBLE_MIN )
        {
        // Only accept these values if they are reasonable. Otherwise, it is an
        // indication that they've never been computed before.
        return;
        }
      }
    }

  range[0] =  VTK_DOUBLE_MAX;
  range[1] =  VTK_DOUBLE_MIN;
  if ( comp < 0 )
    {
    this->ComputeVectorRange(range);
    }
  else
    {
    this->ComputeScalarRange(range, comp);
    }

  info->Set( rkey, range, 2 );
}

//----------------------------------------------------------------------------
void vtkDataArray::ComputeScalarRange(double range[2], int comp)
{
  vtkIdType numTuples=this->GetNumberOfTuples();
  for (vtkIdType i=0; i<numTuples; i++)
    {
    double s = this->GetComponent(i,comp);
    if ( s < range[0] )
      {
      range[0] = s;
      }
    if ( s > range[1] )
      {
      range[1] = s;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkDataArray::ComputeVectorRange(double range[2])
{
  vtkIdType numTuples=this->GetNumberOfTuples();
  for (vtkIdType i=0; i<numTuples; i++)
    {
    // Compute range of vector magnitude.
    double s = 0.0;
    for (int j=0; j < this->NumberOfComponents; ++j)
      {
      double t = this->GetComponent(i,j);
      s += t*t;
      }
    s = sqrt(s);
    if ( s < range[0] )
      {
      range[0] = s;
      }
    if ( s > range[1] )
      {
      range[1] = s;
      }
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(double range[2])
{
  vtkDataArray::GetDataTypeRange(this->GetDataType(), range);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin()
{
  return vtkDataArray::GetDataTypeMin(this->GetDataType());
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax()
{
  return vtkDataArray::GetDataTypeMax(this->GetDataType());
}

//----------------------------------------------------------------------------
void vtkDataArray::GetDataTypeRange(int type, double range[2])
{
  range[0] = vtkDataArray::GetDataTypeMin(type);
  range[1] = vtkDataArray::GetDataTypeMax(type);
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMin(int type)
{
  switch (type)
    {
    case VTK_BIT:                return static_cast<double>(VTK_BIT_MIN);
    case VTK_SIGNED_CHAR:        return static_cast<double>(VTK_SIGNED_CHAR_MIN);
    case VTK_UNSIGNED_CHAR:      return static_cast<double>(VTK_UNSIGNED_CHAR_MIN);
    case VTK_CHAR:               return static_cast<double>(VTK_CHAR_MIN);
    case VTK_UNSIGNED_SHORT:     return static_cast<double>(VTK_UNSIGNED_SHORT_MIN);
    case VTK_SHORT:              return static_cast<double>(VTK_SHORT_MIN);
    case VTK_UNSIGNED_INT:       return static_cast<double>(VTK_UNSIGNED_INT_MIN);
    case VTK_INT:                return static_cast<double>(VTK_INT_MIN);
    case VTK_UNSIGNED_LONG:      return static_cast<double>(VTK_UNSIGNED_LONG_MIN);
    case VTK_LONG:               return static_cast<double>(VTK_LONG_MIN);
#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_UNSIGNED_LONG_LONG: return static_cast<double>(VTK_UNSIGNED_LONG_LONG_MIN);
    case VTK_LONG_LONG:          return static_cast<double>(VTK_LONG_LONG_MIN);
#endif
#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:            return static_cast<double>(VTK___INT64_MIN);
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:   return static_cast<double>(VTK_UNSIGNED___INT64_MIN);
# endif
#endif
    case VTK_FLOAT:              return static_cast<double>(VTK_FLOAT_MIN);
    case VTK_DOUBLE:             return static_cast<double>(VTK_DOUBLE_MIN);
    case VTK_ID_TYPE:            return static_cast<double>(VTK_ID_MIN);
    default: return 0;
    }
}

//----------------------------------------------------------------------------
double vtkDataArray::GetDataTypeMax(int type)
{
  switch (type)
    {
    case VTK_BIT:                return static_cast<double>(VTK_BIT_MAX);
    case VTK_SIGNED_CHAR:        return static_cast<double>(VTK_SIGNED_CHAR_MAX);
    case VTK_UNSIGNED_CHAR:      return static_cast<double>(VTK_UNSIGNED_CHAR_MAX);
    case VTK_CHAR:               return static_cast<double>(VTK_CHAR_MAX);
    case VTK_UNSIGNED_SHORT:     return static_cast<double>(VTK_UNSIGNED_SHORT_MAX);
    case VTK_SHORT:              return static_cast<double>(VTK_SHORT_MAX);
    case VTK_UNSIGNED_INT:       return static_cast<double>(VTK_UNSIGNED_INT_MAX);
    case VTK_INT:                return static_cast<double>(VTK_INT_MAX);
    case VTK_UNSIGNED_LONG:      return static_cast<double>(VTK_UNSIGNED_LONG_MAX);
    case VTK_LONG:               return static_cast<double>(VTK_LONG_MAX);
#if defined(VTK_TYPE_USE_LONG_LONG)
    case VTK_UNSIGNED_LONG_LONG: return static_cast<double>(VTK_UNSIGNED_LONG_LONG_MAX);
    case VTK_LONG_LONG:          return static_cast<double>(VTK_LONG_LONG_MAX);
#endif
#if defined(VTK_TYPE_USE___INT64)
    case VTK___INT64:            return static_cast<double>(VTK___INT64_MAX);
# if defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    case VTK_UNSIGNED___INT64:   return static_cast<double>(VTK_UNSIGNED___INT64_MAX);
# endif
#endif
    case VTK_FLOAT:              return static_cast<double>(VTK_FLOAT_MAX);
    case VTK_DOUBLE:             return static_cast<double>(VTK_DOUBLE_MAX);
    case VTK_ID_TYPE:            return static_cast<double>(VTK_ID_MAX);
    default: return 1;
    }
}

//----------------------------------------------------------------------------
void vtkDataArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  const char* name = this->GetName();
  if (name)
    {
    os << indent << "Name: " << name << "\n";
    }
  else
    {
    os << indent << "Name: (none)\n";
    }
  os << indent << "Number Of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Number Of Tuples: " << this->GetNumberOfTuples() << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }
}
