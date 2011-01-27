/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleImage - interactive manipulation of the camera specialized for images
// .SECTION Description
// vtkInteractorStyleImage allows the user to interactively manipulate
// (rotate, pan, zoomm etc.) the camera. vtkInteractorStyleImage is specially
// designed to work with images that are being rendered with
// vtkImageActor. Several events are overloaded from its superclass
// vtkInteractorStyle, hence the mouse bindings are different. (The bindings
// keep the camera's view plane normal perpendicular to the x-y plane.) In
// summary the mouse events for 2D image interaction are as follows:
// + Left Mouse button triggers window level events
// + CTRL Left Mouse spins the camera around its view plane normal
// + SHIFT Left Mouse pans the camera
// + CTRL SHIFT Left Mouse dollys (a positional zoom) the camera
// + Middle mouse button pans the camera
// + Right mouse button dollys the camera.
// + SHIFT Right Mouse triggers pick events
// If SetInteractionModeToImage3D() is called, then some of the mouse
// events are changed as follows:
// + SHIFT Left Mouse rotates the camera for oblique slicing
// + SHIFT Middle Mouse slices through the image
// + CTRL Right Mouse also slices through the image
// In both modes, the following key bindings are in effect:
// + R Reset the Window/Level
// + X Reset to a sagittal view
// + Y Reset to a coronal view
// + Z Reset to an axial view
// Note that the renderer's actors are not moved; instead the camera is moved.

// .SECTION See Also
// vtkInteractorStyle vtkInteractorStyleTrackballActor 
// vtkInteractorStyleJoystickCamera vtkInteractorStyleJoystickActor

#ifndef __vtkInteractorStyleImage_h
#define __vtkInteractorStyleImage_h

#include "vtkInteractorStyleTrackballCamera.h"

// Motion flags

#define VTKIS_WINDOW_LEVEL 1024
#define VTKIS_PICK         1025
#define VTKIS_SLICE        1026

// Style flags

#define VTKIS_IMAGE2D 2
#define VTKIS_IMAGE3D 3

class vtkImageProperty;

class VTK_RENDERING_EXPORT vtkInteractorStyleImage : public vtkInteractorStyleTrackballCamera
{
public:
  static vtkInteractorStyleImage *New();
  vtkTypeMacro(vtkInteractorStyleImage, vtkInteractorStyleTrackballCamera);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Some useful information for handling window level
  vtkGetVector2Macro(WindowLevelStartPosition,int);
  vtkGetVector2Macro(WindowLevelCurrentPosition,int);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();

  // Description:
  // Override the "fly-to" (f keypress) for images.
  virtual void OnChar();

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void WindowLevel();
  virtual void Pick();
  virtual void Slice();
  
  // Interaction mode entry points used internally.  
  virtual void StartWindowLevel();
  virtual void EndWindowLevel();
  virtual void StartPick();
  virtual void EndPick();
  virtual void StartSlice();
  virtual void EndSlice();

  // Description:
  // Set/Get current mode to 2D or 3D.  The default is 2D.  In 3D mode,
  // it is possible to rotate the camera to view oblique slices.
  vtkSetClampMacro(InteractionMode, int, VTKIS_IMAGE2D, VTKIS_IMAGE3D);
  vtkGetMacro(InteractionMode, int);
  void SetInteractionModeToImage2D() {
    this->SetInteractionMode(VTKIS_IMAGE2D); }
  void SetInteractionModeToImage3D() {
    this->SetInteractionMode(VTKIS_IMAGE3D); }

  // Description:
  // Set the canonical orientations for X, Y, and Z.  These allow you to
  // specify, for each view, what the "Up" direction should be and what
  // the "Left-to-Right" direction should be, where "left" and "right"
  // correspond to your own left and right while you are looking at the
  // computer screen.  The "Up" direction will be orthogonalized with
  // respect to the "LeftToRight" direction.
  vtkSetVector3Macro(XViewLeftToRight, double);
  vtkGetVector3Macro(XViewLeftToRight, double);
  vtkSetVector3Macro(XViewUp, double);
  vtkGetVector3Macro(XViewUp, double);
  vtkSetVector3Macro(YViewLeftToRight, double);
  vtkGetVector3Macro(YViewLeftToRight, double);
  vtkSetVector3Macro(YViewUp, double);
  vtkGetVector3Macro(YViewUp, double);
  vtkSetVector3Macro(ZViewLeftToRight, double);
  vtkGetVector3Macro(ZViewLeftToRight, double);
  vtkSetVector3Macro(ZViewUp, double);
  vtkGetVector3Macro(ZViewUp, double);

  // Description:
  // Set the image orientation.  The first parameter is the
  // LeftToRight vector and the second parameter is the Up vector.
  // All this does is change the position of the camera to achieve
  // the specified viewpoint.
  void SetImageOrientation(const double leftToRight[3],
                           const double viewUp[3]);

protected:
  vtkInteractorStyleImage();
  ~vtkInteractorStyleImage();

  int WindowLevelStartPosition[2];
  int WindowLevelCurrentPosition[2];
  double WindowLevelInitial[2];
  vtkImageProperty *WindowLevelProperty;
 
  int InteractionMode;
  double XViewLeftToRight[3];
  double XViewUp[3];
  double YViewLeftToRight[3];
  double YViewUp[3];
  double ZViewLeftToRight[3];
  double ZViewUp[3];

private:
  vtkInteractorStyleImage(const vtkInteractorStyleImage&);  // Not implemented.
  void operator=(const vtkInteractorStyleImage&);  // Not implemented.
};

#endif
