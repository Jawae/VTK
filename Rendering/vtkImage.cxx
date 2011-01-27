/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImage.h"

#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"
#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkLinearTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

#include <math.h>

vtkStandardNewMacro(vtkImage);

//----------------------------------------------------------------------------
// Creates a Volume with the following defaults: origin(0,0,0)
// position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
vtkImage::vtkImage()
{
  this->Mapper = NULL;
  this->Property = NULL;
}

//----------------------------------------------------------------------------
// Destruct a volume
vtkImage::~vtkImage()
{
  if (this->Property)
    {
    this->Property->UnRegister(this);
    }

  this->SetMapper(NULL);
}

//----------------------------------------------------------------------------
void vtkImage::GetImages(vtkPropCollection *vc)
{
  vc->AddItem(this);
}

//----------------------------------------------------------------------------
// Shallow copy of an volume.
void vtkImage::ShallowCopy(vtkProp *prop)
{
  vtkImage *v = vtkImage::SafeDownCast(prop);

  if (v != NULL)
    {
    this->SetMapper(v->GetMapper());
    this->SetProperty(v->GetProperty());
    }

  // Now do superclass
  this->vtkProp3D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkImage::SetMapper(vtkImageMapper3D *mapper)
{
  if (this->Mapper != mapper)
    {
    if (this->Mapper != NULL)
      {
      this->Mapper->UnRegister(this);
      }
    this->Mapper = mapper;
    if (this->Mapper != NULL)
      {
      this->Mapper->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double *vtkImage::GetBounds()
{
  int i,n;
  double *bounds, bbox[24], *fptr;

  // get the bounds of the Mapper if we have one
  if (!this->Mapper)
    {
    return this->Bounds;
    }

  bounds = this->Mapper->GetBounds();
  // Check for the special case when the mapper's bounds are unknown
  if (!bounds)
    {
    return bounds;
    }

  // fill out vertices of a bounding box
  bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
  bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
  bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
  bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
  bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
  bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
  bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
  bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];

  // make sure matrix (transform) is up-to-date
  this->ComputeMatrix();

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++)
    {
    double homogeneousPt[4] = {fptr[0], fptr[1], fptr[2], 1.0};
    this->Matrix->MultiplyPoint(homogeneousPt, homogeneousPt);
    fptr[0] = homogeneousPt[0] / homogeneousPt[3];
    fptr[1] = homogeneousPt[1] / homogeneousPt[3];
    fptr[2] = homogeneousPt[2] / homogeneousPt[3];
    fptr += 3;
    }

  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  for (i = 0; i < 8; i++)
    {
    for (n = 0; n < 3; n++)
      {
      if (bbox[i*3+n] < this->Bounds[n*2])
        {
        this->Bounds[n*2] = bbox[i*3+n];
        }
      if (bbox[i*3+n] > this->Bounds[n*2+1])
        {
        this->Bounds[n*2+1] = bbox[i*3+n];
        }
      }
    }

  return this->Bounds;
}

//----------------------------------------------------------------------------
// Get the minimum X bound
double vtkImage::GetMinXBound()
{
  this->GetBounds();
  return this->Bounds[0];
}

// Get the maximum X bound
double vtkImage::GetMaxXBound()
{
  this->GetBounds();
  return this->Bounds[1];
}

// Get the minimum Y bound
double vtkImage::GetMinYBound()
{
  this->GetBounds();
  return this->Bounds[2];
}

// Get the maximum Y bound
double vtkImage::GetMaxYBound()
{
  this->GetBounds();
  return this->Bounds[3];
}

// Get the minimum Z bound
double vtkImage::GetMinZBound()
{
  this->GetBounds();
  return this->Bounds[4];
}

// Get the maximum Z bound
double vtkImage::GetMaxZBound()
{
  this->GetBounds();
  return this->Bounds[5];
}

//----------------------------------------------------------------------------
int vtkImage::GetIsOpaque()
{
  /*
  vtkImageMapper3D *mapper = this->GetMapper();
  vtkImageProperty *property = this->GetProperty();
  if (mapper && property)
    {
    if (property->GetOpacity() < 1.0)
      {
      return 1;
      }
    }
  */
  return 0;
}

//----------------------------------------------------------------------------
// Does this prop have some translucent polygonal geometry?
int vtkImage::HasTranslucentPolygonalGeometry()
{
  return !this->GetIsOpaque();
}

//----------------------------------------------------------------------------
int vtkImage::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImage::RenderTranslucentPolygonalGeometry");

  if (!this->GetIsOpaque())
    {
    this->Render(vtkRenderer::SafeDownCast(viewport));
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkImage::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImage::RenderOpaqueGeometry");

  if (this->GetIsOpaque())
    {
    this->Render(vtkRenderer::SafeDownCast(viewport));
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkImage::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkImage::RenderOverlay");

  // Render the image as an underlay

  return 0;
}

//----------------------------------------------------------------------------
void vtkImage::Render(vtkRenderer *ren)
{
  this->Update();

  if (!this->Mapper)
    {
    vtkErrorMacro( << "You must specify a mapper!\n" );
    return;
    }

  // If we don't have any input return silently
  if (!this->Mapper->GetInput())
    {
    return;
    }

  // Force the creation of a property
  if (!this->Property)
    {
    this->GetProperty();
    }

  if (!this->Property)
    {
    vtkErrorMacro( << "Error generating a property!\n" );
    return;
    }

  this->Mapper->Render(ren, this);
  this->EstimatedRenderTime += this->Mapper->GetTimeToDraw();
}

//----------------------------------------------------------------------------
void vtkImage::ReleaseGraphicsResources(vtkWindow *win)
{
  // pass this information onto the mapper
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(win);
    }
}

//----------------------------------------------------------------------------
void vtkImage::Update()
{
  if (this->Mapper)
    {
    this->Mapper->Update();
    }
}

//----------------------------------------------------------------------------
void vtkImage::SetProperty(vtkImageProperty *property)
{
  if (this->Property != property)
    {
    if (this->Property != NULL)
      {
      this->Property->UnRegister(this);
      }
    this->Property = property;
    if (this->Property != NULL)
      {
      this->Property->Register(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkImageProperty *vtkImage::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkImageProperty::New();
    this->Property->Register(this);
    this->Property->Delete();
    }
  return this->Property;
}

//----------------------------------------------------------------------------
unsigned long int vtkImage::GetMTime()
{
  unsigned long mTime = this->vtkObject::GetMTime();
  unsigned long time;

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserMatrix != NULL )
    {
    time = this->UserMatrix->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserTransform != NULL )
    {
    time = this->UserTransform->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
unsigned long vtkImage::GetRedrawMTime()
{
  unsigned long mTime = this->GetMTime();
  unsigned long time;

  if ( this->Mapper != NULL )
    {
    time = this->Mapper->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->GetMapper()->GetInput() != NULL)
      {
      this->GetMapper()->GetInput()->Update();
      time = this->Mapper->GetInput()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );

    if ( this->Property->GetLookupTable() != NULL )
      {
      // check the lookup table mtime
      time = this->Property->GetLookupTable()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (not defined)\n";
    }

  if( this->Mapper )
    {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Mapper: (not defined)\n";
    }

  // make sure our bounds are up to date
  if ( this->Mapper )
    {
    this->GetBounds();
    os << indent << "Bounds: (" << this->Bounds[0] << ", "
       << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
       << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
       << this->Bounds[5] << ")\n";
    }
  else
    {
    os << indent << "Bounds: (not defined)\n";
    }
}
