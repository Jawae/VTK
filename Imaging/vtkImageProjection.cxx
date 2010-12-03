/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProjection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageProjection.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkStandardNewMacro(vtkImageProjection);

//----------------------------------------------------------------------------
vtkImageProjection::vtkImageProjection()
{
  this->Operation = VTK_PROJECTION_AVERAGE;
  this->SliceDirection = 2;
  this->SliceRange[0] = VTK_INT_MIN;
  this->SliceRange[1] = VTK_INT_MAX;
  this->OutputScalarType = 0;
  this->MultiSliceOutput = 0;
}

//----------------------------------------------------------------------------
vtkImageProjection::~vtkImageProjection()
{
}

//----------------------------------------------------------------------------
void vtkImageProjection::ExecuteInformation(vtkImageData *input,
                                            vtkImageData *output)
{
  int extent[6];
  int range[2];
  double origin[3];
  double sliceSpacing;
  int dimIndex;
  int scalarType;

  // get the direction along which to sum slices
  dimIndex = this->GetSliceDirection();

  // clamp the range to the whole extent
  input->GetWholeExtent(extent);
  this->GetSliceRange(range);
  if (range[0] < extent[2*dimIndex])
    {
    range[0] = extent[2*dimIndex];
    }
  if (range[1] > extent[2*dimIndex+1])
    {
    range[1] = extent[2*dimIndex+1];
    }

  // this avoids confusion about whether origin is double or float
  origin[0] = input->GetOrigin()[0];
  origin[1] = input->GetOrigin()[1];
  origin[2] = input->GetOrigin()[2];

  // set new origin to be in the center of the stack of slices
  sliceSpacing = input->GetSpacing()[dimIndex];
  origin[dimIndex] = (origin[dimIndex] +
                      0.5*sliceSpacing*(range[0] + range[1]));

  if (this->GetMultiSliceOutput())
    {
    // output extent is input extent, decreased by the slice range
    extent[2*dimIndex] -= range[0];
    extent[2*dimIndex+1] -= range[1];
    }
  else
    {
    // set new extent to single-slice
    extent[2*dimIndex] = 0;
    extent[2*dimIndex+1] = 0;
    }

  // set the output scalar type
  scalarType = this->GetOutputScalarType();
  if (scalarType == 0)
    {
    scalarType = input->GetScalarType();
    }

  // set the output information
  output->SetWholeExtent(extent);
  output->SetSpacing(input->GetSpacing());
  output->SetOrigin(origin[0], origin[1], origin[2]);
  output->SetScalarType(scalarType);
  output->SetNumberOfScalarComponents(input->GetNumberOfScalarComponents());
}

//----------------------------------------------------------------------------
void vtkImageProjection::ComputeInputUpdateExtent(int inExt[6],
                                                  int outExt[6])
{
  int extent[6];
  int range[2];
  int dimIndex;

  // initialize intput extent to output extent
  inExt[0] = outExt[0];
  inExt[1] = outExt[1];
  inExt[2] = outExt[2];
  inExt[3] = outExt[3];
  inExt[4] = outExt[4];
  inExt[5] = outExt[5];

  // get the direction along which to sum slices
  dimIndex = this->GetSliceDirection();

  // clamp the range to the whole extent
  this->GetInput()->GetWholeExtent(extent);
  this->GetSliceRange(range);
  if (range[0] < extent[2*dimIndex])
    {
    range[0] = extent[2*dimIndex];
    }
  if (range[1] > extent[2*dimIndex+1])
    {
    range[1] = extent[2*dimIndex+1];
    }

  // input range is the output range plus the specified slice range
  inExt[2*dimIndex] += range[0];
  inExt[2*dimIndex+1] += range[1];
}

//----------------------------------------------------------------------------
inline int vtkProjectionRound(double x)
{
#if defined mips || defined sparc || defined __ppc__
  return (int)((unsigned int)(x + 2147483648.5) - 2147483648U);
#elif defined i386 || defined _M_IX86
  unsigned int hilo[2];
  *((double *)hilo) = x + 103079215104.5;  // (2**(52-16))*1.5
  return (int)((hilo[1]<<16)|(hilo[0]>>16));
#else
  return (int)(floor(x+0.5));
#endif
}

