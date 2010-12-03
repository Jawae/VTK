/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFloodFill.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageFloodFill - Flood fill an image, given thresholds.
// .SECTION Description
// vtkImageFloodFill will perform a flood fill on an image, given upper
// and lower pixel intensity thresholds. Essentially, it works the same
// as vtkImageThreshold, but also allows the user to set seed points
// to limit the threshold operation to contiguous regions of the image.
// The filled region, or the "inside", will be passed through to the
// output by default, while the "outside" will be replaced with zeros.
// This behavior can be change by using the ReplaceIn() and ReplaceOut()
// methods.  The scalar type of the output is the same as the input.
// .SECTION see also
// vtkImageThreshold
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef __vtkImageFloodFill_h
#define __vtkImageFloodFill_h

#include "vtkImageToImageFilter.h"
#include "vtkImageData.h"

class vtkPoints;
class vtkImageStencilData;

class VTK_IMAGING_EXPORT vtkImageFloodFill : public vtkImageToImageFilter
{
public:
  static vtkImageFloodFill *New();
  vtkTypeRevisionMacro(vtkImageFloodFill, vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the seeds.
  void SetSeedPoints(vtkPoints *points);
  vtkGetObjectMacro(SeedPoints, vtkPoints);

  // Description:
  // Limit the flood to the specified region of the image.
  vtkSetVector6Macro(FloodExtent, int);
  vtkGetVector6Macro(FloodExtent, int);

  // Description:
  // Values greater than or equal to this threshold will be filled.
  void ThresholdByUpper(double thresh);

  // Description:
  // Values less than or equal to this threshold will be filled.
  void ThresholdByLower(double thresh);

  // Description:
  // Values within this range will be filled, where the range inludes
  // values that are exactly equal to the lower and upper thresholds.
  void ThresholdBetween(double lower, double upper);

  // Description:
  // Determines whether to replace the filled region by the value
  // set by SetInValue().
  vtkSetMacro(ReplaceIn, int);
  vtkGetMacro(ReplaceIn, int);
  vtkBooleanMacro(ReplaceIn, int);

  // Description:
  // If ReplaceIn is set, the filled region will be replaced by this
  // value.
  void SetInValue(double val);
  vtkGetMacro(InValue, double);

  // Description:
  // Determines whether to replace the filled region by the value
  // set by SetInValue().
  vtkSetMacro(ReplaceOut, int);
  vtkGetMacro(ReplaceOut, int);
  vtkBooleanMacro(ReplaceOut, int);

  // Description:
  // If ReplaceIn is set, the filled region will be replaced by this
  // value.
  void SetOutValue(double val);
  vtkGetMacro(OutValue, double);

  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold, double);
  vtkGetMacro(LowerThreshold, double);

  // Description:
  // Specify a stencil that will be used to limit the flood fill to
  // an arbitrarily-shaped region of the image.
  virtual void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Reverse the stencil set by SetStencil().
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // For multi-component images, you must set the component to threshold
  // during the flood-fill operation.
  vtkSetMacro(ActiveComponent,int);
  vtkGetMacro(ActiveComponent,int);

  // Description:
  // Override the MTime to account for the seed points.
  unsigned long GetMTime();

  // Description:
  // After the filter has executed, use GetNumberOfVoxels() to find
  // out how many voxels were filled.
  vtkSetMacro(NumberOfInVoxels, int);
  vtkGetMacro(NumberOfInVoxels, int);

  // Description:
  // For internal use only.
  vtkSetVector6Macro(FloodBounds, int);
  vtkGetVector6Macro(FloodBounds, int);

  // Description:
  // For internal use only.
  vtkSetObjectMacro(ImageMask, vtkImageData);
  vtkGetObjectMacro(ImageMask, vtkImageData);

protected:
  vtkImageFloodFill();
  ~vtkImageFloodFill();

  double UpperThreshold;
  double LowerThreshold;
  int ReplaceIn;
  double InValue;
  int ReplaceOut;
  double OutValue;

  vtkPoints *SeedPoints;
  int FloodExtent[6];
  int FloodBounds[6];

  int NumberOfInVoxels;

  int ReverseStencil;
  int ActiveComponent;

  vtkImageData *ImageMask;

  void ExecuteData(vtkDataObject *out);

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);

private:
  vtkImageFloodFill(const vtkImageFloodFill&);  // Not implemented.
  void operator=(const vtkImageFloodFill&);  // Not implemented.
};

#endif
