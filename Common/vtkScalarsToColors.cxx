/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarsToColors.h"
#include "vtkTemplateAliasMacro.h"
#include "vtkUnsignedCharArray.h"

#include <math.h>


//----------------------------------------------------------------------------
vtkScalarsToColors::vtkScalarsToColors()
{
  this->Alpha = 1.0;
  this->VectorComponent = 0;
  this->VectorMode = vtkScalarsToColors::COMPONENT;
  this->UseMagnitude = 0;
}

//----------------------------------------------------------------------------
// Description:
// Return true if all of the values defining the mapping have an opacity
// equal to 1. Default implementation return true.
int vtkScalarsToColors::IsOpaque()
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToComponent()
{
  this->SetVectorMode(vtkScalarsToColors::COMPONENT);
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToMagnitude()
{
  this->SetVectorMode(vtkScalarsToColors::MAGNITUDE);
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::SetVectorModeToColors()
{
  this->SetVectorMode(vtkScalarsToColors::COLORS);
}

//----------------------------------------------------------------------------
// do not use SetMacro() because we do not want the table to rebuild.
void vtkScalarsToColors::SetAlpha(double alpha)
{
  this->Alpha = (alpha < 0.0 ? 0.0 : (alpha > 1.0 ? 1.0 : alpha));
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *vtkScalarsToColors::MapScalars(vtkDataArray *scalars,
                                                     int colorMode, int comp)
{
  int numberOfComponents = scalars->GetNumberOfComponents();
  vtkUnsignedCharArray *newColors;
  vtkUnsignedCharArray *colors;

  // map scalars through lookup table only if needed
  if ( colorMode == VTK_COLOR_MODE_DEFAULT && 
       (colors=vtkUnsignedCharArray::SafeDownCast(scalars)) != NULL )
    {
    newColors = this->
      ConvertUnsignedCharToRGBA(colors, colors->GetNumberOfComponents(),
                                scalars->GetNumberOfTuples());
    }
  else
    {
    newColors = vtkUnsignedCharArray::New();
    newColors->SetNumberOfComponents(4);
    newColors->SetNumberOfTuples(scalars->GetNumberOfTuples());

    // If mapper did not specify a component, use the VectorMode
    if (comp < 0 && numberOfComponents > 1)
      {
      this->MapVectorsThroughTable(scalars->GetVoidPointer(comp),
                                   newColors->GetPointer(0),
                                   scalars->GetDataType(),
                                   scalars->GetNumberOfTuples(),
                                   scalars->GetNumberOfComponents(),
                                   VTK_RGBA);
      }
    else
      {
      if (comp < 0)
        {
        comp = 0;
        }
      if (comp >= numberOfComponents)
        {
        comp = numberOfComponents - 1;
        }

      // Map the scalars to colors
      this->MapScalarsThroughTable(scalars->GetVoidPointer(comp),
                                   newColors->GetPointer(0),
                                   scalars->GetDataType(),
                                   scalars->GetNumberOfTuples(),
                                   scalars->GetNumberOfComponents(),
                                   VTK_RGBA);
      }
    }

  return newColors;
}

//----------------------------------------------------------------------------
// Map a set of vector values through the table
void vtkScalarsToColors::MapVectorsThroughTable(
  void *input, unsigned char *output, int scalarType,
  int numValues, int inComponents, int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
    {
    vtkErrorMacro(<< "MapVectorsThroughTable: unrecognized color format");
    return;
    }

  this->UseMagnitude = 0;

  switch(this->GetVectorMode())
    {
    case vtkScalarsToColors::COMPONENT:
      {
      int scalarSize = vtkDataArray::GetDataTypeSize(scalarType);
      int vectorComponent = this->GetVectorComponent();
      if (vectorComponent < 0)
        {
        vectorComponent = 0;
        }
      if (vectorComponent >= inComponents)
        {
        vectorComponent = inComponents - 1;
        }

      this->MapScalarsThroughTable2(
        static_cast<unsigned char *>(input) + vectorComponent*scalarSize,
        output, scalarType, numValues, inComponents, outputFormat);
      }
      break;

    case vtkScalarsToColors::MAGNITUDE:
      {
      this->UseMagnitude = 1;
      this->MapScalarsThroughTable2(
        input, output, scalarType, numValues, inComponents, outputFormat);
      }
      break;

    case vtkScalarsToColors::COLORS:
      {
      this->MapColorsToColors(
        input, output, scalarType, numValues, inComponents, outputFormat);
      }
      break;
   }
}

//----------------------------------------------------------------------------
// Map a set of scalar values through the table
void vtkScalarsToColors::MapScalarsThroughTable(vtkDataArray *scalars, 
                                                unsigned char *output,
                                                int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
    {
    vtkErrorMacro(<< "MapScalarsThroughTable: unrecognized color format");
    return;
    }

  this->MapScalarsThroughTable(scalars->GetVoidPointer(0),
                               output,
                               scalars->GetDataType(),
                               scalars->GetNumberOfTuples(),
                               scalars->GetNumberOfComponents(),
                               outputFormat);
}

//----------------------------------------------------------------------------
// Color type converters

#define vtkScalarsToColorsLuminance(r, g, b) \
    ((r)*0.30 + (g)*0.59 + (b)*0.11)

void vtkColorsToColorsLuminanceToLuminance(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
    {
    *outPtr++ = *inPtr;
    inPtr += numComponents;
    }
  while (--count);
}

void vtkColorsToColorsLuminanceToRGB(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
    {
    unsigned char l = *inPtr;
    outPtr[0] = l;
    outPtr[1] = l;
    outPtr[2] = l;
    inPtr += numComponents;
    outPtr += 3;
    }
  while (--count);
}

void vtkColorsToColorsRGBToLuminance(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
    {
    unsigned char r = inPtr[0];
    unsigned char g = inPtr[1];
    unsigned char b = inPtr[2];
    *outPtr++ = static_cast<unsigned char>(
                  vtkScalarsToColorsLuminance(r, g, b) + 0.5);
    inPtr += numComponents;
    }
  while (--count);
}

void vtkColorsToColorsRGBToRGB(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents)
{
  do
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    inPtr += numComponents;
    outPtr += 3;
    }
  while (--count);
}

void vtkColorsToColorsLuminanceToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

void vtkColorsToColorsLuminanceToRGBA(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    unsigned char l = inPtr[0];
    outPtr[0] = l;
    outPtr[1] = l;
    outPtr[2] = l;
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
    }
  while (--count);
}

void vtkColorsToColorsRGBToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    unsigned char r = inPtr[0];
    unsigned char g = inPtr[1];
    unsigned char b = inPtr[2];
    outPtr[0] = static_cast<unsigned char>(
                  vtkScalarsToColorsLuminance(r, g, b) + 0.5);
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

void vtkColorsToColorsRGBToRGBA(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
    }
  while (--count);
}


void vtkColorsToColorsLuminanceAlphaToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  if (alpha >= 1)
    {
    do
      {
      outPtr[0] = inPtr[0];
      outPtr[1] = inPtr[1];
      inPtr += numComponents;
      outPtr += 2;
      }
    while (--count);
    }
  else
    {
    do
      {
      outPtr[0] = inPtr[0];
      outPtr[1] = static_cast<unsigned char>(inPtr[1]*alpha + 0.5);
      inPtr += numComponents;
      outPtr += 2;
      }
    while (--count);
    }
}

void vtkColorsToColorsLuminanceAlphaToRGBA(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  if (alpha >= 1)
    {
    do
      {
      float l = inPtr[0];
      float a = inPtr[1];
      outPtr[0] = l;
      outPtr[1] = l;
      outPtr[2] = l;
      outPtr[3] = a;
      inPtr += numComponents;
      outPtr += 4;
      }
    while (--count);
    }
  else
    {
    do
      {
      float l = inPtr[0];
      float a = inPtr[1];
      outPtr[0] = l;
      outPtr[1] = l;
      outPtr[2] = l;
      outPtr[3] = static_cast<unsigned char>(a*alpha + 0.5);
      inPtr += numComponents;
      outPtr += 4;
      }
    while (--count);
    }
}

