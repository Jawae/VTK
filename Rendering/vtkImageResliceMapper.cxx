/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageResliceMapper.h"

#include "vtkRenderer.h"
#include "vtkImage.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkGraphicsFactory.h"

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkImageResliceMapper);

//----------------------------------------------------------------------------
vtkImageResliceMapper* vtkImageResliceMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkImageResliceMapper");
  return static_cast<vtkImageResliceMapper *>(ret);
}

//----------------------------------------------------------------------------
vtkImageResliceMapper::vtkImageResliceMapper()
{
  this->UseFocalPointAsSlicePoint = 1;
  this->UseViewPlaneNormalAsSliceNormal = 1;

  this->SlicePoint[0] = 0.0;
  this->SlicePoint[1] = 0.0;
  this->SlicePoint[2] = 0.0;

  this->SliceNormal[0] = 0.0;
  this->SliceNormal[1] = 0.0;
  this->SliceNormal[2] = 1.0;
}

//----------------------------------------------------------------------------
vtkImageResliceMapper::~vtkImageResliceMapper()
{
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::ReleaseGraphicsResources(vtkWindow *renWin)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::Render(vtkRenderer *ren, vtkImage *image)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageResliceMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
unsigned long vtkImageResliceMapper::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();

  return mTime;
}

//----------------------------------------------------------------------------
double *vtkImageResliceMapper::GetBounds()
{
  // Modify to give just the slice bounds
  if (!this->GetInput())
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
    }
  else
    {
    vtkImageData *input = this->GetInput();
    input->UpdateInformation();
    double *spacing = input->GetSpacing();
    double *origin = input->GetOrigin();
    int *extent = input->GetWholeExtent();

    int swapXBounds = (spacing[0] < 0);  // 1 if true, 0 if false
    int swapYBounds = (spacing[1] < 0);  // 1 if true, 0 if false
    int swapZBounds = (spacing[2] < 0);  // 1 if true, 0 if false

    this->Bounds[0] = origin[0] + (extent[0+swapXBounds] * spacing[0]);
    this->Bounds[2] = origin[1] + (extent[2+swapYBounds] * spacing[1]);
    this->Bounds[4] = origin[2] + (extent[4+swapZBounds] * spacing[2]);

    this->Bounds[1] = origin[0] + (extent[1-swapXBounds] * spacing[0]);
    this->Bounds[3] = origin[1] + (extent[3-swapYBounds] * spacing[1]);
    this->Bounds[5] = origin[2] + (extent[5-swapZBounds] * spacing[2]);

    return this->Bounds;
    }
}
