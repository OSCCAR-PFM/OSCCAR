# the code below is needed to import objects from paraview.simple
# plus the definition of vtkTrivialProducer into this python script.
try: paraview.simple
except: from paraview.simple import *

def DoCoProcessing(datadescription):
  timestep = datadescription.GetTimeStep()

  grid = datadescription.GetInputDescriptionByName("input").GetGrid()
  pressure = grid.GetPointData().GetArray('Pressure')

  grid.GetPointData().SetScalars(pressure)
  trivialproducer = PVTrivialProducer()
  obj = trivialproducer.GetClientSideObject()
  obj.SetOutput(grid)

  if grid.IsA("vtkImageData") == True or grid.IsA("vtkStructuredGrid") == True or grid.IsA("vtkRectilinearGrid") == True:
    extent = datadescription.GetInputDescriptionByName("input").GetWholeExtent()
    trivialproducer.WholeExtent= [ extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] ]

  # get global range of Pressure
  trivialproducer.UpdatePipeline()
  RenderView1 = GetRenderView()
  RenderView1.ViewSize = [200, 300]
  DataRepresentation1 = Show()
  DataRepresentation1.Visibility = 0
  Contour1 = Contour( PointMergeMethod="Uniform Binning" )

  Contour1.PointMergeMethod = "Uniform Binning"
  Contour1.ContourBy = ['POINTS', 'Pressure']
  Contour1.Isosurfaces = [2952.0]

  RenderView1.Background = [1,1,1]

  RenderView1.CameraPosition = [5.0, 25.0, 347.53624862725769]
  RenderView1.CameraFocalPoint = [5.0, 25.0, 307.5]
  RenderView1.CameraClippingRange = [24.710886140985117, 59.424292356666555]
  RenderView1.CameraParallelScale = 13.743685418725535
  DataRepresentation2 = Show()
  DataRepresentation2.ScaleFactor = 1.5
  DataRepresentation2.EdgeColor = [0.0, 0.0, 0.50000762951094835]

  fname = 'CPGrid' + str(timestep) + '.png'
  WriteImage(fname)

  DataRepresentation2 = Show(trivialproducer)
  DataRepresentation2.LookupTable = MakeBlueToRedLT(2702, 3202)
  DataRepresentation2.ColorArrayName = 'Pressure'
  DataRepresentation2.ColorAttributeType = 'POINT_DATA'
  DataRepresentation2.Representation="Surface"
  DataRepresentation2 = Show(Contour1)
  RenderView1 = Render()
  RenderView1.Background=[1,1,1]  #white
  RenderView1.CameraPosition = [5.0, 25.0, 347.53624862725769]
  RenderView1.CameraFocalPoint = [5.0, 25.0, 307.5]
  RenderView1.CameraClippingRange = [24.710886140985117, 59.424292356666555]
  RenderView1.CameraParallelScale = 13.743685418725535

  fname = 'CPPressure' + str(timestep) + '.png'
  WriteImage(fname)


  # explicitly delete the proxies -- may have to do this multiple times
  tobedeleted = GetNextProxyToDelete()
  while tobedeleted != None:
      Delete(tobedeleted)
      tobedeleted = GetNextProxyToDelete()

def GetNextProxyToDelete():
  iter = servermanager.vtkSMProxyIterator()
  iter.SetSession(servermanager.ActiveConnection.Session)
  iter.Begin()
  while not iter.IsAtEnd():
    if iter.GetGroup().find("prototypes") != -1:
       iter.Next()
       continue
    proxy = servermanager._getPyProxy(iter.GetProxy())
    proxygroup = iter.GetGroup()
    iter.Next()
    if proxygroup != 'timekeeper' and proxy != None and proxygroup.find("pq_helper_proxies") == -1 :
        return proxy

  return None

def RequestDataDescription(datadescription):
  time = datadescription.GetTime()
  timestep = datadescription.GetTimeStep()
  if timestep % 20 == 0:
    # add in some fields
    #print 'added Pressure and wanting to do coprocessing'
    datadescription.GetInputDescriptionByName("input").AddPointField("Pressure")
    datadescription.GetInputDescriptionByName('input').GenerateMeshOn()
  return
