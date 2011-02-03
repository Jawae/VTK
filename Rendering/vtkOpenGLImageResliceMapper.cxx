/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageResliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLImageResliceMapper.h"

#include "vtkObjectFactory.h"
#include "vtkImageResliceToColors.h"
#include "vtkImageData.h"
#include "vtkImage.h"
#include "vtkImageProperty.h"
#include "vtkDataArray.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkGarbageCollector.h"
#include "vtkTemplateAliasMacro.h"

#include <math.h>

#include "vtkOpenGL.h"
#include "vtkgl.h" // vtkgl namespace

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLImageResliceMapper);
#endif

//----------------------------------------------------------------------------
// Initializes an instance, generates a unique index.
vtkOpenGLImageResliceMapper::vtkOpenGLImageResliceMapper()
{
  this->Index = 0;
  this->ImageReslice = 0;
  this->RenderWindow = 0;
  this->TextureSize[0] = 0;
  this->TextureSize[1] = 0;
  this->TextureBytesPerPixel = 1;
  this->ImageReslice = vtkImageResliceToColors::New();
  this->ResliceMatrix = vtkMatrix4x4::New();
  this->WorldToDataMatrix = vtkMatrix4x4::New();
  this->SliceToWorldMatrix = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------------
vtkOpenGLImageResliceMapper::~vtkOpenGLImageResliceMapper()
{
  if (this->ImageReslice)
    {
    this->ImageReslice->Delete();
    }
  if (this->ResliceMatrix)
    {
    this->ResliceMatrix->Delete();
    }
  if (this->WorldToDataMatrix)
    {
    this->WorldToDataMatrix->Delete();
    }
  if (this->SliceToWorldMatrix)
    {
    this->SliceToWorldMatrix->Delete();
    }
  this->RenderWindow = NULL;
}

//----------------------------------------------------------------------------
// Release the graphics resources used by this texture.
void vtkOpenGLImageResliceMapper::ReleaseGraphicsResources(vtkWindow *renWin)
{
  if (this->Index && renWin && renWin->GetMapped())
    {
    static_cast<vtkRenderWindow *>(renWin)->MakeCurrent();
#ifdef GL_VERSION_1_1
    // free any textures
    if (glIsTexture(this->Index))
      {
      GLuint tempIndex;
      tempIndex = this->Index;
      // NOTE: Sun's OpenGL seems to require disabling of texture
      // before deletion
      glDisable(GL_TEXTURE_2D);
      glDeleteTextures(1, &tempIndex);
      }
#else
    if (glIsList(this->Index))
      {
      glDeleteLists(this->Index,1);
      }
#endif
    this->TextureSize[0] = 0;
    this->TextureSize[1] = 0;
    this->TextureBytesPerPixel = 1;
    }
  this->Index = 0;
  this->RenderWindow = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLImageResliceMapper::ComputeTextureSize(
  const int extent[6], int &xdim, int &ydim,
  int imageSize[2], int textureSize[2])
{
  // find dimension indices that will correspond to the
  // columns and rows of the 2D texture
  xdim = 1;
  ydim = 2;
  if (extent[0] != extent[1])
    {
    xdim = 0;
    if (extent[2] != extent[3])
      {
      ydim = 1;
      }
    }

  // compute the image dimensions
  imageSize[0] = (extent[xdim*2+1] - extent[xdim*2] + 1);
  imageSize[1] = (extent[ydim*2+1] - extent[ydim*2] + 1);

  // find the target size of the power-of-two texture
  for (int i = 0; i < 2; i++)
    {
    int powerOfTwo = 1;
    while (powerOfTwo < imageSize[i])
      {
      powerOfTwo <<= 1;
      }
    textureSize[i] = powerOfTwo;
    }
}

//----------------------------------------------------------------------------
// Copy unsigned char data
static
void vtkImageMapperLookupTable(
  void *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  int scalarType, vtkScalarsToColors *lookupTable)
{
  // number of values per row of input image
  int rowLength = extent[1] - extent[0] + 1;
  int scalarSize = vtkDataArray::GetDataTypeSize(scalarType);

  // convert incY from continuous increment to regular increment
  outIncY += 4*rowLength;
  inIncY += rowLength;
  inIncY *= scalarSize;
  inIncZ *= scalarSize;

  // loop through the data and copy it for the texture
  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
      {
      lookupTable->MapScalarsThroughTable2(
        inPtr, outPtr, scalarType, rowLength, numComp, VTK_RGBA);

      outPtr += outIncY;
      inPtr = static_cast<void *>(static_cast<char *>(inPtr) + inIncY);
      }
    outPtr += outIncZ;
    inPtr = static_cast<void *>(static_cast<char *>(inPtr) + inIncZ);
    }
}

//----------------------------------------------------------------------------
// Copy unsigned char data
static
void vtkImageMapperCopy(
  unsigned char *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ)
{
  double alpha = 1.0;

  // number of values per row of input image
  int rowLength = extent[1] - extent[0] + 1;
  if (rowLength <= 0)
    {
    return;
    }
  unsigned char alpha2 = static_cast<unsigned char>(alpha*255);
  int alpha3 = static_cast<int>(alpha*65536);

  // loop through the data and copy it for the texture
  if (numComp == 1)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          unsigned char val = *inPtr++;
          *outPtr++ = val;
          *outPtr++ = val;
          *outPtr++ = val;
          *outPtr++ = alpha2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 2)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          unsigned char val = *inPtr++;
          *outPtr++ = val;
          *outPtr++ = val;
          *outPtr++ = val;
          *outPtr++ = (((*inPtr++)*alpha3) >> 16);
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 3)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          *outPtr++ = *inPtr++;
          *outPtr++ = *inPtr++;
          *outPtr++ = *inPtr++;
          *outPtr++ = alpha2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 4)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          *outPtr++ = *inPtr++;
          *outPtr++ = *inPtr++;
          *outPtr++ = *inPtr++;
          *outPtr++ = (((*inPtr++)*alpha3) >> 16);
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
// Convert data to unsigned char
template<class T>
void vtkImageMapperShiftScale(
  const T *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  double shift, double scale)
{
  double alpha = 1.0;

  // number of values per row of input image
  int rowLength = (extent[1] - extent[0] + 1);
  if (rowLength <= 0)
    {
    return;
    }

  unsigned char alpha2 = static_cast<unsigned char>(alpha*255);

  // loop through the data and copy it for the texture
  if (numComp == 1)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          double val = (*inPtr++ + shift)*scale;
          if (val < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char cval = static_cast<unsigned char>(val);
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = alpha2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 2)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          double val = (*inPtr++ + shift)*scale;
          if (val < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char cval = static_cast<unsigned char>(val);
          val = (*inPtr++ + shift)*scale;
          if (val < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char aval = static_cast<unsigned char>(val*alpha);
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = aval;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 3)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          double r = (*inPtr++ + shift)*scale;
          if (r < 0) { r = 0; }
          if (r > 255) { r = 255; }
          double g = (*inPtr++ + shift)*scale;
          if (g < 0) { g = 0; }
          if (g > 255) { g = 255; }
          double b = (*inPtr++ + shift)*scale;
          if (b < 0) { b = 0; }
          if (b > 255) { b = 255; }
          *outPtr++ = static_cast<unsigned char>(r);
          *outPtr++ = static_cast<unsigned char>(g);
          *outPtr++ = static_cast<unsigned char>(b);
          *outPtr++ = alpha2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 4)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          double r = (*inPtr++ + shift)*scale;
          if (r < 0) { r = 0; }
          if (r > 255) { r = 255; }
          double g = (*inPtr++ + shift)*scale;
          if (g < 0) { g = 0; }
          if (g > 255) { g = 255; }
          double b = (*inPtr++ + shift)*scale;
          if (b < 0) { b = 0; }
          if (b > 255) { b = 255; }
          double a = (*inPtr++ + shift)*scale;
          if (a < 0) { a = 0; }
          if (a > 255) { a = 255; }
          *outPtr++ = static_cast<unsigned char>(r);
          *outPtr++ = static_cast<unsigned char>(g);
          *outPtr++ = static_cast<unsigned char>(b);
          *outPtr++ = static_cast<unsigned char>(a*alpha);
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

//----------------------------------------------------------------------------
// Convert 8-bit and 16-bit data to unsigned char by integer math,
// on some systems this will be significantly faster than float math
template<class T>
void vtkImageMapperIntegerShiftScale(
  const T *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  double shift, double scale)
{
  double alpha = 1.0;

  // Find the number of bits to use for the fraction:
  // continue increasing the bits until there is an overflow
  // in the worst case, then decrease by 1.
  // The "*2.0" and "*1.0" ensure that the comparison is done
  // with double-precision math.
  int bitShift = 0;
  double absScale = ((scale < 0) ? -scale : scale);

  while ((static_cast<int>(1 << bitShift)*absScale) *
         2.0*VTK_UNSIGNED_SHORT_MAX < 1.0*VTK_INT_MAX)
    {
    bitShift++;
    }
  bitShift--;

  int intScale = static_cast<int>(scale*(1 << bitShift));
  int intShift = static_cast<int>(intScale*shift);

  // number of values per row of input image
  int rowLength = (extent[1] - extent[0] + 1);
  if (rowLength <= 0)
    {
    return;
    }

  unsigned char alpha2 = static_cast<unsigned char>(alpha*255);
  int alpha3 = static_cast<unsigned char>(alpha*65536);

  // loop through the data and copy it for the texture
  if (numComp == 1)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          int tmpval = *inPtr++ * intScale + intShift;
          int val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char cval = static_cast<unsigned char>(val);
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = alpha2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 2)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          int tmpval = *inPtr++ * intScale + intShift;
          int val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char cval = static_cast<unsigned char>(val);
          tmpval = *inPtr++ * intScale + intShift;
          val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char aval = static_cast<unsigned char>((val*alpha3) >> 16);
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = cval;
          *outPtr++ = aval;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 3)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          int tmpval = *inPtr++ * intScale + intShift;
          int val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char r = static_cast<unsigned char>(val);
          tmpval = *inPtr++ * intScale + intShift;
          val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char g = static_cast<unsigned char>(val);
          tmpval = *inPtr++ * intScale + intShift;
          val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char b = static_cast<unsigned char>(val);
          *outPtr++ = r;
          *outPtr++ = g;
          *outPtr++ = b;
          *outPtr++ = alpha2;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
  else if (numComp == 4)
    {
    for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
      {
      for (int idxY = extent[2]; idxY <= extent[3]; idxY++)
        {
        int idx = rowLength;
        do
          {
          // Pixel operation
          int tmpval = *inPtr++ * intScale + intShift;
          int val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char r = static_cast<unsigned char>(val);
          tmpval = *inPtr++ * intScale + intShift;
          val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char g = static_cast<unsigned char>(val);
          tmpval = *inPtr++ * intScale + intShift;
          val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char b = static_cast<unsigned char>(val);
          tmpval = *inPtr++ * intScale + intShift;
          val = (tmpval >> bitShift);
          if (tmpval < 0) { val = 0; }
          if (val > 255) { val = 255; }
          unsigned char a = static_cast<unsigned char>((val*alpha3) >> 16);
          *outPtr++ = r;
          *outPtr++ = g;
          *outPtr++ = b;
          *outPtr++ = a;
          }
        while (--idx);

        outPtr += outIncY;
        inPtr += inIncY;
        }
      outPtr += outIncZ;
      inPtr += inIncZ;
      }
    }
}