//----------------------------------------------------------------------------
// rounding functions for each type

inline void vtkProjectionRound(double val, signed char& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, char& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, unsigned char& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, short& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, unsigned short& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, int& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, unsigned int& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, long& rnd)
{
  rnd = vtkProjectionRound(val);
}

inline void vtkProjectionRound(double val, unsigned long& rnd)
{
  rnd = vtkProjectionRound(val);
}

#ifdef VTK_LONG_LONG_MIN
inline void vtkProjectionRound(double val, long long& rnd)
{
  rnd = (long long)(floor(val + 0.5));
}
#else /* VTK_LONG_LONG_MIN */
#ifdef VTK___INT64_MIN
inline void vtkProjectionRound(double val, __int64& rnd)
{
  rnd = (__int64)(floor(val + 0.5));
}
#endif /* VTK___INT64_MIN */
#endif /* VTK_LONG_LONG_MIN */

#ifdef VTK_UNSIGNED_LONG_LONG_MIN
inline void vtkProjectionRound(double val, unsigned long long& rnd)
{
  rnd = (unsigned long long)(floor(val + 0.5));
}
#else /* VTK_UNSIGNED_LONG_LONG_MIN */
#ifdef VTK_UNSIGNED__INT64_MIN
inline void vtkProjectionRound(double val, unsigned __int64& clamp)
{
  rnd = (unsigned __int64)(floor(rnd + 0.5));
}
#endif /* VTK_UNSIGNED__INT64_MIN */
#endif /* VTK_UNSIGNED_LONG_LONG_MIN */

inline void vtkProjectionRound(double val, float& rnd)
{
  rnd = val;
}

inline void vtkProjectionRound(double val, double& rnd)
{
  rnd = val;
}

//----------------------------------------------------------------------------
// clamping functions for each type

inline void vtkProjectionClamp(double val, signed char& clamp)
{
  if (val >= -128)
    {
    if (val <= 127)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = 127;
    return;
    }
  clamp = -128;
  return;
}

