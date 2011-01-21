#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
#VTK_DATA_ROOT="/Volumes/Work/Kitware/vtk-git-data"

reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataMask(0x7fff)
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
# for fullHead data use the following
#reader.SetDataExtent(0,255,0,255,1,93)
#reader.SetDataSpacing(0.8,0.8,1.5)
#reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/fullHead/headsq")

# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

im = vtk.vtkOpenGLImageMapper3D()
im.SetInputConnection(reader.GetOutputPort())

ip = vtk.vtkImageProperty()
ip.SetColorWindow(2000)
ip.SetColorLevel(1000)
ip.SetInterpolationTypeToLinear()

ia = vtk.vtkImage()
ia.SetMapper(im)
ia.SetProperty(ip)

ren1.AddViewProp(ia)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(400,400)

iren = vtk.vtkRenderWindowInteractor()
style = vtk.vtkInteractorStyleImage()
style.SetInteractionModeToImage3D()
iren.SetInteractorStyle(style)
renWin.SetInteractor(iren)

# render the image
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.ParallelProjectionOn()
ren1.ResetCameraClippingRange()
renWin.Render()

iren.Start()
