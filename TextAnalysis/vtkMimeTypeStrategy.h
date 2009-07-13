/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMimeTypeStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef _vtkMimeTypeStrategy_h
#define _vtkMimeTypeStrategy_h

#include <vtkObject.h>
#include <vtkStdString.h> //Needed for lookup

/// Abstract interface for an object that can identify the MIME type of a file
class VTK_TEXT_ANALYSIS_EXPORT vtkMimeTypeStrategy :
  public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkMimeTypeStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Called to give this strategy an opportunity to identify the MIME type of a file.
  /// Return the MIME type if positively identified, otherwise return an empty string.
  virtual vtkStdString Lookup(const vtkStdString& path) = 0;

protected:
  vtkMimeTypeStrategy();
  virtual ~vtkMimeTypeStrategy();

private:
  vtkMimeTypeStrategy(const vtkMimeTypeStrategy&); //Not implemented.
  void operator=(const vtkMimeTypeStrategy&); //Not implemented.
};

#endif // !_vtkMimeTypeStrategy_h

