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
// .NAME vtkImageMapper3D - abstract class for mapping images to the screen
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

class VTK_RENDERING_EXPORT vtkImageMapper3D : public vtkAbstractMapper3D
{
public:
  vtkTypeMacro(vtkImageMapper3D,vtkAbstractMapper3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This should only be called by the renderer.
  virtual void Render(vtkRenderer *renderer, vtkImage *prop) = 0;

  // Description:
  // Release any graphics resources that are being consumed by
  // this mapper.  The parameter window is used to determine
  // which graphic resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) = 0;

  // Description:
  // The input data for this mapper.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  vtkDataSet *GetDataSetInput();
  vtkDataObject *GetDataObjectInput();

protected:
  vtkImageMapper3D();
  ~vtkImageMapper3D();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkLookupTable *DefaultLookupTable;

private:
  vtkImageMapper3D(const vtkImageMapper3D&);  // Not implemented.
  void operator=(const vtkImageMapper3D&);  // Not implemented.
};

#endif
