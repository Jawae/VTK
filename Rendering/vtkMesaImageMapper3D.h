/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaImageMapper3D - OpenGL mapper for image slice display
// .SECTION Description
// vtkMesaImageMapper3D is a concrete implementation of the abstract
// class vtkImageMapper3D that interfaces to the Mesa rendering library.
// Depending on the operations that are being performed, it will either
// do the rendering completely on the GPU, or will use a combination of
// CPU and GPU computations.  The CPU is used for operations like oblique
// slice extraction and cubic interpolation.

#ifndef __vtkMesaImageMapper3D_h
#define __vtkMesaImageMapper3D_h

#include "vtkImageMapper3D.h"

class vtkWindow;
class vtkRenderer;
class vtkRenderWindow;
class vtkImage;
class vtkImageProperty;
class vtkImageData;
class vtkImageReslice;
class vtkMatrix4x4;

class VTK_RENDERING_EXPORT vtkMesaImageMapper3D : public vtkImageMapper3D
{
public:
  static vtkMesaImageMapper3D *New();
  vtkTypeMacro(vtkMesaImageMapper3D,vtkImageMapper3D);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.  Load the texture and geometry.
  void Load(vtkRenderer *ren, vtkImageProperty *property);

  // Description:
  // Implement base class method.  Perform the render.
  void Render(vtkRenderer *ren, vtkImage *prop);

  // Description:
  // Release any graphics resources that are being consumed by this
  // mapper, the image texture in particular. Using the same texture
  // in multiple render windows is NOT currently supported.
  void ReleaseGraphicsResources(vtkWindow *);

protected:
  vtkMesaImageMapper3D();
  ~vtkMesaImageMapper3D();

  // Description:
  // Non-recursive internal method, generate a single texture
  // and its corresponding geometry.
  void InternalLoad(
    vtkRenderer *ren, vtkImageProperty *property, vtkImageData *image,
    int extent[6]);

  // Description:
  // Recursive internal method, will call the non-recursive method
  // as many times as necessary if the texture must be broken up into
  // pieces that are small enough for the GPU to render
  void RecursiveLoad(
    vtkRenderer *ren, vtkImageProperty *property, vtkImageData *image,
    int extent[6]);

  // Description:
  // Given an extent that describes a slice (i.e. unit thickness in
  // one of the directions), return the dimension indices that correspond
  // to the texture "x" and "y", provide the x, y image size, and
  // provide the texture size (padded to a power of two).
  void ComputeTextureSize(
    const int extent[6], int &xdim, int &ydim,
    int imageSize[2], int textureSize[2]);

  // Description:
  // Test whether a given texture size is supported.  This includes a
  // check of whether the texture will fit into texture memory.
  int TextureSizeOK(const int size[2]);

  // Description:
  // Perform window/level and color mapping operations to produce
  // unsigned char data that can be used as a texture.  This method
  // also sets the Coords and TCoords.
  unsigned char *MakeTextureData(
    vtkImageProperty *property, vtkImageData *input, int extent[6],
    int &xsize, int &ysize, int &bytesPerPixel,
    int &release, int &reuseTexture);

  // Description:
  // Build the reslice information.
  void BuildResliceInformation(vtkRenderer *ren);

  // Description:
  // Garbage collection for reference loops.
  void ReportReferences(vtkGarbageCollector*);

  vtkTimeStamp LoadTime;
  long Index; // OpenGL ID for texture or display list
  vtkRenderWindow *RenderWindow; // RenderWindow used for previous render
  vtkImageReslice *ImageReslice; // For software interpolation
  vtkMatrix4x4 *ResliceMatrix;
  vtkMatrix4x4 *WorldToDataMatrix; // Data to World transform matrix
  vtkMatrix4x4 *SliceToWorldMatrix; // Slice to World transform matrix
  double Coords[12];
  double TCoords[8];
  int TextureSize[2];
  int TextureBytesPerPixel;

private:
  vtkMesaImageMapper3D(const vtkMesaImageMapper3D&);  // Not implemented.
  void operator=(const vtkMesaImageMapper3D&);  // Not implemented.
};

#endif
