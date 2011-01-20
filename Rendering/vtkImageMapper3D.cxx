/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMapper3D.h"

#include "vtkRenderer.h"
#include "vtkImage.h"
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkMath.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkGraphicsFactory.h"

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkImageMapper3D);

//----------------------------------------------------------------------------
vtkImageMapper3D* vtkImageMapper3D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkImageMapper3D");
  return static_cast<vtkImageMapper3D *>(ret);
}

//----------------------------------------------------------------------------
vtkImageMapper3D::vtkImageMapper3D()
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
vtkImageMapper3D::~vtkImageMapper3D()
{
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::SetInput(vtkImageData *input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    // Setting a NULL input removes the connection.
    this->SetInputConnection(0, 0);
    }
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMapper3D::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::ReleaseGraphicsResources(vtkWindow *renWin)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::Render(vtkRenderer *ren, vtkImage *image)
{
  // see OpenGL subclass for implementation
}

//----------------------------------------------------------------------------
void vtkImageMapper3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
unsigned long vtkImageMapper3D::GetMTime()
{
  unsigned long mTime = this->Superclass::GetMTime();

  return mTime;
}

//----------------------------------------------------------------------------
double *vtkImageMapper3D::GetBounds()
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

//----------------------------------------------------------------------------
vtkDataObject *vtkImageMapper3D::GetDataObjectInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return this->GetInputDataObject(0, 0);
}

//----------------------------------------------------------------------------
vtkDataSet *vtkImageMapper3D::GetDataSetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkDataSet::SafeDownCast(this->GetInputDataObject(0, 0));
}

//----------------------------------------------------------------------------
int vtkImageMapper3D::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
