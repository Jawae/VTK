/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMapper3D - map a slice of a vtkImageData to the screen
// .SECTION Description
// vtkImageMapper3D is a mapper that will draw a 2D image, or a slice
// of a 3D image.  By default, the slice plane will be set automatically
// according to camera focal point and orientation, i.e. the slice
// will cut through the input image at the focal point, and the slice
// normal will point towards the camera.

#ifndef __vtkImageMapper3D_h
#define __vtkImageMapper3D_h

#include "vtkAbstractMapper3D.h"

class vtkRenderer;
class vtkLookupTable;
class vtkImage;
class vtkImageData;

// What about the texture?
// 1) always the size of the image
// 2) always the size of the window in display coords
// 3) always the size of the image projected to display coords
// 4) automatic switching as needed

// Auto-switch with these criteria:
// 1) use image->texture if
//  a) slice is not oblique
//  b) interpolation is linear or nearest neighbor
//  c) image is smaller than a certain max size
// 2) otherwise use the image projected to display coords,
//  then clipped to the window bounds

// Like vtkDataSetMapper, this will have it own internal pipeline:
// 1) vtkImageReslice (the workhorse)
// 2) vtkImageProjection (optional... average, MIP, project)
// 3) map-to-colors or window/level (optional)
// 4) rendering code stolen from vtkImageActor

// Should vtkImageReslice ever be optional?  (Performance)
// Yes, since there is already good code in vtkImageActor for
// directly using an x, y, or z slice as a texture.

// Should the Z interpolation mode be controllable independent of
// the X, Y interpolation mode?  The idea here is that sometimes
// people like to lock the Z interpolation to nearest-neighbor
// while using linear or cubic interpolation in X, Y.  It is
// probably better to have a snap-to-slice mode instead.

// There are three transforms to keep track of:
// 1. WorldToData (the inverse of the vtkProp3D matrix)
// 2. SliceToWorld (which will usually follow the camera)
// 3. DisplayToSlice (convert from pixel coords to slice-view coords)
// In general, 1 and 2 will be concatenated to create the ResliceAxes
// matrix, while 3 will be handled by setting the reslice spacing and
// origin.
// For performance, care must be taken when building the ResliceAxes
// matrix that roundoff error does not cause a permutation matrix to
// become slightly oblique.  I have code that can turn any matrix into
// a permutation matrix plus a residual, this code can be used to do
// the check.

// Clipping planes can be handled by applying them when the
// geometry is rendered.  For projection (MIPs and DRRs), the
// clipping planes must be made into a stencil... vtkImageProjection
// could easily be modified to use a stencil.  I could use my
// line/plane intersection code from vtkCellPicker to build the stencil.
// But that can wait... MIP and DRR support is still a ways off.

// Translucency and front-to-back ordering are going to be a pain.
// In most cases, all image rendering should be done during the
// "Opaque" pass, and care must be taken that images in the same
// plane always share identical point geometry for their polygons.

class VTK_RENDERING_EXPORT vtkImageMapper3D : public vtkAbstractMapper3D
{
public:
  static vtkImageMapper3D *New();
  vtkTypeMacro(vtkImageMapper3D,vtkAbstractMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If this is on, then the camera FocalPoint will be used
  // as the SlicePoint.  This is on by default.
  vtkSetMacro(UseFocalPointAsSlicePoint, int);
  vtkGetMacro(UseFocalPointAsSlicePoint, int);
  vtkBooleanMacro(UseFocalPointAsSlicePoint, int);

  // Description:
  // If this is on, then the camera ViewPlaneNormal will
  // be used as the SliceNormal.  This is on by default.
  vtkSetMacro(UseViewPlaneNormalAsSliceNormal, int);
  vtkGetMacro(UseViewPlaneNormalAsSliceNormal, int);
  vtkBooleanMacro(UseViewPlaneNormalAsSliceNormal, int);

  // Description:
  // The slice plane will always pass through this point,
  // which is in world coordinates rather than data coordinates.
  // If UseFocalPointAsSlicePoint is on, then this point
  // will automatically be set to the camera focal point.
  vtkSetVector3Macro(SlicePoint, double);
  vtkGetVector3Macro(SlicePoint, double);

  // Description:
  // The slice plane will always use this normal, which
  // is in world coordinates rather than data coordinates.
  // If UseViewPlaneNormalAsSliceNormal is on, then this
  // will automatically be set to the view plane normal.
  vtkSetVector3Macro(SliceNormal, double);
  vtkGetVector3Macro(SliceNormal, double);

  // Description:
  // This should only be called by the renderer.
  virtual void Render(vtkRenderer *renderer, vtkImage *prop);

  // Description:
  // Release any graphics resources that are being consumed by
  // this mapper.  The parameter window is used to determine
  // which graphic resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the mtime for the mapper.
  unsigned long GetMTime();

  // Description:
  // The input data for this mapper.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  vtkDataSet *GetDataSetInput();
  vtkDataObject *GetDataObjectInput();

  // Description:
  // The bounding box (array of six doubles) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  double *GetBounds();
  void GetBounds(double bounds[6])
    { this->vtkAbstractMapper3D::GetBounds(bounds); };

protected:
  vtkImageMapper3D();
  ~vtkImageMapper3D();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  double SlicePoint[3];
  double SliceNormal[3];

  int UseFocalPointAsSlicePoint;
  int UseViewPlaneNormalAsSliceNormal;
  vtkLookupTable *DefaultLookupTable;

private:
  vtkImageMapper3D(const vtkImageMapper3D&);  // Not implemented.
  void operator=(const vtkImageMapper3D&);  // Not implemented.
};

#endif