void vtkColorsToColorsRGBAToLuminanceAlpha(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  do
    {
    unsigned char r = inPtr[0];
    unsigned char g = inPtr[1];
    unsigned char b = inPtr[2];
    unsigned char a = inPtr[3];
    outPtr[0] = static_cast<unsigned char>(
                  vtkScalarsToColorsLuminance(r, g, b) + 0.5);
    outPtr[1] = static_cast<unsigned char>(a*alpha + 0.5);
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

void vtkColorsToColorsRGBAToRGBA(
  const unsigned char *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float alpha)
{
  if (alpha >= 1)
    {
    do
      {
      outPtr[0] = inPtr[0];
      outPtr[1] = inPtr[1];
      outPtr[2] = inPtr[2];
      outPtr[3] = inPtr[3];
      inPtr += numComponents;
      outPtr += 4;
      }
    while (--count);
    }
  else
    {
    do
      {
      outPtr[0] = inPtr[0];
      outPtr[1] = inPtr[1];
      outPtr[2] = inPtr[2];
      outPtr[3] = static_cast<unsigned char>(inPtr[3]*alpha + 0.5);
      inPtr += numComponents;
      outPtr += 4;
      }
    while (--count);
    }
}

//----------------------------------------------------------------------------

template<class T>
void vtkColorsToColorsLuminanceToLuminance(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale)
{
  do
    {
    float l = inPtr[0];
    l = (l + shift)*scale;
    if (l < 0) { l = 0; }
    if (l > 255) { l = 255; }
    l += 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    inPtr += numComponents;
    outPtr += 1;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsLuminanceToRGB(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale)
{
  do
    {
    float l = inPtr[0];
    l = (l + shift)*scale;
    if (l < 0) { l = 0; }
    if (l > 255) { l = 255; }
    unsigned char lc = static_cast<unsigned char>(l + 0.5);
    outPtr[0] = lc;
    outPtr[1] = lc;
    outPtr[2] = lc;
    inPtr += numComponents;
    outPtr += 3;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsRGBToLuminance(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale)
{
  do
    {
    float r = inPtr[0];
    float g = inPtr[1];
    float b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    if (g < 0) { g = 0; }
    if (g > 255) { g = 255; }
    if (b < 0) { b = 0; }
    if (b > 255) { b = 255; }
    float l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    inPtr += numComponents;
    outPtr += 1;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsRGBToRGB(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale)
{
  do
    {
    float r = inPtr[0];
    float g = inPtr[1];
    float b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    if (g < 0) { g = 0; }
    if (g > 255) { g = 255; }
    if (b < 0) { b = 0; }
    if (b > 255) { b = 255; }
    r += 0.5;
    g += 0.5;
    b += 0.5;
    outPtr[0] = static_cast<unsigned char>(r);
    outPtr[1] = static_cast<unsigned char>(g);
    outPtr[2] = static_cast<unsigned char>(b);
    inPtr += numComponents;
    outPtr += 3;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsLuminanceToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    float l = inPtr[0];
    l = (l + shift)*scale;
    if (l < 0) { l = 0; }
    if (l > 255) { l = 255; }
    l += 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsLuminanceToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    float l = inPtr[0];
    l = (l + shift)*scale;
    if (l < 0) { l = 0; }
    if (l > 255) { l = 255; }
    unsigned char lc = static_cast<unsigned char>(l + 0.5);
    outPtr[0] = lc;
    outPtr[1] = lc;
    outPtr[2] = lc;
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsRGBToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    float r = inPtr[0];
    float g = inPtr[1];
    float b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    if (g < 0) { g = 0; }
    if (g > 255) { g = 255; }
    if (b < 0) { b = 0; }
    if (b > 255) { b = 255; }
    float l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = a;
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsRGBToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  unsigned char a = static_cast<unsigned char>(alpha*255 + 0.5);

  do
    {
    float r = inPtr[0];
    float g = inPtr[1];
    float b = inPtr[2];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    if (g < 0) { g = 0; }
    if (g > 255) { g = 255; }
    if (b < 0) { b = 0; }
    if (b > 255) { b = 255; }
    r += 0.5;
    g += 0.5;
    b += 0.5;
    outPtr[0] = static_cast<unsigned char>(r);
    outPtr[1] = static_cast<unsigned char>(g);
    outPtr[2] = static_cast<unsigned char>(b);
    outPtr[3] = a;
    inPtr += numComponents;
    outPtr += 4;
    }
  while (--count);
}


template<class T>
void vtkColorsToColorsLuminanceAlphaToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  do
    {
    float l = inPtr[0];
    float a = inPtr[1];
    l = (l + shift)*scale;
    if (l < 0) { l = 0; }
    if (l > 255) { l = 255; }
    if (a < 0) { a = 0; }
    if (a > 255) { a = 255; }
    l += 0.5;
    a = a*alpha + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsLuminanceAlphaToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  do
    {
    float l = inPtr[0];
    float a = inPtr[1];
    l = (l + shift)*scale;
    if (l < 0) { l = 0; }
    if (l > 255) { l = 255; }
    if (a < 0) { a = 0; }
    if (a > 255) { a = 255; }
    unsigned char lc = static_cast<unsigned char>(l + 0.5);
    a = a*alpha + 0.5;
    outPtr[0] = lc;
    outPtr[1] = lc;
    outPtr[2] = lc;
    outPtr[3] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 4;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsRGBAToLuminanceAlpha(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  do
    {
    float r = inPtr[0];
    float g = inPtr[1];
    float b = inPtr[2];
    float a = inPtr[3];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    a = (a + shift)*scale;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    if (g < 0) { g = 0; }
    if (g > 255) { g = 255; }
    if (b < 0) { b = 0; }
    if (b > 255) { b = 255; }
    if (a < 0) { a = 0; }
    if (a > 255) { a = 255; }
    a = a*alpha + 0.5;
    float l = vtkScalarsToColorsLuminance(r, g, b) + 0.5;
    outPtr[0] = static_cast<unsigned char>(l);
    outPtr[1] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 2;
    }
  while (--count);
}

template<class T>
void vtkColorsToColorsRGBAToRGBA(
  const T *inPtr, unsigned char *outPtr, vtkIdType count,
  int numComponents, float shift, float scale, float alpha)
{
  do
    {
    float r = inPtr[0];
    float g = inPtr[1];
    float b = inPtr[2];
    float a = inPtr[3];
    r = (r + shift)*scale;
    g = (g + shift)*scale;
    b = (b + shift)*scale;
    a = (a + shift)*scale;
    if (r < 0) { r = 0; }
    if (r > 255) { r = 255; }
    if (g < 0) { g = 0; }
    if (g > 255) { g = 255; }
    if (b < 0) { b = 0; }
    if (b > 255) { b = 255; }
    if (a < 0) { a = 0; }
    if (a > 255) { a = 255; }
    r += 0.5;
    g += 0.5;
    b += 0.5;
    a = a*alpha + 0.5;
    outPtr[0] = static_cast<unsigned char>(r);
    outPtr[1] = static_cast<unsigned char>(g);
    outPtr[2] = static_cast<unsigned char>(b);
    outPtr[3] = static_cast<unsigned char>(a);
    inPtr += numComponents;
    outPtr += 4;
    }
  while (--count);
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::MapColorsToColors(
  void *inPtr, unsigned char *outPtr, int inputDataType,
  int numberOfTuples, int numberOfComponents, int outputFormat)
{
  if (outputFormat < VTK_LUMINANCE || outputFormat > VTK_RGBA)
    {
    vtkErrorMacro(<< "MapColorsToColors: unrecognized color format");
    return;
    }

  if (numberOfTuples <= 0)
    {
    return;
    }

  unsigned char *newPtr = 0;
  if (inputDataType == VTK_BIT)
    {
    vtkIdType n = (numberOfTuples*numberOfComponents + 7) % 8;
    newPtr = new unsigned char [n];
    unsigned char *tmpPtr = newPtr;
    unsigned char *bitdata = static_cast<unsigned char *>(inPtr);
    for (vtkIdType i = 0; i < n; i += 8)
      {
      unsigned char b = *bitdata++;
      int j = 8;
      do
        {
        *tmpPtr++ = ((b >> (--j)) & 0x01);
        }
      while (j);
      }
    inPtr = newPtr;
    inputDataType = VTK_UNSIGNED_CHAR;
    }

  double *range = this->GetRange();
  float shift = static_cast<float>(-range[0]);
  float scale = static_cast<float>(range[1] - range[0]);
  if (scale*scale > 1e-30)
    {
    scale = 255/scale;
    }
  else if (scale < 0)
    {
    scale = -2.55e17;
    }
  else
    {
    scale = 2.55e17;
    }

  float alpha = static_cast<float>(this->Alpha);
  if (alpha < 0) { alpha = 0; }
  if (alpha > 1) { alpha = 1; }

  if (inputDataType == VTK_UNSIGNED_CHAR &&
      static_cast<int>(shift*scale + 0.5) == 0 &&
      static_cast<int>((255 + shift)*scale + 0.5) == 255)
    {
    if (outputFormat == VTK_RGBA)
      {
      if (numberOfComponents == 1)
        {
        vtkColorsToColorsLuminanceToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      else if (numberOfComponents == 2)
        {
        vtkColorsToColorsLuminanceAlphaToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      else if (numberOfComponents == 3)
        {
        vtkColorsToColorsRGBToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      else
        {
        vtkColorsToColorsRGBAToRGBA(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      }
    else if (outputFormat == VTK_RGB)
      {
      if (numberOfComponents < 3)
        {
        vtkColorsToColorsLuminanceToRGB(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
        }
      else
        {
        vtkColorsToColorsRGBToRGB(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
        }
      }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
      {
      if (numberOfComponents == 1)
        {
        vtkColorsToColorsLuminanceToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      else if (numberOfComponents == 2)
        {
        vtkColorsToColorsLuminanceAlphaToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      else if (numberOfComponents == 3)
        {
        vtkColorsToColorsRGBToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      else
        {
        vtkColorsToColorsRGBAToLuminanceAlpha(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents, alpha);
        }
      }
    else if (outputFormat == VTK_LUMINANCE)
      {
      if (numberOfComponents < 3)
        {
        vtkColorsToColorsLuminanceToLuminance(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
        }
      else
        {
        vtkColorsToColorsRGBToLuminance(
          static_cast<unsigned char*>(inPtr), outPtr,
          numberOfTuples, numberOfComponents);
        }
      }
    }
  else
    {
    // must apply shift scale and/or do type conversion
    if (outputFormat == VTK_RGBA)
      {
      if (numberOfComponents == 1)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsLuminanceToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      else if (numberOfComponents == 2)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsLuminanceAlphaToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      else if (numberOfComponents == 3)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsRGBToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      else
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsRGBAToRGBA(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      }
    else if (outputFormat == VTK_RGB)
      {
      if (numberOfComponents < 3)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsLuminanceToRGB(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
          }
        }
      else
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsRGBToRGB(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
          }
        }
      }
    else if (outputFormat == VTK_LUMINANCE_ALPHA)
      {
      if (numberOfComponents == 1)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsLuminanceToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      else if (numberOfComponents == 2)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsLuminanceAlphaToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      else if (numberOfComponents == 3)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsRGBToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      else
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsRGBAToLuminanceAlpha(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale, alpha));
          }
        }
      }
    else if (outputFormat == VTK_LUMINANCE)
      {
      if (numberOfComponents < 3)
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsLuminanceToLuminance(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
          }
        }
      else
        {
        switch (inputDataType)
          {
          vtkTemplateAliasMacro(
            vtkColorsToColorsRGBToLuminance(
              static_cast<VTK_TT*>(inPtr), outPtr,
              numberOfTuples, numberOfComponents, shift, scale));
          }
        }
      }
    }

  if (newPtr)
    {
    delete [] newPtr;
    }
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *vtkScalarsToColors::ConvertUnsignedCharToRGBA(
  vtkUnsignedCharArray *colors, int numComp, int numTuples)
{
  if ( numComp == 4 && this->Alpha >= 1.0 )
    {
    colors->Register(this);
    return colors;
    }
    
  unsigned char *cptr = colors->GetPointer(0);
  vtkUnsignedCharArray *newColors = vtkUnsignedCharArray::New();
  newColors->SetNumberOfComponents(4);
  newColors->SetNumberOfTuples(numTuples);
  unsigned char *nptr = newColors->GetPointer(0);
  int i;

  if ( this->Alpha >= 1.0 )
    {
    switch (numComp)
      {
      case 1:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = 255;
          }
        break;

      case 2:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          }
        break;

      case 3:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = 255;
          }
        break;

      default:
        vtkErrorMacro(<<"Cannot convert colors");
        return NULL;
      }
    }
  else //blending required
    {
    unsigned char alpha;
    switch (numComp)
      {
      case 1:
        alpha = static_cast<unsigned char>(this->Alpha*255);
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = alpha;
          }
        break;

      case 2:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = static_cast<unsigned char>((*cptr)*this->Alpha); cptr++;
          }
        break;

      case 3:
        alpha = static_cast<unsigned char>(this->Alpha*255);
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = alpha;
          }
        break;

      case 4:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = static_cast<unsigned char>((*cptr)*this->Alpha); cptr++;
          }
        break;

      default:
        vtkErrorMacro(<<"Cannot convert colors");
        return NULL;
      }
    }
  
  return newColors;
}

//----------------------------------------------------------------------------
void vtkScalarsToColors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Alpha: " << this->Alpha << endl;
  if (this->VectorMode == vtkScalarsToColors::MAGNITUDE)
    {
    os << indent << "VectorMode: Magnitude\n";
    }
  else
    {
    os << indent << "VectorMode: Component\n";
    os << indent << "VectorComponent: " << this->VectorComponent << endl;
    }
}