inline void vtkProjectionClamp(double val, char& clamp)
{
  if (val >= VTK_CHAR_MIN)
    {
    if (val <= VTK_CHAR_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_CHAR_MAX;
    return;
    }
  clamp = VTK_CHAR_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned char& clamp)
{
  if (val >= VTK_UNSIGNED_CHAR_MIN)
    {
    if (val <= VTK_UNSIGNED_CHAR_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_UNSIGNED_CHAR_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_CHAR_MIN;
  return;
}

inline void vtkProjectionClamp(double val, short& clamp)
{
  if (val >= VTK_SHORT_MIN)
    {
    if (val <= VTK_SHORT_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_SHORT_MAX;
    return;
    }
  clamp = VTK_SHORT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned short& clamp)
{
  if (val >= VTK_UNSIGNED_SHORT_MIN)
    {
    if (val <= VTK_UNSIGNED_SHORT_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_UNSIGNED_SHORT_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_SHORT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, int& clamp)
{
  if (val >= VTK_INT_MIN)
    {
    if (val <= VTK_INT_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_INT_MAX;
    return;
    }
  clamp = VTK_INT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned int& clamp)
{
  if (val >= VTK_UNSIGNED_INT_MIN)
    {
    if (val <= VTK_UNSIGNED_INT_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_UNSIGNED_INT_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_INT_MIN;
  return;
}

inline void vtkProjectionClamp(double val, long& clamp)
{
  if (val >= VTK_LONG_MIN)
    {
    if (val <= VTK_LONG_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_LONG_MAX;
    return;
    }
  clamp = VTK_LONG_MIN;
  return;
}

inline void vtkProjectionClamp(double val, unsigned long& clamp)
{
  if (val >= VTK_UNSIGNED_LONG_MIN)
    {
    if (val <= VTK_UNSIGNED_LONG_MAX)
      {
      clamp = vtkProjectionRound(val);
      return;
      }
    clamp = VTK_UNSIGNED_LONG_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_LONG_MIN;
  return;
}

#if defined(VTK_LONG_LONG_MIN) && defined(VTK_LONG_LONG_MAX)
inline void vtkProjectionClamp(double val, long long& clamp)
{
  if (val >= VTK_LONG_LONG_MIN)
    {
    if (val <= VTK_LONG_LONG_MAX)
      {
      clamp = (long long)(floor(val + 0.5));
      return;
      }
    clamp = VTK_LONG_LONG_MAX;
    return;
    }
  clamp = VTK_LONG_LONG_MIN;
  return;
}
#endif /* defined(VTK_LONG_LONG_MIN) && defined(VTK_LONG_LONG_MAX) */

#if defined(VTK_UNSIGNED_LONG_LONG_MIN) && defined(VTK_UNSIGNED_LONG_LONG_MAX)
inline void vtkProjectionClamp(double val, unsigned long long& clamp)
{
  if (val >= VTK_UNSIGNED_LONG_LONG_MIN)
    {
    if (val <= VTK_UNSIGNED_LONG_LONG_MAX)
      {
      clamp = (unsigned long long)(floor(val + 0.5));
      return;
      }
    clamp = VTK_UNSIGNED_LONG_LONG_MAX;
    return;
    }
  clamp = VTK_UNSIGNED_LONG_LONG_MIN;
  return;
}
#endif
/* defined(VTK_UNSIGNED_LONG_LONG_MIN) && defined(VTK_UNSIGNED_LONG_LONG_MAX)*/

#if defined(VTK_TYPE_USE___INT64) /* to ensure that __int64 is unique */
#if defined(VTK___INT64_MIN) && defined(VTK___INT64_MAX)
inline void vtkProjectionClamp(double val, __int64& clamp)
{
  if (val >= VTK___INT64_MIN)
    {
    if (val <= VTK___INT64_MAX)
      {
      clamp = (__int64)(floor(val + 0.5));
      return;
      }
    clamp = VTK___INT64_MAX;
    return;
    }
  clamp = VTK___INT64_MIN;
  return;
}
#endif /* defined(VTK___INT64_MIN) && defined(VTK___INT64_MAX) */
#endif /* defined(VTK_TYPE_USE___INT64) */

#if defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
#if defined(VTK_UNSIGNED__INT64_MIN) && defined(VTK_UNSIGNED__INT64_MAX)
inline void vtkProjectionClamp(double val, unsigned __int64& clamp)
{
  if (val >= VTK_UNSIGNED__INT64_MIN)
    {
    if (val <= VTK_UNSIGNED__INT64_MAX)
      {
      clamp = (unsigned __int64)(floor(val + 0.5));
      return;
      }
    clamp = VTK_UNSIGNED__INT64_MAX;
    return;
    }
  clamp = VTK_UNSIGNED__INT64_MIN;
  return;
}
#endif
/* defined(VTK_UNSIGNED__INT64_MIN) && defined(VTK_UNSIGNED__INT64_MAX) */
#endif
/* defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE) */

inline void vtkProjectionClamp(double val, float& clamp)
{
  clamp = val;
}

inline void vtkProjectionClamp(double val, double& clamp)
{
  clamp = val;
}

//----------------------------------------------------------------------------
template <class T1, class T2>
void vtkImageProjectionExecute(vtkImageProjection *self,
                               vtkImageData *inData, T1 *inPtr,
                               vtkImageData *outData, T2 *outPtr,
                               int outExt[6], int id)
{
  vtkIdType outIncX, outIncY, outIncZ;
  vtkIdType inInc[3];
  int inExt[6];

  // get increments to march through data
  inData->GetExtent(inExt);
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int numscalars = inData->GetNumberOfScalarComponents();
  int rowlen = (outExt[1] - outExt[0] + 1)*numscalars;

  // get the operation
  int operation = self->GetOperation();

  // get the dimension along which to do the projection
  int dimIndex = self->GetSliceDirection();
  if (dimIndex < 0)
    {
    dimIndex = 0;
    }
  else if (dimIndex > 2)
    {
    dimIndex = 2;
    }

  // clamp the range to the whole extent
  int range[2];
  self->GetSliceRange(range);
  if (range[0] < inExt[2*dimIndex])
    {
    range[0] = inExt[2*dimIndex];
    }
  if (range[1] > inExt[2*dimIndex+1])
    {
    range[1] = inExt[2*dimIndex+1];
    }
  int numSlices = range[1] - range[0] + 1;

  // averaging requires double precision summation
  double *rowBuffer = 0;
  if (operation == VTK_PROJECTION_AVERAGE ||
      operation == VTK_PROJECTION_SUM)
    {
    rowBuffer = new double[rowlen];
    }

  unsigned long count = 0;
  unsigned long target = ((unsigned long)(outExt[3]-outExt[2]+1)
                          *(outExt[5]-outExt[4]+1));
  target++;

  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    T1 *inPtrY = inPtr;
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (!id)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(1.0*target));
          }
        count++;
        }

      // ====== code for handling average and sum ======
      if (operation == VTK_PROJECTION_AVERAGE ||
          operation == VTK_PROJECTION_SUM)
        {
        T1 *inSlicePtr = inPtrY;
        double *rowPtr = rowBuffer;

        // initialize using first row
        T1 *inPtrX = inSlicePtr;
        for (int j = 0; j < rowlen; j++)
          {
          *rowPtr++ = *inPtrX++;
          }
        inSlicePtr += inInc[dimIndex];

        // perform the summation
        for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
          {
          inPtrX = inSlicePtr;
          rowPtr = rowBuffer;

          for (int i = 0; i < rowlen; i++)
            {
            *rowPtr++ += *inPtrX++;
            }
          inSlicePtr += inInc[dimIndex];
          }

        rowPtr = rowBuffer;
        if (operation == VTK_PROJECTION_AVERAGE)
          {
          // do the division via multiplication
          double factor = 1.0/numSlices;
          for (int k = 0; k < rowlen; k++)
            {
            vtkProjectionRound((*rowPtr++)*factor, *outPtr++);
            }
          }
        else // VTK_PROJECTION_SUM
          {
          // clamp to limits of numeric type
          for (int k = 0; k < rowlen; k++)
            {
            vtkProjectionClamp(*rowPtr++, *outPtr++);
            }
          }
        }

      // ====== code for handling max and min ======
      else
        {
        T1 *inSlicePtr = inPtrY;
        T2 *outPtrX = outPtr;

        // initialize using first row
        T1 *inPtrX = inSlicePtr;
        for (int j = 0; j < rowlen; j++)
          {
          *outPtrX++ = *inPtrX++;
          }
        inSlicePtr += inInc[dimIndex];

        if (operation == VTK_PROJECTION_MINIMUM)
          {
          for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
            {
            inPtrX = inSlicePtr;
            outPtrX = outPtr;

            for (int i = 0; i < rowlen; i++)
              {
              // branch prediction: most often, output is not changed
              T2 inVal = *inPtrX++;
              T2 outVal = *outPtrX;
              if (inVal > outVal)
                {
                outPtrX++;
                continue;
                }
              *outPtrX++ = inVal;
              }

            inSlicePtr += inInc[dimIndex];
            }
          }
        else // VTK_PROJECTION_MAXIMUM
          {
          for (int sliceIdx = 1; sliceIdx < numSlices; sliceIdx++)
            {
            inPtrX = inSlicePtr;
            outPtrX = outPtr;

            for (int i = 0; i < rowlen; i++)
              {
              // branch prediction: most often, output is not changed
              T2 inVal = *inPtrX++;
              T2 outVal = *outPtrX;
              if (inVal < outVal)
                {
                outPtrX++;
                continue;
                }
              *outPtrX++ = inVal;
              }

            inSlicePtr += inInc[dimIndex];
            }
          }

        outPtr += rowlen;
        }

      // ====== end of operation-specific code ======

      outPtr += outIncY;
      inPtrY += inInc[1];
      }

    outPtr += outIncZ;
    inPtr += inInc[2];
    }

  if (operation == VTK_PROJECTION_AVERAGE ||
      operation == VTK_PROJECTION_SUM)
    {
    delete [] rowBuffer;
    }
}


//----------------------------------------------------------------------------
void vtkImageProjection::ThreadedExecute(vtkImageData *inData,
                                         vtkImageData *outData,
                                         int outExt[6], int id)
{
  void *inPtr;
  void *outPtr;
  int inExt[6];
  int extent[6];
  int dimIndex;
  int range[2];

  vtkDebugMacro("Execute: inData = " << inData << ", outData = " << outData);

  // get the direction along which to sum slices
  dimIndex = this->GetSliceDirection();

  // clamp the range to the whole extent
  inData->GetWholeExtent(extent);
  this->GetSliceRange(range);
  if (range[0] < extent[2*dimIndex])
    {
    range[0] = extent[2*dimIndex];
    }
  if (range[1] > extent[2*dimIndex+1])
    {
    range[1] = extent[2*dimIndex+1];
    }

  // initialize input extent to output extent
  inExt[0] = outExt[0];
  inExt[1] = outExt[1];
  inExt[2] = outExt[2];
  inExt[3] = outExt[3];
  inExt[4] = outExt[4];
  inExt[5] = outExt[5];

  // the adjust for the slice range
  inExt[2*dimIndex] += range[0];
  inExt[2*dimIndex+1] += range[1];

  // now get the pointers for the extents
  inPtr = inData->GetScalarPointerForExtent(inExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);

  // get the scalar type
  int outScalarType = outData->GetScalarType();
  int inScalarType = inData->GetScalarType();

  // and call the execute method
  if (outScalarType == inScalarType)
    {
    switch (inScalarType)
      {
#if (VTK_MAJOR_VERSION < 5)
      vtkTemplateMacro7(vtkImageProjectionExecute, this,
                        inData, (VTK_TT *)(inPtr),
                        outData, (VTK_TT *)(outPtr), outExt, id);
#else
      vtkTemplateMacro(
        vtkImageProjectionExecute(this,
                                  inData, (VTK_TT *)(inPtr),
                                  outData, (VTK_TT *)(outPtr), outExt, id));
#endif
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
        return;
      }
    }
  else if (outScalarType == VTK_FLOAT)
    {
    switch (inScalarType)
      {
#if (VTK_MAJOR_VERSION < 5)
      vtkTemplateMacro7(vtkImageProjectionExecute, this,
                        inData, (VTK_TT *)(inPtr),
                        outData, (float *)(outPtr), outExt, id);
#else
      vtkTemplateMacro(
        vtkImageProjectionExecute( this,
                                   inData, (VTK_TT *)(inPtr),
                                   outData, (float *)(outPtr), outExt, id));
#endif
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
        return;
      }
    }
  else if (outScalarType == VTK_DOUBLE)
    {
    switch (inScalarType)
      {
#if (VTK_MAJOR_VERSION < 5)
      vtkTemplateMacro7(vtkImageProjectionExecute, this,
                        inData, (VTK_TT *)(inPtr),
                        outData, (double *)(outPtr), outExt, id);
#else
      vtkTemplateMacro(
        vtkImageProjectionExecute(this,
                                  inData, (VTK_TT *)(inPtr),
                                  outData, (double *)(outPtr), outExt, id));
#endif
      default:
        vtkErrorMacro("Execute: Unknown ScalarType");
        return;
      }
    }
  else
    {
    vtkErrorMacro("Execute: Unknown ScalarType");
    return;
    }
}

void vtkImageProjection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Operation: " << this->GetOperationAsString() << "\n";
  os << indent << "SliceDirection: " << this->GetSliceDirection() << "\n";
  os << indent << "SliceRange: " << this->GetSliceRange()[0] << " "
     << this->GetSliceRange()[1] << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "MultiSliceOutput: "
     << (this->MultiSliceOutput ? "On\n" : "Off\n");
}