inline void vtkImageMapperShiftScale(
  const vtkTypeInt8 *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  double shift, double scale)
{
   vtkImageMapperIntegerShiftScale(inPtr, outPtr, extent, numComp,
                                   inIncY, inIncZ, outIncY, outIncZ,
                                   shift, scale);
}

inline void vtkImageMapperShiftScale(
  const vtkTypeUInt8 *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  double shift, double scale)
{
  vtkImageMapperIntegerShiftScale(inPtr, outPtr, extent, numComp,
                                  inIncY, inIncZ, outIncY, outIncZ,
                                  shift, scale);
}

inline void vtkImageMapperShiftScale(
  const vtkTypeInt16 *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  double shift, double scale)
{
  vtkImageMapperIntegerShiftScale(inPtr, outPtr, extent, numComp,
                                  inIncY, inIncZ, outIncY, outIncZ,
                                  shift, scale);
}

inline void vtkImageMapperShiftScale(
  const vtkTypeUInt16 *inPtr, unsigned char *outPtr, const int extent[6],
  int numComp, int inIncY, int inIncZ, int outIncY, int outIncZ,
  double shift, double scale)
{
  vtkImageMapperIntegerShiftScale(inPtr, outPtr, extent, numComp,
                                  inIncY, inIncZ, outIncY, outIncZ,
                                  shift, scale);
}

