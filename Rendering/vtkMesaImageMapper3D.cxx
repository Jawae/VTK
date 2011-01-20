/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageMapper3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This keeps the New method from being defined in included cxx file.
#define VTK_IMPLEMENT_MESA_CXX
#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"

#include <math.h>
#include "vtkToolkits.h"
#include "vtkMesaImageMapper3D.h"
#include "vtkMesaCamera.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRenderWindow.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaImageMapper3D classes defined.
#include "vtkOpenGLImageMapper3D.h"
#include "vtkMesaImageMapper3D.h"

// Make sure vtkMesaImageMapper3D is a copy of vtkOpenGLImageMapper3D
// with vtkOpenGLImageMapper3D replaced with vtkMesaImageMapper3D
#define vtkOpenGLImageMapper3D vtkMesaImageMapper3D
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#include "vtkOpenGLImageMapper3D.cxx"
#undef vtkOpenGLImageMapper3D
#undef vtkOpenGLRenderWindow

vtkStandardNewMacro(vtkMesaImageMapper3D);
