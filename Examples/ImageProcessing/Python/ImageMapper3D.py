#!/usr/bin/env python

import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()
#VTK_DATA_ROOT="/Volumes/Work/Kitware/vtk-git-data"

table = vtk.vtkLookupTable()
table.SetValueRange(0.0, 1.0)
table.SetSaturationRange(1.0, 1.0)
table.SetHueRange(0.0, 0.0)
table.SetAlphaRange(0.0, 1.0)
table.SetRampToLinear()
table.Build()
table.SetRange(0,2000)

transform = vtk.vtkTransform()
transform.RotateWXYZ(10.0, 0, 0, 1)

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
reader.Update()

colors = vtk.vtkImageMapToColors()
colors.SetInputConnection(reader.GetOutputPort())
colors.SetLookupTable(table)
colors.SetOutputFormatToRGBA()
colors.Update()

# Create the RenderWindow, Renderer and both Actors
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

im = vtk.vtkImageResliceMapper()
im.SetInputConnection(reader.GetOutputPort())

ip = vtk.vtkImageProperty()
ip.SetColorWindow(2000)
ip.SetColorLevel(1000)
#ip.SetLookupTable(table)
ip.SetInterpolationTypeToLinear()

ia = vtk.vtkImage()
ia.SetMapper(im)
ia.SetProperty(ip)

# a second image to superimpose on the first
im2 = vtk.vtkImageResliceMapper()
im2.SetInputConnection(reader.GetOutputPort())

ip2 = vtk.vtkImageProperty()
ip2.SetInterpolationTypeToLinear()
ip2.SetColorWindow(2000)
ip2.SetColorLevel(1000)
ip2.SetLookupTable(table)

ia2 = vtk.vtkImage()
ia2.SetUserMatrix(transform.GetMatrix())
ia2.SetMapper(im2)
ia2.SetProperty(ip2)

ren1.AddViewProp(ia)
#ren1.AddViewProp(ia2)
ren1.SetBackground(0.1,0.2,0.4)
renWin.SetSize(300,300)

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