//----------------------------------------------------------------------------
// Given an image and an extent, this method will return a
// contiguous block of unsigned char data that can be loaded
// into a texture.  The dimensions of the output data are
// xsize, ysize.  If releaseData is set, then the returned
// array must be freed after use with delete [].  If reuseTexture
// is set, then the returned data should be loaded into the
// current texture instead of a new texture being created.
unsigned char *vtkOpenGLImageResliceMapper::MakeTextureData(
  vtkImageProperty *property, vtkImageData *input, int extent[6],
  int &xsize, int &ysize, int &bytesPerPixel,
  int &releaseData, int &reuseTexture)
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  vtkOpenGLImageResliceMapper::ComputeTextureSize(
    extent, xdim, ydim, imageSize, textureSize);

  // the texture can only be reused under certain circumstances
  reuseTexture = 0;

  // will be set if the extent represents contiguous memory
  int contiguous = 0;

  // number of components
  int numComp = input->GetNumberOfScalarComponents();
  bytesPerPixel = 4;
  int scalarType = input->GetScalarType();

  // lookup table and window/level
  double colorWindow = 255.0;
  double colorLevel = 125.0;
  vtkScalarsToColors *lookupTable = 0;

  if (property)
    {
    colorWindow = property->GetColorWindow();
    colorLevel = property->GetColorLevel();
    lookupTable = property->GetLookupTable();
    }

  if (lookupTable)
    {
    bytesPerPixel = 4;
    }

  // get spacing/origin for the quad coordinates
  double *spacing = input->GetSpacing();
  double *origin = input->GetOrigin();

  // compute the world coordinates of the quad
  this->Coords[0] = extent[0]*spacing[0] + origin[0];
  this->Coords[1] = extent[2]*spacing[1] + origin[1];
  this->Coords[2] = extent[4]*spacing[2] + origin[2];

  this->Coords[3] = extent[1]*spacing[0] + origin[0];
  this->Coords[4] = extent[2 + (xdim == 1)]*spacing[1] + origin[1];
  this->Coords[5] = extent[4]*spacing[2] + origin[2];

  this->Coords[6] = extent[1]*spacing[0] + origin[0];
  this->Coords[7] = extent[3]*spacing[1] + origin[1];
  this->Coords[8] = extent[5]*spacing[2] + origin[2];

  this->Coords[9] = extent[0]*spacing[0] + origin[0];
  this->Coords[10] = extent[2 + (ydim == 1)]*spacing[1] + origin[1];
  this->Coords[11] = extent[5]*spacing[2] + origin[2];

  // compute the tcoords
  this->TCoords[0] = 0.5/textureSize[0];
  this->TCoords[1] = 0.5/textureSize[1];

  this->TCoords[2] = (imageSize[0] - 0.5)/textureSize[0];
  this->TCoords[3] = this->TCoords[1];

  this->TCoords[4] = this->TCoords[2];
  this->TCoords[5] = (imageSize[1] - 0.5)/textureSize[1];

  this->TCoords[6] = this->TCoords[0];
  this->TCoords[7] = this->TCoords[5];

  // generally, the whole texture will have to be reloaded
  xsize = textureSize[0];
  ysize = textureSize[1];

