/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleImage.h"

#include "vtkAbstractPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkPropCollection.h"

#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImage.h";
#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"

vtkStandardNewMacro(vtkInteractorStyleImage);

//----------------------------------------------------------------------------
vtkInteractorStyleImage::vtkInteractorStyleImage() 
{
  this->WindowLevelStartPosition[0]   = 0;
  this->WindowLevelStartPosition[1]   = 0;  

  this->WindowLevelCurrentPosition[0] = 0;
  this->WindowLevelCurrentPosition[1] = 0;  

  this->WindowLevelInitial[0] = 1.0; // Window
  this->WindowLevelInitial[1] = 0.5; // Level

  this->WindowLevelProperty = 0;

  this->InteractionMode = VTKIS_IMAGE2D;

  this->XViewLeftToRight[0] = 0;
  this->XViewLeftToRight[1] = 1;
  this->XViewLeftToRight[2] = 0;

  this->XViewUp[0] = 0;
  this->XViewUp[1] = 0;
  this->XViewUp[2] = -1;

  this->YViewLeftToRight[0] = 1;
  this->YViewLeftToRight[1] = 0;
  this->YViewLeftToRight[2] = 0;

  this->YViewUp[0] = 0;
  this->YViewUp[1] = 0;
  this->YViewUp[2] = -1;

  this->ZViewLeftToRight[0] = 1;
  this->ZViewLeftToRight[1] = 0;
  this->ZViewLeftToRight[2] = 0;

  this->ZViewUp[0] = 0;
  this->ZViewUp[1] = 1;
  this->ZViewUp[2] = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleImage::~vtkInteractorStyleImage() 
{
  if (this->WindowLevelProperty)
    {
    this->WindowLevelProperty->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartWindowLevel() 
{
  if (this->State != VTKIS_NONE) 
    {
    return;
    }
  this->StartState(VTKIS_WINDOW_LEVEL);
  if (this->HandleObservers &&
      this->HasObserver(vtkCommand::StartWindowLevelEvent))
    {
    this->InvokeEvent(vtkCommand::StartWindowLevelEvent, this);
    }
  else
    {
    if (this->CurrentRenderer)
      {
      // search for a vtkImage object and get its property, this
      // is a stopgap measure until image picking is fully enabled
      vtkPropCollection *props = this->CurrentRenderer->GetViewProps();
      vtkProp *prop = 0;
      vtkAssemblyPath *path;
      vtkImage *imageProp = 0;
      vtkCollectionSimpleIterator pit;
      for (props->InitTraversal(pit); (prop = props->GetNextProp(pit)); )
        {
        for (prop->InitPathTraversal(); (path = prop->GetNextPath()); )
          {
          vtkProp *tryProp = path->GetLastNode()->GetViewProp();
          if (tryProp->IsA("vtkImage"))
            {
            imageProp = static_cast<vtkImage *>(tryProp);
            }
          }
        }

      if (imageProp)
        {
        vtkImageProperty *property = imageProp->GetProperty();
        if (property)
          {
          if (this->WindowLevelProperty)
            {
            this->WindowLevelProperty->Delete();
            }

          this->WindowLevelProperty = property;
          this->WindowLevelProperty->Register(this);

          this->WindowLevelInitial[0] = property->GetColorWindow();
          this->WindowLevelInitial[1] = property->GetColorLevel();
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndWindowLevel() 
{
  if (this->State != VTKIS_WINDOW_LEVEL) 
    {
    return;
    }
  if (this->HandleObservers)
    {
    this->InvokeEvent(vtkCommand::EndWindowLevelEvent, this);
    }
  if (this->WindowLevelProperty)
    {
    this->WindowLevelProperty->Delete();
    this->WindowLevelProperty = NULL;
    }
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartPick() 
{
  if (this->State != VTKIS_NONE) 
    {
    return;
    }
  this->StartState(VTKIS_PICK);
  if (this->HandleObservers)
    {
    this->InvokeEvent(vtkCommand::StartPickEvent, this);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndPick() 
{
  if (this->State != VTKIS_PICK) 
    {
    return;
    }
  if (this->HandleObservers)
    {
    this->InvokeEvent(vtkCommand::EndPickEvent, this);
    }
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartSlice()
{
  if (this->State != VTKIS_NONE)
    {
    return;
    }
  this->StartState(VTKIS_SLICE);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndSlice()
{
  if (this->State != VTKIS_SLICE)
    {
    return;
    }
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMouseMove() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State) 
    {
    case VTKIS_WINDOW_LEVEL:
      this->FindPokedRenderer(x, y);
      this->WindowLevel();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_PICK:
      this->FindPokedRenderer(x, y);
      this->Pick();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_SLICE:
      this->FindPokedRenderer(x, y);
      this->Slice();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  // Redefine this button to handle window/level
  this->GrabFocus(this->EventCallbackCommand);
  if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey()) 
    {
    this->WindowLevelStartPosition[0] = x;
    this->WindowLevelStartPosition[1] = y;      
    this->StartWindowLevel();
    }

  // If shift is held down, do a rotation
  else if (this->InteractionMode == VTKIS_IMAGE3D &&
           this->Interactor->GetShiftKey())
    {
    this->StartRotate();
    }

  // The rest of the button + key combinations remain the same

  else
    {
    this->Superclass::OnLeftButtonDown();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_WINDOW_LEVEL:
      this->EndWindowLevel();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;

    case VTKIS_SLICE:
      this->EndSlice();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }
  
  // Call parent to handle all other states and perform additional work

  this->Superclass::OnLeftButtonUp();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMiddleButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // If shift is held down, change the slice
  if (this->InteractionMode == VTKIS_IMAGE3D &&
      this->Interactor->GetShiftKey())
    {
    this->StartSlice();
    }

   // The rest of the button + key combinations remain the same

  else
    {
    this->Superclass::OnMiddleButtonDown();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMiddleButtonUp()
{
  switch (this->State)
    {
    case VTKIS_SLICE:
      this->EndSlice();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMiddleButtonUp();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Redefine this button + shift to handle pick
  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetShiftKey())
    {
    this->StartPick();
    }

  else if (this->InteractionMode == VTKIS_IMAGE3D &&
           this->Interactor->GetControlKey())
    {
    this->StartSlice();
    }

  // The rest of the button + key combinations remain the same

  else
    {
    this->Superclass::OnRightButtonDown();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonUp() 
{
  switch (this->State) 
    {
    case VTKIS_PICK:
      this->EndPick();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;

    case VTKIS_SLICE:
      this->EndSlice();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnRightButtonUp();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnChar() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode()) 
    {
    case 'f' :      
    case 'F' :
      {
      this->AnimState = VTKIS_ANIM_ON;
      vtkAssemblyPath *path=NULL;
      this->FindPokedRenderer(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1]);
      rwi->GetPicker()->Pick(rwi->GetEventPosition()[0],
                             rwi->GetEventPosition()[1], 0.0, 
                             this->CurrentRenderer);
      vtkAbstractPropPicker *picker;
      if ( (picker=vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker())) )
        {
        path = picker->GetPath();
        }
      if ( path != NULL )
        {
        rwi->FlyToImage(this->CurrentRenderer,picker->GetPickPosition());
        }
      this->AnimState = VTKIS_ANIM_OFF;
      break;
      }

    case 'r' :      
    case 'R' :
      // Allow either shift/ctrl to trigger the usual 'r' binding
      // otherwise trigger reset window level event
      if (rwi->GetShiftKey() || rwi->GetControlKey())
        {
        this->Superclass::OnChar();
        }
      else
        {
          this->InvokeEvent(vtkCommand::ResetWindowLevelEvent, this);
        }
      break;

    case 'x' :
    case 'X' :
      {
      this->SetImageOrientation(this->XViewLeftToRight, this->XViewUp);
      this->Interactor->Render();
      }
      break;

    case 'y' :
    case 'Y' :
      {
      this->SetImageOrientation(this->YViewLeftToRight, this->YViewUp);
      this->Interactor->Render();
      }
      break;

    case 'z' :
    case 'Z' :
      {
      this->SetImageOrientation(this->ZViewLeftToRight, this->ZViewUp);
      this->Interactor->Render();
      }
      break;

    default:
      this->Superclass::OnChar();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::WindowLevel()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  this->WindowLevelCurrentPosition[0] = rwi->GetEventPosition()[0];
  this->WindowLevelCurrentPosition[1] = rwi->GetEventPosition()[1];
  
  if (this->WindowLevelProperty)
    {
    int *size = this->CurrentRenderer->GetSize();

    double window = this->WindowLevelInitial[0];
    double level = this->WindowLevelInitial[1];

    // Compute normalized delta

    double dx = (this->WindowLevelCurrentPosition[0] -
                 this->WindowLevelStartPosition[0]) * 4.0 / size[0];
    double dy = (this->WindowLevelStartPosition[1] -
                 this->WindowLevelCurrentPosition[1]) * 4.0 / size[1];

    // Scale by current values

    if ( fabs( window ) > 0.01 )
      {
      dx = dx * window;
      }
    else
      {
      dx = dx * ( window < 0 ? -0.01 : 0.01 );
      }
    if ( fabs( level ) > 0.01 )
      {
      dy = dy * level;
      }
    else
      {
      dy = dy * ( level < 0 ? -0.01 : 0.01 );
      }

    // Abs so that direction does not flip

    if ( window < 0.0 )
      {
      dx = -1 * dx;
      }
    if ( level < 0.0 )
      {
      dy = -1 * dy;
      }

    // Compute new window level

    double newWindow = dx + window;
    double newLevel = level - dy;

    if ( newWindow < 0.01 )
      {
      newWindow = 0.01;
      }

    this->WindowLevelProperty->SetColorWindow(newWindow);
    this->WindowLevelProperty->SetColorLevel(newLevel);

    this->Interactor->Render();
    }
  else
    {
    this->InvokeEvent(vtkCommand::WindowLevelEvent, this);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::Pick()
{
  this->InvokeEvent(vtkCommand::PickEvent, this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::Slice()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  double *range = camera->GetClippingRange();
  double distance = camera->GetDistance();

  // scale the interaction by the height of the viewport
  double viewportHeight = 0.0;
  if (camera->GetParallelProjection())
    {
    viewportHeight = camera->GetParallelScale();
    }
  else
    {
    double angle = vtkMath::RadiansFromDegrees(camera->GetViewAngle());
    viewportHeight = 2.0*distance*tan(0.5*angle);
    }

  int *size = this->CurrentRenderer->GetSize();
  double delta = dy*viewportHeight/size[1];
  distance += delta;

  // clamp the distance to the clipping range
  if (distance < range[0])
    {
    distance = range[0] + viewportHeight*1e-3;
    }
  if (distance > range[1])
    {
    distance = range[1] - viewportHeight*1e-3;
    }
  camera->SetDistance(distance);

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::SetImageOrientation(
  const double leftToRight[3], const double viewUp[3])
{
  if (this->CurrentRenderer)
    {
    double vector[3];
    vtkMath::Cross(leftToRight, viewUp, vector);
    double focus[3];
    vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
    camera->GetFocalPoint(focus);
    double d = camera->GetDistance();
    camera->SetPosition(focus[0] - d*vector[0],
                        focus[1] - d*vector[1],
                        focus[2] - d*vector[2]);
    camera->SetFocalPoint(focus);
    camera->SetViewUp(viewUp);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Window Level Current Position: ("
     << this->WindowLevelCurrentPosition[0] << ", "
     << this->WindowLevelCurrentPosition[1] << ")\n";

  os << indent << "Window Level Start Position: ("
     << this->WindowLevelStartPosition[0] << ", "
     << this->WindowLevelStartPosition[1] << ")\n";

  os << indent << "Interaction Mode: " <<
     ((this->InteractionMode == VTKIS_IMAGE3D) ? "Image3D\n" : "Image2D\n");

  os << indent << "X View Left To Right: ("
     << this->XViewLeftToRight[0] << ", "
     << this->XViewLeftToRight[1] << ", "
     << this->XViewLeftToRight[2] << ")\n";

  os << indent << "X View Up: ("
     << this->XViewUp[0] << ", "
     << this->XViewUp[1] << ", "
     << this->XViewUp[2] << ")\n";

  os << indent << "Y View Left To Right: ("
     << this->YViewLeftToRight[0] << ", "
     << this->YViewLeftToRight[1] << ", "
     << this->YViewLeftToRight[2] << ")\n";

  os << indent << "Y View Up: ("
     << this->YViewUp[0] << ", "
     << this->YViewUp[1] << ", "
     << this->YViewUp[2] << ")\n";

  os << indent << "Z View Left To Right: ("
     << this->ZViewLeftToRight[0] << ", "
     << this->ZViewLeftToRight[1] << ", "
     << this->ZViewLeftToRight[2] << ")\n";

  os << indent << "Z View Up: ("
     << this->ZViewUp[0] << ", "
     << this->ZViewUp[1] << ", "
     << this->ZViewUp[2] << ")\n";
}
