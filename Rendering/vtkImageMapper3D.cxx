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
#include "vtkImageData.h"
#include "vtkImageProperty.h"
#include "vtkLookupTable.h"
#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkGraphicsFactory.h"

//----------------------------------------------------------------------------
vtkImageMapper3D::vtkImageMapper3D()
{
  // Build a default greyscale lookup table
  this->DefaultLookupTable = vtkLookupTable::New();
  this->DefaultLookupTable->SetRampToLinear();
  this->DefaultLookupTable->SetValueRange(0.0, 1.0);
  this->DefaultLookupTable->SetSaturationRange(0.0, 0.0);
  this->DefaultLookupTable->SetAlphaRange(1.0, 1.0);
  this->DefaultLookupTable->Build();
  this->DefaultLookupTable->SetVectorModeToColors();
}

//----------------------------------------------------------------------------
vtkImageMapper3D::~vtkImageMapper3D()
{
  if (this->DefaultLookupTable)
    {
    this->DefaultLookupTable->Delete();
    }
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