#ifdef GL_VERSION_1_1
  // reuse texture if texture size has not changed
  if (textureSize[0] == this->TextureSize[0] &&
      textureSize[1] == this->TextureSize[1] &&
      bytesPerPixel == this->TextureBytesPerPixel)
    {
    // if texture is reused, only reload the image portion
    xsize = imageSize[0];
    ysize = imageSize[1];
    reuseTexture = 1;
    }
#endif

  // if the image is already of the desired size and type
  if (xsize == imageSize[0] && ysize == imageSize[1])
    {
    // Check if the data needed for the texture is a contiguous region
    // of the input data: this requires that xdim = 0 and ydim = 1
    // OR xextent = 1 pixel and xdim = 1 and ydim = 2
    // OR xdim = 0 and ydim = 2 and yextent = 1 pixel.
    // In addition the corresponding x display extents must match the
    // extent of the data
    int *dataExtent = input->GetExtent();

    if ( (xdim == 0 && ydim == 1 &&
          extent[0] == dataExtent[0] && extent[1] == dataExtent[1]) ||
         (dataExtent[0] == dataExtent[1] && xdim == 1 &&
          extent[2] == dataExtent[2] && extent[3] == dataExtent[3]) ||
         (dataExtent[2] == dataExtent[3] && xdim == 0 && ydim == 2 &&
          extent[0] == dataExtent[0] && extent[1] == dataExtent[1]) )
      {
      contiguous = 1;
      // if contiguous and correct data type, use data as-is
      if (scalarType == VTK_UNSIGNED_CHAR && numComp == bytesPerPixel)
        {
        releaseData = 0;
        return static_cast<unsigned char *>(
          input->GetScalarPointerForExtent(extent));
        }
      }
    }

  // could not directly use input data, so allocate a new array
  releaseData = 1;

  unsigned char *outPtr = new unsigned char [ysize*xsize*bytesPerPixel];

  // output increments
  vtkIdType outIncY = bytesPerPixel*(xsize - imageSize[0]);
  vtkIdType outIncZ = 0;
  if (ydim == 2)
    {
    outIncZ = outIncY;
    outIncY = 0;
    }

  // input pointer and increments
  vtkIdType inIncX, inIncY, inIncZ;
  void *inPtr = input->GetScalarPointerForExtent(extent);
  input->GetContinuousIncrements(extent, inIncX, inIncY, inIncZ);

  // reformat the data for use as a texture
  /*
  if (property && lookupTable)
    {
    // apply a lookup table
    vtkImageMapperLookupTable(inPtr, outPtr, extent, numComp,
                              inIncY, inIncZ, outIncY, outIncZ,
                              scalarType, lookupTable);
    }
  else // no lookup table, do a shift/scale calculation
    {
    double shift = 0.0;
    double scale = 1.0;

    if (property)
      {
      shift = 0.5*colorWindow - colorLevel;
      if (colorWindow != 0.0)
        {
        scale = 255.0/colorWindow;
        }
      else
        {
        scale = 1e+32;
        }
      }

    // Check if the data can be simply copied
    if (scalarType == VTK_UNSIGNED_CHAR &&
        static_cast<int>(shift*scale) == 0 &&
        static_cast<int>((255 + shift)*scale) == 255)
      {
    */
      vtkImageMapperCopy(static_cast<unsigned char *>(inPtr),
                         outPtr, extent, 4, //numComp,
                         inIncY, inIncZ, outIncY, outIncZ);
    /*
      }
    else
      {
      switch (scalarType)
        {
        vtkTemplateAliasMacro(
          vtkImageMapperShiftScale(static_cast<VTK_TT*>(inPtr),
                                   outPtr, extent, numComp,
                                   inIncY, inIncZ, outIncY, outIncZ,
                                   shift, scale));
        default:
          vtkErrorWithObjectMacro(
            this, "MakeTextureData: Unknown input ScalarType");
        }
      }
    }
  */

  return outPtr;
}

