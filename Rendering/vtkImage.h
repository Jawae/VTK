/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImage - represents an image in a 3D scene
//
// .SECTION Description
// vtkImage is used to represent an image in a 3D scene.  It displays
// the image either as a slice or as a projection from the camera's
// perspective. Adjusting the position and orientation of the slice
// is done by adjusting the focal point and direction of the camera,
// or alternatively the slice can be set manually in vtkImageMapper3D.
// The lookup table and window/leve are set in vtkImageProperty.
// Prop3D methods such as SetPosition() and RotateWXYZ() change the
// position and orientation of the data with respect to VTK world
// coordinates.
// .SECTION see also
// vtkImageMapper3D vtkImageProperty vtkProp3D

#ifndef __vtkImage_h
#define __vtkImage_h

#include "vtkProp3D.h"

class vtkRenderer;
class vtkPropCollection;
class vtkImageProperty;
class vtkImageMapper3D;

class VTK_RENDERING_EXPORT vtkImage : public vtkProp3D
{
public:
  vtkTypeMacro(vtkImage,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates an Image with the following defaults: origin(0,0,0)
  // position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0).
  static vtkImage *New();

  // Description:
  // Set/Get the mapper.
  void SetMapper(vtkImageMapper3D *mapper);
  vtkGetObjectMacro(Mapper, vtkImageMapper3D);

  // Description:
  // Set/Get the image display properties.
  void SetProperty(vtkImageProperty *property);
  vtkImageProperty *GetProperty();

  // Description:
  // Update the rendering pipeline by updating the ImageMapper
  void Update();

  // Description:
  // Get the bounds - either all six at once
  // (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
  double *GetBounds();
  void GetBounds(double bounds[6]) { this->vtkProp3D::GetBounds( bounds ); };
  double GetMinXBound();
  double GetMaxXBound();
  double GetMinYBound();
  double GetMaxYBound();
  double GetMinZBound();
  double GetMaxZBound();

  // Description:
  // Return the MTime also considering the property etc.
  unsigned long int GetMTime();

  // Description:
  // Return the mtime of anything that would cause the rendered image to
  // appear differently. Usually this involves checking the mtime of the
  // prop plus anything else it depends on such as properties, mappers,
  // etc.
  unsigned long GetRedrawMTime();

  // Description:
  // Shallow copy of this vtkImage. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // For some exporters and other other operations we must be
  // able to collect all the actors, volumes, and images. These
  // methods are used in that process.
  void GetImages(vtkPropCollection *);

  // Description:
  // Support the standard render methods.
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // This causes the image and its mapper to be rendered. Note that a side
  // effect of this method is that the pipeline will be updated.
  virtual void Render(vtkRenderer *);

  // Description:
  // Release any resources held by this prop.
  void ReleaseGraphicsResources(vtkWindow *win);

protected:
  vtkImage();
  ~vtkImage();

  vtkImageMapper3D *Mapper;
  vtkImageProperty *Property;

  int GetIsOpaque();

private:
  vtkImage(const vtkImage&);  // Not implemented.
  void operator=(const vtkImage&);  // Not implemented.
};

#endif
