/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageResliceMapper.cxx

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
#include "vtkMesaImageResliceMapper.h"
#include "vtkMesaCamera.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaRenderWindow.h"


// make sure this file is included before the #define takes place
// so we don't get two vtkMesaImageResliceMapper classes defined.
#include "vtkOpenGLImageMapper3D.h"
#include "vtkMesaImageResliceMapper.h"

// Make sure vtkMesaImageResliceMapper is a copy of vtkOpenGLImageMapper3D
// with vtkOpenGLImageMapper3D replaced with vtkMesaImageResliceMapper
#define vtkOpenGLImageMapper3D vtkMesaImageResliceMapper
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#include "vtkOpenGLImageMapper3D.cxx"
#undef vtkOpenGLImageMapper3D
#undef vtkOpenGLRenderWindow

vtkStandardNewMacro(vtkMesaImageResliceMapper);