//----------------------------------------------------------------------------
// Load the given image extent into a texture and render it
void vtkOpenGLImageResliceMapper::InternalLoad(
  vtkRenderer *ren, vtkImageProperty *property, vtkImageData *input,
  int extent[6])
{
  GLenum format = GL_LUMINANCE;

  // Get the mtime of the property, including the lookup table
  unsigned long propertyMTime = 0;
  if (property)
    {
    propertyMTime = property->GetMTime();
    vtkScalarsToColors *table = property->GetLookupTable();
    if (table)
      {
      unsigned long mtime = table->GetMTime();
      if (mtime > propertyMTime)
        {
        propertyMTime = mtime;
        }
      }
    }

  // need to reload the texture
  if (this->GetMTime() > this->LoadTime.GetMTime() ||
      propertyMTime > this->LoadTime.GetMTime() ||
      this->WorldToDataMatrix->GetMTime() > this->LoadTime.GetMTime() ||
      input->GetMTime() > this->LoadTime.GetMTime() ||
      ren->GetRenderWindow() != this->RenderWindow ||
      static_cast<vtkOpenGLRenderWindow*>(ren->GetRenderWindow())
        ->GetContextCreationTime() > this->LoadTime)
    {
    int xsize, ysize, bytesPerPixel;
    int releaseData, reuseTexture;
    unsigned char *data = this->MakeTextureData(property,
      input, extent, xsize, ysize, bytesPerPixel, releaseData, reuseTexture);
    GLuint tempIndex=0;

    if (reuseTexture)
      {
#ifdef GL_VERSION_1_1
      glBindTexture(GL_TEXTURE_2D, this->Index);
#endif
      }
    else
      {
      // free any old display lists
      this->ReleaseGraphicsResources(ren->GetRenderWindow());
      this->RenderWindow = ren->GetRenderWindow();

      // define a display list for this texture
      // get a unique display list id
#ifdef GL_VERSION_1_1
      glGenTextures(1, &tempIndex);
      this->Index = static_cast<long>(tempIndex);
      glBindTexture(GL_TEXTURE_2D, this->Index);
#else
      this->Index = glGenLists(1);
      glDeleteLists(static_cast<GLuint>(this->Index),
                    static_cast<GLsizei>(0));
      glNewList (static_cast<GLuint>(this->Index), GL_COMPILE);
#endif

      static_cast<vtkOpenGLRenderWindow *>(ren->GetRenderWindow())
        ->RegisterTextureResource(this->Index);
      }

    // only if texture, rather than reslice, is doing the interpolation
    if (1) //property->GetInterpolationType())
      {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
    else
      {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    int internalFormat = bytesPerPixel;
    switch (bytesPerPixel)
      {
      case 1: format = GL_LUMINANCE; break;
      case 2: format = GL_LUMINANCE_ALPHA; break;
      case 3: format = GL_RGB; break;
      case 4: format = GL_RGBA; break;
      }
    // if we are using OpenGL 1.1, you can force 32 bit textures
#ifdef GL_VERSION_1_1
    switch (bytesPerPixel)
      {
      case 1: internalFormat = GL_LUMINANCE8; break;
      case 2: internalFormat = GL_LUMINANCE8_ALPHA8; break;
      case 3: internalFormat = GL_RGB8; break;
      case 4: internalFormat = GL_RGBA8; break;
      }
#endif

    if (reuseTexture)
      {
#ifdef GL_VERSION_1_1
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                      xsize, ysize, format, GL_UNSIGNED_BYTE,
                      static_cast<const GLvoid *>(data));
#endif
      }
    else
      {
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                   xsize, ysize, 0, format,
                   GL_UNSIGNED_BYTE, static_cast<const GLvoid *>(data));
      this->TextureSize[0] = xsize;
      this->TextureSize[1] = ysize;
      this->TextureBytesPerPixel = bytesPerPixel;
      }

#ifndef GL_VERSION_1_1
    glEndList();
#endif
    // modify the load time to the current time
    this->LoadTime.Modified();
    if (releaseData)
      {
      delete [] data;
      }
    }

  // execute the display list that uses creates the texture
#ifdef GL_VERSION_1_1
  glBindTexture(GL_TEXTURE_2D, this->Index);
#else
  glCallList(static_cast<GLuint>(this->Index));
#endif

  // don't accept fragments if they have zero opacity:
  // this will stop the zbuffer from be blocked by totally
  // transparent texture fragments.
  glAlphaFunc(GL_GREATER, static_cast<GLclampf>(0));
  glEnable(GL_ALPHA_TEST);

  // now bind it
  glEnable(GL_TEXTURE_2D);

/*
  GLint uUseTexture = -1;
  GLint uTexture = -1;

  // depth peeling
  vtkOpenGLRenderer *oRenderer = static_cast<vtkOpenGLRenderer *>(ren);

  if (oRenderer->GetDepthPeelingHigherLayer())
    {
    uUseTexture = oRenderer->GetUseTextureUniformVariable();
    uTexture = oRenderer->GetTextureUniformVariable();
    vtkgl::Uniform1i(uUseTexture, 1);
    vtkgl::Uniform1i(uTexture, 0); // active texture 0
    }

#ifdef GL_VERSION_1_1
  // do an offset to avoid depth buffer issues
  if (vtkMapper::GetResolveCoincidentTopology() !=
      VTK_RESOLVE_SHIFT_ZBUFFER )
    {
    double f, u;
    glEnable(GL_POLYGON_OFFSET_FILL);
    vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(f,u);
    glPolygonOffset(f,u);
    }
#endif
*/

  // draw the quad
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glColor4f(1.0, 1.0, 1.0, property->GetOpacity());
  glBegin(GL_QUADS);
  for (int i = 0; i < 4; i++ )
    {
    glTexCoord2dv(this->TCoords + i*2);
    glVertex3dv(this->Coords + i*3);
    }
  glEnd();
  // Turn lighting back on
  glEnable(GL_LIGHTING);
}

