package require vtk
package require vtkinteraction

vtkImageReader2 reader
  reader SetFilePrefix "$VTK_DATA_ROOT/Data/headsq/quarter"
  reader SetDataExtent 0 63 0 63 1 93
  reader SetDataSpacing 3.2 3.2 1.5
  reader SetDataOrigin 0.0 0.0 0.0
  reader SetDataScalarTypeToUnsignedShort
  reader UpdateWholeExtent

vtkOpenGLImageResliceMapper im
  im SetInputConnection [ reader GetOutputPort ]

vtkImageProperty ip
  ip SetColorWindow 2000
  ip SetColorLevel 1000
  ip SetInterpolationTypeToLinear

vtkImage ia
  ia SetMapper im
  ia SetProperty ip

vtkOutlineFilter outline
  outline SetInputConnection [reader GetOutputPort ]

vtkPolyDataMapper om
  om SetInputConnection [ outline GetOutputPort ]

vtkActor oa
  oa SetMapper om

vtkRenderer renderer
vtkRenderWindow window
  window AddRenderer renderer

vtkInteractorStyleImage imageStyle
  imageStyle SetInteractionModeToImage3D

vtkRenderWindowInteractor interactor
  interactor SetInteractorStyle imageStyle
  interactor SetRenderWindow window

renderer AddViewProp ia
renderer AddViewProp oa
renderer SetBackground 0.1 0.2 0.4
window SetSize 400 400
window SetInteractor interactor

# render the image
window Render
[renderer GetActiveCamera] ParallelProjectionOn
renderer ResetCameraClippingRange
window Render

interactor Initialize

#
# Hide the default . widget
#
wm withdraw .