//----------------------------------------------------------------------------
// Determine if a given texture size is supported by the video card
int vtkOpenGLImageResliceMapper::TextureSizeOK(const int size[2])
{
#ifdef GL_VERSION_1_1
  // First ask OpenGL what the max texture size is
  GLint maxSize;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
  if (size[0] > maxSize || size[1] > maxSize)
    {
    return 0;
    }

  // Test a proxy texture to see if it fits in memory
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGBA8, size[0], size[1],
               0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  GLint params = 0;
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,
                           &params);

  // if it does fit, we will render it later
  return (params == 0 ? 0 : 1);
#else
  // Otherwise we are version 1.0 and we'll just assume the card
  // can do 1024x1024
  return (size[0] <= 1024 && size[1] <= 1024);
#endif
}

//----------------------------------------------------------------------------
// Subdivide the image until the pieces fit into texture memory
void vtkOpenGLImageResliceMapper::RecursiveLoad(
  vtkRenderer *ren, vtkImageProperty *property, vtkImageData *input,
  int extent[6])
{
  int xdim, ydim;
  int imageSize[2];
  int textureSize[2];

  // compute image size and texture size from extent
  vtkOpenGLImageResliceMapper::ComputeTextureSize(
    extent, xdim, ydim, imageSize, textureSize);

  // Check if we can fit this texture in memory
  if (this->TextureSizeOK(textureSize))
    {
    // We can fit it - render
    this->InternalLoad(ren, property, input, extent);
    }

  // If the texture does not fit, then subdivide and render
  // each half.  Unless the graphics card couldn't handle
  // a texture a small as 256x256, because if it can't handle
  // that, then something has gone horribly wrong.
  else if (textureSize[0] > 256 || textureSize[1] > 256)
    {
    int subExtent[6];
    subExtent[0] = extent[0]; subExtent[1] = extent[1];
    subExtent[2] = extent[2]; subExtent[3] = extent[3];
    subExtent[4] = extent[4]; subExtent[5] = extent[5];

    // Which is larger, x or y?
    int idx = ydim;
    int tsize = textureSize[1];
    if (textureSize[0] > textureSize[1])
      {
      idx = xdim;
      tsize = textureSize[0];
      }

    // Divide size by two
    tsize /= 2;

    // Render each half recursively
    subExtent[idx*2] = extent[idx*2];
    subExtent[idx*2 + 1] = extent[idx*2] + tsize - 1;
    this->RecursiveLoad(ren, property, input, subExtent);

    subExtent[idx*2] = subExtent[idx*2] + tsize;
    subExtent[idx*2 + 1] = extent[idx*2 + 1];
    this->RecursiveLoad(ren, property, input, subExtent);
    }
}

//----------------------------------------------------------------------------
// Load the texture and the geometry
void vtkOpenGLImageResliceMapper::Load(vtkRenderer *ren, vtkImageProperty *property)
{
  vtkImageResliceToColors *reslice = this->ImageReslice;
  vtkScalarsToColors *lookupTable = this->DefaultLookupTable;

  reslice->SetInput(this->GetInput());
  //reslice->OptimizationOff();
  reslice->GenerateStencilOutputOn();
  if (property)
    {
    double colorWindow = property->GetColorWindow();
    double colorLevel = property->GetColorLevel();
    if (property->GetLookupTable())
      {
      lookupTable = property->GetLookupTable();
      if (!property->GetUseLookupTableScalarRange())
        {
        lookupTable->SetRange(colorLevel - 0.5*colorWindow,
                              colorLevel + 0.5*colorWindow);
        }
      }
    else
      {
      lookupTable->SetRange(colorLevel - 0.5*colorWindow,
                            colorLevel + 0.5*colorWindow);
      }
    }
  else
    {
    lookupTable->SetRange(0, 255);
    }

  reslice->SetLookupTable(lookupTable);
  reslice->UpdateWholeExtent();

  this->RecursiveLoad(
    ren, property, reslice->GetOutput(), reslice->GetOutputExtent());
}


//----------------------------------------------------------------------------
// Do all the fancy math to set up the reslicing
void vtkOpenGLImageResliceMapper::BuildResliceInformation(vtkRenderer *ren)
{
  // This is where all the fun stuff happens.
  vtkMatrix4x4 *resliceMatrix = this->ResliceMatrix;
  vtkImageResliceToColors *reslice = this->ImageReslice;

  // Get the projection matrix
  double aspect = ren->GetTiledAspectRatio();
  vtkCamera *camera = ren->GetActiveCamera();
  vtkMatrix4x4 *viewMatrix = camera->GetViewTransformMatrix();
  vtkMatrix4x4 *projMatrix = camera->GetProjectionTransformMatrix(
                               aspect, 0, 1);
  double worldToView[16];
  vtkMatrix4x4::Multiply4x4(
    *projMatrix->Element, *viewMatrix->Element, worldToView);
  double viewToWorld[16];
  vtkMatrix4x4::Invert(worldToView, viewToWorld);

  // Get point/normal from camera
  if (this->UseFocalPointAsSlicePoint)
    {
    camera->GetFocalPoint(this->SlicePoint);
    }
  if (this->UseViewPlaneNormalAsSliceNormal)
    {
    camera->GetViewPlaneNormal(this->SliceNormal);
    }

  // The ResliceExtent is set to the renderer size
  int *size = ren->GetSize();
  int extent[6];
  extent[0] = 0;
  extent[1] = size[0] - 1;
  extent[2] = 0;
  extent[3] = size[1] - 1;
  extent[4] = 0;
  extent[5] = 0;

  // Clip the extent to the bounding rect of the image
  // after reslicing.
  double point[3];
  double normal[3];
  this->GetSlicePoint(point);
  this->GetSliceNormal(normal);

  // First, get the depth coord of "point"
  double worldPoint[4];
  worldPoint[0] = point[0];
  worldPoint[1] = point[1];
  worldPoint[2] = point[2];
  worldPoint[3] = 1.0;

  double viewPoint[4];
  vtkMatrix4x4::MultiplyPoint(worldToView, worldPoint, viewPoint);
  double d = viewPoint[2]/viewPoint[3];

  // Find world coords of the lower, left corner of the viewport,
  // which corresponds to the display point (0, 0)
  viewPoint[0] = -1.0;
  viewPoint[1] = -1.0;
  viewPoint[2] = d;
  viewPoint[3] = 1.0;

  vtkMatrix4x4::MultiplyPoint(viewToWorld, viewPoint, worldPoint);

  double cornerPoint[3];
  cornerPoint[0] = worldPoint[0]/worldPoint[3];
  cornerPoint[1] = worldPoint[1]/worldPoint[3];
  cornerPoint[2] = worldPoint[2]/worldPoint[3];

  // Find horizontal vector to lower, right corner of the viewport,
  // this corner corresponds to the display point (width-1, 0)
  viewPoint[0] = +1.0;
  viewPoint[1] = -1.0;
  viewPoint[2] = d;
  viewPoint[3] = 1.0;

  vtkMatrix4x4::MultiplyPoint(viewToWorld, viewPoint, worldPoint);

  double v1[3];
  v1[0] = worldPoint[0]/worldPoint[3] - cornerPoint[0];
  v1[1] = worldPoint[1]/worldPoint[3] - cornerPoint[1];
  v1[2] = worldPoint[2]/worldPoint[3] - cornerPoint[2];

  double l1 = sqrt(v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2]);
  v1[0] /= l1;
  v1[1] /= l1;
  v1[2] /= l1;

  // Find vertical vector to upper, left corner of the viewport,
  // this corner corresponds to the display point (0, height-1)
  viewPoint[0] = -1.0;
  viewPoint[1] = +1.0;
  viewPoint[2] = d;
  viewPoint[3] = 1.0;

  vtkMatrix4x4::MultiplyPoint(viewToWorld, viewPoint, worldPoint);

  double v2[3];
  v2[0] = worldPoint[0]/worldPoint[3] - cornerPoint[0];
  v2[1] = worldPoint[1]/worldPoint[3] - cornerPoint[1];
  v2[2] = worldPoint[2]/worldPoint[3] - cornerPoint[2];

  double l2 = sqrt(v2[0]*v2[0] + v2[1]*v2[1] + v2[2]*v2[2]);
  v2[0] /= l2;
  v2[1] /= l2;
  v2[2] /= l2;

  // Create a slice-to-world transform matrix
  // The columns are v1, v2, normal
  double dp = vtkMath::Dot(normal, point);
  vtkMatrix4x4 *sliceToWorld = this->SliceToWorldMatrix;

  sliceToWorld->Element[0][0] = v1[0];
  sliceToWorld->Element[1][0] = v1[1];
  sliceToWorld->Element[2][0] = v1[2];
  sliceToWorld->Element[3][0] = 0.0;

  sliceToWorld->Element[0][1] = v2[0];
  sliceToWorld->Element[1][1] = v2[1];
  sliceToWorld->Element[2][1] = v2[2];
  sliceToWorld->Element[3][1] = 0.0;

  sliceToWorld->Element[0][2] = normal[0];
  sliceToWorld->Element[1][2] = normal[1];
  sliceToWorld->Element[2][2] = normal[2];
  sliceToWorld->Element[3][2] = 0.0;

  sliceToWorld->Element[0][3] = -dp*normal[0];
  sliceToWorld->Element[1][3] = -dp*normal[1];
  sliceToWorld->Element[2][3] = dp-dp*normal[2];
  sliceToWorld->Element[3][3] = 1.0;

  // Create the reslice matrix by multiplying by the prop's matrix
  vtkMatrix4x4::Multiply4x4(
    this->WorldToDataMatrix, sliceToWorld, resliceMatrix);

  // Compute the reslice origin
  double origin[3];
  worldPoint[0] = cornerPoint[0] + dp*normal[0];
  worldPoint[1] = cornerPoint[1] + dp*normal[1];
  worldPoint[2] = cornerPoint[2] - dp + dp*normal[2];
  origin[0] = vtkMath::Dot(v1, worldPoint);
  origin[1] = vtkMath::Dot(v2, worldPoint);
  origin[2] = vtkMath::Dot(normal, worldPoint);

  // Find the spacing
  double spacing[3];
  spacing[0] = 1.0;
  spacing[1] = 1.0;
  spacing[2] = 1.0;
  if (size[0] > 1)
    {
    spacing[0] = l1/(size[0] - 1);
    }
  if (size[1] > 1)
    {
    spacing[1] = l2/(size[1] - 1);
    }

  // For hardware-accelerated zoom (linear or no interpolation,
  // no oblique angles), directly use the permuted extent, spacing,
  // and origin of the input image.

  // The ResliceAxes are set to the actor matrix.
  // Potentially the ResliceAxes could also include
  // camera perspective for doing DRRs and MIPs

  reslice->SetResliceAxes(resliceMatrix);
  reslice->SetOutputExtent(extent);
  reslice->SetOutputSpacing(spacing);
  reslice->SetOutputOrigin(origin);
}

//----------------------------------------------------------------------------
// Set the modelview transform and load the texture
void vtkOpenGLImageResliceMapper::Render(vtkRenderer *ren, vtkImage *prop)
{
  vtkImageProperty *property = prop->GetProperty();

  // copy the matrix, but only if it has changed (we do this to
  // preserve the modified time of the matrix)
  double tmpmat[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
  if (!prop->GetIsIdentity())
    {
    this->WorldToDataMatrix->Invert(*prop->GetMatrix()->Element, tmpmat);
    }
  double *mat = *this->WorldToDataMatrix->Element;
  for (int i = 0; i < 16; i++)
    {
    if (mat[i] != tmpmat[i])
      {
      this->WorldToDataMatrix->DeepCopy(tmpmat);
      break;
      }
    }

  // set the reslice spacing/origin/extent/axes
  this->BuildResliceInformation(ren);

  // set the interpoltion mode
  if (property)
    {
    int interpMode = VTK_RESLICE_NEAREST;
    switch(property->GetInterpolationType())
      {
      case VTK_NEAREST_INTERPOLATION:
        interpMode = VTK_RESLICE_NEAREST;
        break;
      case VTK_LINEAR_INTERPOLATION:
        interpMode = VTK_RESLICE_LINEAR;
        break;
      case VTK_CUBIC_INTERPOLATION:
        interpMode = VTK_RESLICE_CUBIC;
        break;
      case VTK_LANCZOS_INTERPOLATION:
        interpMode = VTK_RESLICE_LANCZOS;
        break;
      }
    this->ImageReslice->SetInterpolationMode(interpMode);
    }

  glPushAttrib(GL_ENABLE_BIT);

  // for picking
  glDepthMask(GL_TRUE);

  // build transformation
  //if (!prop->GetIsIdentity())
    {
    // transpose VTK matrix to create OpenGL matrix
    double mat[16];
    vtkMatrix4x4::Transpose(*this->SliceToWorldMatrix->Element, mat);

    // insert model transformation
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd(mat);
    }

  // render the texture
  this->Load(ren, property);

  // pop transformation matrix
  //if (!prop->GetIsIdentity())
    {
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    }

  glPopAttrib();
}

//----------------------------------------------------------------------------
//int vtkOpenGLImageResliceMapper::IsMatrixOblique(vtkMatrix4x4 *matrix)
//{
  // Don't worry about in-plane rotation, just out-of-plane rotation.
//}

//----------------------------------------------------------------------------
void vtkOpenGLImageResliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkOpenGLImageResliceMapper::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->ImageReslice, "ImageReslice");
}
