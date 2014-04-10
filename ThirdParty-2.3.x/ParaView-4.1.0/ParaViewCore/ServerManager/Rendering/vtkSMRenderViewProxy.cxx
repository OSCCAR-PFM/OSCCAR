/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRenderViewProxy.h"

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkDataArray.h"
#include "vtkEventForwarderCommand.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVLastSelectionInformation.h"
#include "vtkPVOptions.h"
#include "vtkPVRenderView.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMDataDeliveryManager.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkTransform.h"
#include "vtkWeakPointer.h"
#include "vtkWindowToImageFilter.h"

#include <map>

namespace
{

  bool vtkIsImageEmpty(vtkImageData* image)
    {
    vtkDataArray* scalars = image->GetPointData()->GetScalars();
    for (int comp=0; comp < scalars->GetNumberOfComponents(); comp++)
      {
      double range[2];
      scalars->GetRange(range, comp);
      if (range[0] != 0.0 || range[1] != 0.0)
        {
        return false;
        }
      }
    return true;
    }

  class vtkRenderHelper : public vtkPVRenderViewProxy
  {
public:
  static vtkRenderHelper* New();
  vtkTypeMacro(vtkRenderHelper, vtkPVRenderViewProxy);

  virtual void EventuallyRender()
    {
    this->Proxy->StillRender();
    }
  virtual vtkRenderWindow* GetRenderWindow() { return NULL; }
  virtual void Render()
    {
    this->Proxy->InteractiveRender();
    }
  // Description:
  // Returns true if the most recent render indeed employed low-res rendering.
  virtual bool LastRenderWasInteractive()
    {
    return this->Proxy->LastRenderWasInteractive();
    }

  vtkWeakPointer<vtkSMRenderViewProxy> Proxy;
  };
  vtkStandardNewMacro(vtkRenderHelper);
};

vtkStandardNewMacro(vtkSMRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMRenderViewProxy::vtkSMRenderViewProxy()
{
  this->IsSelectionCached = false;
  this->NewMasterObserverId = 0;
  this->DeliveryManager = NULL;
  this->NeedsUpdateLOD = true;
}

//----------------------------------------------------------------------------
vtkSMRenderViewProxy::~vtkSMRenderViewProxy()
{
  if( this->NewMasterObserverId != 0 &&
      this->Session && this->Session->GetCollaborationManager())
    {
    this->Session->GetCollaborationManager()->RemoveObserver(
          this->NewMasterObserverId);
    this->NewMasterObserverId = 0;
    }

  if (this->DeliveryManager)
    {
    this->DeliveryManager->Delete();
    this->DeliveryManager = NULL;
    }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::LastRenderWasInteractive()
{
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetUsedLODForLastRender() : false;
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::IsSelectionAvailable()
{
  const char* msg = this->IsSelectVisibleCellsAvailable();
  if (msg)
    {
    //vtkErrorMacro(<< msg);
    return false;
    }

  return true;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisibleCellsAvailable()
{
  vtkSMSession* session = this->GetSession();

  if(session->IsMultiClients() && !session->GetCollaborationManager()->IsMaster())
    {
    return "Cannot support selection in collaboration mode when not MASTER";
    }

  if (session->GetController(vtkPVSession::DATA_SERVER_ROOT) !=
    session->GetController(vtkPVSession::RENDER_SERVER_ROOT))
    {
    // when the two controller are different, we have a separate render-server
    // and data-server session.
    return "Cannot support selection in render-server mode";
    }

  vtkPVServerInformation* server_info = session->GetServerInformation();
  if (server_info && server_info->GetNumberOfMachines() > 0)
    {
    return "Cannot support selection in CAVE mode.";
    }

  //check if we don't have enough color depth to do color buffer selection
  //if we don't then disallow selection
  int rgba[4];
  vtkRenderWindow *rwin = this->GetRenderWindow();
  if (!rwin)
    {
    return "No render window available";
    }

  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    return "Selection not supported due to insufficient color depth.";
    }

  return NULL;
}

//-----------------------------------------------------------------------------
const char* vtkSMRenderViewProxy::IsSelectVisiblePointsAvailable()
{
  return this->IsSelectVisibleCellsAvailable();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::Update()
{
  this->NeedsUpdateLOD |= this->NeedsUpdate;
  this->Superclass::Update();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::UpdateLOD()
{
  if (this->ObjectsCreated && this->NeedsUpdateLOD)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "UpdateLOD"
           << vtkClientServerStream::End;
    this->GetSession()->PrepareProgress();
    this->ExecuteStream(stream);
    this->GetSession()->CleanupPendingProgress();
   
    this->NeedsUpdateLOD = false;
    }
}

//-----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::StreamingUpdate(bool render_if_needed)
{
  // FIXME: add a check to not do anything when in multi-client mode. We don't
  // support streaming in multi-client mode.
  this->GetSession()->PrepareProgress();

  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  double planes[24];
  vtkRenderer* ren = view->GetRenderer();
  ren->GetActiveCamera()->GetFrustumPlanes(
    ren->GetTiledAspectRatio(), planes);

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "StreamingUpdate"
         << vtkClientServerStream::InsertArray(planes, 24)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  // Now fetch any pieces that the server streamed back to the client.
  bool something_delivered = this->DeliveryManager->DeliverStreamedPieces();
  if (render_if_needed && something_delivered)
    {
    this->StillRender();
    }

  this->GetSession()->CleanupPendingProgress();
  return something_delivered;
}

//-----------------------------------------------------------------------------
vtkTypeUInt32 vtkSMRenderViewProxy::PreRender(bool interactive)
{
  this->Superclass::PreRender(interactive);

  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  assert(rv != NULL);

  if (interactive && rv->GetUseLODForInteractiveRender())
    {
    // for interactive renders, we need to determine if we are going to use LOD.
    // If so, we may need to update the LOD geometries.
    this->UpdateLOD();
    }
  this->DeliveryManager->Deliver(interactive);
  return interactive? rv->GetInteractiveRenderProcesses():
    rv->GetStillRenderProcesses();
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PostRender(bool interactive)
{
  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  this->SynchronizeCameraProperties();
  this->Superclass::PostRender(interactive);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::SynchronizeCameraProperties()
{
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMProxy* cameraProxy = this->GetSubProxy("ActiveCamera");
  cameraProxy->UpdatePropertyInformation();
  vtkSMPropertyIterator* iter = cameraProxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty *cur_property = iter->GetProperty();
    vtkSMProperty *info_property = cur_property->GetInformationProperty();
    if (!info_property)
      {
      continue;
      }
    cur_property->Copy(info_property);
    //cur_property->UpdateLastPushedValues();
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkSMRenderViewProxy::GetRenderer()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetRenderer() : NULL;
}

//----------------------------------------------------------------------------
vtkCamera* vtkSMRenderViewProxy::GetActiveCamera()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetActiveCamera() : NULL;
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor* vtkSMRenderViewProxy::GetInteractor()
{
  this->CreateVTKObjects();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  return rv? rv->GetInteractor() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  // If prototype, no need to go thurther...
  if(this->Location == 0)
    {
    return;
    }

  if (!this->ObjectsCreated)
    {
    return;
    }


  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());

  vtkCamera* camera = vtkCamera::SafeDownCast(this
                                              ->GetSubProxy( "ActiveCamera" )
                                              ->GetClientSideObject() );
  rv->SetActiveCamera( camera );

  if (rv->GetInteractor())
    {
    vtkRenderHelper* helper = vtkRenderHelper::New();
    helper->Proxy = this;
    rv->GetInteractor()->SetPVRenderView(helper);
    helper->Delete();
    }

  vtkEventForwarderCommand* forwarder = vtkEventForwarderCommand::New();
  forwarder->SetTarget(this);
  rv->AddObserver(vtkCommand::SelectionChangedEvent, forwarder);
  rv->AddObserver(vtkCommand::ResetCameraEvent, forwarder);
  forwarder->Delete();

  // We'll do this for now. But we need to not do this here. I am leaning
  // towards not making stereo a command line option as mentioned by a very
  // not-too-pleased user on the mailing list a while ago.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* pvoptions = pm->GetOptions();
  if (pvoptions->GetUseStereoRendering())
    {
    vtkSMPropertyHelper(this, "StereoCapableWindow").Set(1);
    vtkSMPropertyHelper(this, "StereoRender").Set(1);
    vtkSMEnumerationDomain* domain = vtkSMEnumerationDomain::SafeDownCast(
      this->GetProperty("StereoType")->GetDomain("enum"));
    if (domain && domain->HasEntryText(pvoptions->GetStereoType()))
      {
      vtkSMPropertyHelper(this, "StereoType").Set(
        domain->GetEntryValueForText(pvoptions->GetStereoType()));
      }
    }

  bool remote_rendering_available = true;
  if (this->GetSession()->GetIsAutoMPI())
    {
    // When the session is an auto-mpi session, we don't support remote
    // rendering.
    remote_rendering_available = false;
    }
 
  if (remote_rendering_available)
    {
    // Update whether render servers can open display i.e. remote rendering is
    // possible on all processes.
    vtkPVDisplayInformation* info = vtkPVDisplayInformation::New();
    this->GetSession()->GatherInformation(
      vtkPVSession::RENDER_SERVER, info, 0);
    if (info->GetCanOpenDisplay() == 0)
      {
      remote_rendering_available = false;
      }
    info->Delete();
    }

  // Disable remote rendering on all processes, if not available.
  if (remote_rendering_available == false)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << VTKOBJECT(this)
           << "RemoteRenderingAvailableOff"
           << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }

  // Attach to the collaborative session a callback to clear the selection cache
  // on the server side when we became master
  if(this->Session->IsMultiClients())
    {
    this->NewMasterObserverId =
        this->Session->GetCollaborationManager()->AddObserver(
          vtkSMCollaborationManager::UpdateMasterUser, this, &vtkSMRenderViewProxy::NewMasterCallback);
    }

  // Setup data-delivery manager.
  this->DeliveryManager = vtkSMDataDeliveryManager::New();
  this->DeliveryManager->SetViewProxy(this);
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMSessionProxyManager* pxm = source->GetSessionProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "UnstructuredGridRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UnstructuredGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "UnstructuredGridBaseRepresentation");

  pp = vtkSMInputProperty::SafeDownCast(prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usgb = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usgb)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UnstructuredGridBaseRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "StructuredGridRepresentation");

  pp = vtkSMInputProperty::SafeDownCast(prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool ssg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (ssg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "StructuredGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "UniformGridRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "UniformGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "AMRRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool ag = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (ag)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "AMRRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "MoleculeRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool mg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (mg)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "MoleculeRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "GeometryRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "GeometryRepresentation"));
    }

  vtkPVXMLElement* hints = source->GetHints();
  if (hints)
    {
    // If the source has an hint as follows, then it's a text producer and must
    // be is display-able.
    //  <Hints>
    //    <OutputPort name="..." index="..." type="text" />
    //  </Hints>

    unsigned int numElems = hints->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < numElems; cc++)
      {
      int index;
      vtkPVXMLElement* child = hints->GetNestedElement(cc);
      const char *childName = child->GetName();
      if (childName &&
        strcmp(childName, "OutputPort") == 0 &&
        child->GetScalarAttribute("index", &index) &&
        index == opport &&
        child->GetAttribute("type") &&
        strcmp(child->GetAttribute("type"), "text") == 0)
        {
        return vtkSMRepresentationProxy::SafeDownCast(
          pxm->NewProxy("representations", "TextSourceRepresentation"));
        }
      else if(childName && !strcmp(childName, "DefaultRepresentations"))
        {
        unsigned int defaultRepCount = child->GetNumberOfNestedElements();
        for(unsigned int i = 0; i < defaultRepCount; i++)
          {
          vtkPVXMLElement *defaultRep = child->GetNestedElement(i);
          const char *representation = defaultRep->GetAttribute("representation");

          return vtkSMRepresentationProxy::SafeDownCast(
            pxm->NewProxy("representations", representation));
          }
        }

      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ZoomTo(vtkSMProxy* representation)
{
  vtkSMPropertyHelper helper(representation, "Input");
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy());
  int port =helper.GetOutputPort();
  if (!input)
    {
    return;
    }

  vtkPVDataInformation* info = input->GetDataInformation(port);
  double bounds[6];
  info->GetBounds(bounds);
  if (!vtkMath::AreBoundsInitialized(bounds))
    {
    return;
    }

  if (representation->GetProperty("Position") &&
    representation->GetProperty("Orientation") &&
    representation->GetProperty("Scale"))
    {
    double position[3], rotation[3], scale[3];
    vtkSMPropertyHelper(representation, "Position").Get(position, 3);
    vtkSMPropertyHelper(representation, "Orientation").Get(rotation, 3);
    vtkSMPropertyHelper(representation, "Scale").Get(scale, 3);

    if (scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 ||
      position[0] != 0.0 || position[1] != 0.0 || position[2] != 0.0 ||
      rotation[0] != 0.0 || rotation[1] != 0.0 || rotation[2] != 0.0)
      {
      vtkTransform* transform = vtkTransform::New();
      transform->Translate(position);
      transform->RotateZ(rotation[2]);
      transform->RotateX(rotation[0]);
      transform->RotateY(rotation[1]);
      transform->Scale(scale);

      int i, j, k;
      double origX[3], x[3];
      vtkBoundingBox bbox;
      for (i = 0; i < 2; i++)
        {
        origX[0] = bounds[i];
        for (j = 0; j < 2; j++)
          {
          origX[1] = bounds[2 + j];
          for (k = 0; k < 2; k++)
            {
            origX[2] = bounds[4 + k];
            transform->TransformPoint(origX, x);
            bbox.AddPoint(x);
            }
          }
        }
      bbox.GetBounds(bounds);
      transform->Delete();
      }
    }
  this->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ResetCamera(double bounds[6])
{
  this->CreateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "ResetCamera"
         << vtkClientServerStream::InsertArray(bounds, 6)
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//-----------------------------------------------------------------------------
void vtkSMRenderViewProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  this->ClearSelectionCache();

  // skip modified properties on camera subproxy.
  if (modifiedProxy != this->GetSubProxy("ActiveCamera"))
    {
    this->Superclass::MarkDirty(modifiedProxy);
    }
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::Pick(int x, int y)
{
  // 1) Create surface selection.
  //   Will returns a surface selection in terms of cells selected on the
  //   visible props from all representations.
  vtkSMRepresentationProxy* repr = NULL;
  vtkCollection* reprs = vtkCollection::New();
  vtkCollection* sources = vtkCollection::New();
  int region[4] = {x,y,x,y};
  if (this->SelectSurfaceCells(region, reprs, sources, false))
    {
    if (reprs->GetNumberOfItems() > 0)
      {
      repr = vtkSMRepresentationProxy::SafeDownCast(reprs->GetItemAsObject(0));
      }
    }
  reprs->Delete();
  sources->Delete();
  return repr;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMRenderViewProxy::PickBlock(int x,
                                                          int y,
                                                          unsigned int &flatIndex)
{
  vtkSMRepresentationProxy* repr = NULL;
  vtkSmartPointer<vtkCollection> reprs = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollection> sources = vtkSmartPointer<vtkCollection>::New();
  int region[4] = {x,y,x,y};
  if(this->SelectSurfaceCells(region, reprs.GetPointer(), sources.GetPointer(), false))
    {
    if (reprs->GetNumberOfItems() > 0)
      {
      repr = vtkSMRepresentationProxy::SafeDownCast(reprs->GetItemAsObject(0));
      }
    }

  if(!repr)
    {
    // nothing selected
    return 0;
    }

  // get data information
  vtkPVDataInformation *info = repr->GetRepresentedDataInformation();
  vtkPVCompositeDataInformation *compositeInfo = info->GetCompositeDataInformation();

  // get selection in order to determine which block of the data set
  // set was selected (if it is a composite data set)
  if(compositeInfo && compositeInfo->GetDataIsComposite())
    {
    vtkSMProxy *selectionSource =
      vtkSMProxy::SafeDownCast(sources->GetItemAsObject(0));

    // get representation input data
    vtkSMSourceProxy *reprInput =
      vtkSMSourceProxy::SafeDownCast(
        vtkSMPropertyHelper(repr, "Input").GetAsProxy());

    // convert cell selection to block selection
    vtkSMProxy *blockSelection =
      vtkSMSelectionHelper::ConvertSelection(vtkSelectionNode::BLOCKS,
                                             selectionSource,
                                             reprInput,
                                             0);

    // set block index
    flatIndex = vtkSMPropertyHelper(blockSelection, "Blocks").GetAsInt();

    blockSelection->Delete();
    }

  // return selected representation
  return repr;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::ConvertDisplayToPointOnSurface(
    const int display_position[2], double world_position[3])
{
  int region[4] = {display_position[0], display_position[1],
    display_position[0], display_position[1] };

  vtkSMSessionProxyManager* spxm = this->GetSessionProxyManager();
  vtkNew<vtkCollection> representations;
  vtkNew<vtkCollection> sources;
  this->SelectSurfaceCells(region, representations.GetPointer(), sources.GetPointer(), false);

  if (representations->GetNumberOfItems() > 0 && sources->GetNumberOfItems() > 0)
    {
    vtkSMPVRepresentationProxy* rep =
      vtkSMPVRepresentationProxy::SafeDownCast(representations->GetItemAsObject(0));
    vtkSMProxy* input = vtkSMPropertyHelper(rep, "Input").GetAsProxy(0);
    vtkSMSourceProxy* selection = vtkSMSourceProxy::SafeDownCast(sources->GetItemAsObject(0));

    // Picking info
    // {r0, r1, 1} => We want to make sure the ray that start from the camera reach
    // the end of the scene so it could cross any cell of the scene
    double nearDisplayPoint[3] = { (double)region[0], (double)region[1], 0.0 };
    double farDisplayPoint[3]  = { (double)region[0], (double)region[1], 1.0 };
    double farLinePoint[3];
    double nearLinePoint[3];

    vtkRenderer* renderer = this->GetRenderer();

    // compute near line point
    renderer->SetDisplayPoint(nearDisplayPoint);
    renderer->DisplayToWorld();
    const double* world = renderer->GetWorldPoint();
    for (int i=0; i < 3; i++)
      {
      nearLinePoint[i] = world[i] / world[3];
      }

    // compute far line point
    renderer->SetDisplayPoint(farDisplayPoint);
    renderer->DisplayToWorld();
    world = renderer->GetWorldPoint();
    for (int i=0; i < 3; i++)
      {
      farLinePoint[i] = world[i] / world[3];
      }

    // Compute the  intersection...
    vtkSMProxy* pickingHelper = spxm->NewProxy("misc","PickingHelper");
    vtkSMPropertyHelper(pickingHelper, "Input").Set( input );
    vtkSMPropertyHelper(pickingHelper, "Selection").Set( selection );
    vtkSMPropertyHelper(pickingHelper, "PointA").Set(nearLinePoint, 3);
    vtkSMPropertyHelper(pickingHelper, "PointB").Set(farLinePoint, 3);
    pickingHelper->UpdateVTKObjects();
    pickingHelper->UpdateProperty("Update",1);
    vtkSMPropertyHelper(pickingHelper, "Intersection").UpdateValueFromServer();
    vtkSMPropertyHelper(pickingHelper, "Intersection").Get(world_position, 3);
    pickingHelper->Delete();
    }
  else
    {
    // Need to warn user when used in RenderServer mode
    if(!this->IsSelectionAvailable())
      {
      vtkWarningMacro("Snapping to the surface is not available therefore "
        "the camera focal point will be used to determine "
        "the depth of the picking.");
      }

    // Use camera focal point to get some Zbuffer
    double cameraFP[4];
    vtkRenderer* renderer = this->GetRenderer();
    vtkCamera* camera = renderer->GetActiveCamera();
    camera->GetFocalPoint(cameraFP); cameraFP[3] = 1.0;
    renderer->SetWorldPoint(cameraFP);
    renderer->WorldToDisplay();
    double *displayCoord = renderer->GetDisplayPoint();

    // Handle display to world conversion
    double display[3] = {(double)region[0], (double)region[1], displayCoord[2]};
    renderer->SetDisplayPoint(display);
    renderer->DisplayToWorld();
    const double* world = renderer->GetWorldPoint();
    for (int i=0; i < 3; i++)
      {
      world_position[i] = world[i] / world[3];
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfaceCells(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
    {
    return false;
    }

  this->IsSelectionCached = true;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "SelectCells"
         << region[0] << region[1] << region[2] << region[3]
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  return this->FetchLastSelection(
    multiple_selections, selectedRepresentations, selectionSources);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectSurfacePoints(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
    {
    return false;
    }

  this->IsSelectionCached = true;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(this)
         << "SelectPoints"
         << region[0] << region[1] << region[2] << region[3]
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  return this->FetchLastSelection(
    multiple_selections, selectedRepresentations, selectionSources);
}

namespace
{
  //-----------------------------------------------------------------------------
  static void vtkShrinkSelection(vtkSelection* sel)
    {
    std::map<void*, int> pixelCounts;
    unsigned int numNodes = sel->GetNumberOfNodes();
    void* choosen = NULL;
    int maxPixels = -1;
    for (unsigned int cc=0; cc < numNodes; cc++)
      {
      vtkSelectionNode* node = sel->GetNode(cc);
      vtkInformation* properties = node->GetProperties();
      if (properties->Has(vtkSelectionNode::PIXEL_COUNT()) &&
        properties->Has(vtkSelectionNode::SOURCE()))
        {
        int numPixels = properties->Get(vtkSelectionNode::PIXEL_COUNT());
        void* source = properties->Get(vtkSelectionNode::SOURCE());
        pixelCounts[source] += numPixels;
        if (pixelCounts[source] > maxPixels)
          {
          maxPixels = numPixels;
          choosen = source;
          }
        }
      }

    std::vector<vtkSmartPointer<vtkSelectionNode> > choosenNodes;
    if (choosen != NULL)
      {
      for (unsigned int cc=0; cc < numNodes; cc++)
        {
        vtkSelectionNode* node = sel->GetNode(cc);
        vtkInformation* properties = node->GetProperties();
        if (properties->Has(vtkSelectionNode::SOURCE()) &&
          properties->Get(vtkSelectionNode::SOURCE()) == choosen)
          {
          choosenNodes.push_back(node);
          }
        }
      }
    sel->RemoveAllNodes();
    for (unsigned int cc=0; cc <choosenNodes.size(); cc++)
      {
      sel->AddNode(choosenNodes[cc]);
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::FetchLastSelection(
  bool multiple_selections,
  vtkCollection* selectedRepresentations, vtkCollection* selectionSources)
{
  if (selectionSources && selectedRepresentations)
    {
    vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
      this->GetClientSideObject());
    vtkSelection* selection = rv->GetLastSelection();
    if (!multiple_selections)
      {
      // only pass through selection over a single representation.
      vtkShrinkSelection(selection);
      }
    vtkSMSelectionHelper::NewSelectionSourcesFromSelection(
      selection, this, selectionSources, selectedRepresentations);
    return (selectionSources->GetNumberOfItems() > 0);
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumCells(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  return this->SelectFrustumInternal(region, selectedRepresentations,
    selectionSources, multiple_selections, vtkSelectionNode::CELL);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumPoints(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  return this->SelectFrustumInternal(region, selectedRepresentations,
    selectionSources, multiple_selections, vtkSelectionNode::POINT);
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectFrustumInternal(int region[4],
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections,
  int fieldAssociation)
{
  // Simply stealing old code for now. This code have many coding style
  // violations and seems too long for what it does. At some point we'll check
  // it out.

  int displayRectangle[4] = {region[0], region[1], region[2], region[3]};
  if (displayRectangle[0] == displayRectangle[2])
    {
    displayRectangle[2] += 1;
    }
  if (displayRectangle[1] == displayRectangle[3])
    {
    displayRectangle[3] += 1;
    }

  // 1) Create frustum selection
  //convert screen rectangle to world frustum
  vtkRenderer *renderer = this->GetRenderer();
  double frustum[32];
  int index=0;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);
  index++;
  renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&frustum[index*4]);

  vtkSMProxy* selectionSource = this->GetSessionProxyManager()->NewProxy("sources",
    "FrustumSelectionSource");
  vtkSMPropertyHelper(selectionSource, "FieldType").Set(fieldAssociation);
  vtkSMPropertyHelper(selectionSource, "Frustum").Set(frustum, 32);
  selectionSource->UpdateVTKObjects();

  // 2) Figure out which representation is "selected".
  vtkExtractSelectedFrustum* extractor =
    vtkExtractSelectedFrustum::New();
  extractor->CreateFrustum(frustum);

  // Now we just use the first selected representation,
  // until we have other mechanisms to select one.
  vtkSMPropertyHelper reprsHelper(this, "Representations");

  for (unsigned int cc=0;  cc < reprsHelper.GetNumberOfElements(); cc++)
    {
    vtkSMRepresentationProxy* repr =
      vtkSMRepresentationProxy::SafeDownCast(reprsHelper.GetAsProxy(cc));
    if (!repr || vtkSMPropertyHelper(repr, "Visibility", true).GetAsInt() == 0)
      {
      continue;
      }
    if (vtkSMPropertyHelper(repr, "Pickable", true).GetAsInt() == 0)
      {
      // skip non-pickable representations.
      continue;
      }
    vtkPVDataInformation* datainfo = repr->GetRepresentedDataInformation();
    if (!datainfo)
      {
      continue;
      }

    double bounds[6];
    datainfo->GetBounds(bounds);

    if (extractor->OverallBoundsTest(bounds))
      {
      selectionSources->AddItem(selectionSource);
      selectedRepresentations->AddItem(repr);
      if (!multiple_selections)
        {
        break;
        }
      }
    }

  extractor->Delete();
  selectionSource->Delete();
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonPoints(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
    {
    return false;
    }
  return this->SelectPolygonInternal(polygonPts, selectedRepresentations,
    selectionSources, multiple_selections, "SelectPolygonPoints");
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonCells(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections)
{
  if (!this->IsSelectionAvailable())
    {
    return false;
    }
  return this->SelectPolygonInternal(polygonPts, selectedRepresentations,
    selectionSources, multiple_selections, "SelectPolygonCells");
}

//----------------------------------------------------------------------------
bool vtkSMRenderViewProxy::SelectPolygonInternal(vtkIntArray* polygonPts,
  vtkCollection* selectedRepresentations,
  vtkCollection* selectionSources,
  bool multiple_selections,
  const char* method)
{
  this->IsSelectionCached = true;

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
  << VTKOBJECT(this)
  << method
  << vtkClientServerStream::InsertArray(polygonPts->GetPointer(0),
     polygonPts->GetNumberOfTuples()*polygonPts->GetNumberOfComponents())
  << polygonPts->GetNumberOfTuples()*polygonPts->GetNumberOfComponents()
  << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  return this->FetchLastSelection(
    multiple_selections, selectedRepresentations, selectionSources);
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::CaptureWindowInternalRender()
{
  vtkPVRenderView* view =
    vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
  if (view->GetUseInteractiveRenderingForSceenshots())
    {
    this->InteractiveRender();
    }
  else
    {
    this->StillRender();
    }
}

//----------------------------------------------------------------------------
vtkImageData* vtkSMRenderViewProxy::CaptureWindowInternal(int magnification)
{
#if !defined(__APPLE__)
  vtkPVRenderView* view =
    vtkPVRenderView::SafeDownCast(this->GetClientSideObject());
#endif

  // Offscreen rendering is not functioning properly on the mac.
  // Do not use it.
#if !defined(__APPLE__)
  vtkRenderWindow* window = this->GetRenderWindow();
  int prevOffscreen = window->GetOffScreenRendering();
  bool use_offscreen = view->GetUseOffscreenRendering() ||
    view->GetUseOffscreenRenderingForScreenshots();
  window->SetOffScreenRendering(use_offscreen? 1: 0);
#endif

  this->GetRenderWindow()->SwapBuffersOff();

  this->CaptureWindowInternalRender();

  vtkSmartPointer<vtkWindowToImageFilter> w2i =
    vtkSmartPointer<vtkWindowToImageFilter>::New();
  w2i->SetInput(this->GetRenderWindow());
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOn();

  // BUG #8715: We go through this indirection since the active connection needs
  // to be set during update since it may request re-renders if magnification >1.
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << w2i.GetPointer() << "Update"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkProcessModule::CLIENT);

  this->GetRenderWindow()->SwapBuffersOn();

#if !defined(__APPLE__)
  window->SetOffScreenRendering(prevOffscreen);

  if (view->GetUseOffscreenRenderingForScreenshots() &&
    vtkIsImageEmpty(w2i->GetOutput()))
    {
    // ensure that some image was capture. Due to buggy offscreen rendering
    // support on some drivers, we may end up with black images, in which case
    // we force on-screen rendering.
    if (vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses() == 1)
      {
      vtkWarningMacro(
        "Disabling offscreen rendering since empty image was detected.");
      view->SetUseOffscreenRenderingForScreenshots(false);
      return this->CaptureWindowInternal(magnification);
      }
    }
#endif

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  this->GetRenderWindow()->Frame();
  return capture;
}

//----------------------------------------------------------------------------
double vtkSMRenderViewProxy::GetZBufferValue(int x, int y)
{
  this->Session->Activate();
  vtkPVRenderView* rv = vtkPVRenderView::SafeDownCast(
    this->GetClientSideObject());
  double result = rv? rv->GetZbufferDataAtPoint(x, y) : 1.0;
  this->Session->DeActivate();

  return result;
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::NewMasterCallback(vtkObject*, unsigned long, void*)
{
  if( this->Session && this->Session->IsMultiClients() &&
      this->Session->GetCollaborationManager()->IsMaster())
    {
    // Make sure we clear the selection cache server side as well as previous
    // master might already have set a selection that has been cached.
    this->ClearSelectionCache(/*force=*/true);
    }
}

//----------------------------------------------------------------------------
void vtkSMRenderViewProxy::ClearSelectionCache(bool force/*=false*/)
{
  if(this->IsSelectionCached || force)
    {
    this->IsSelectionCached = false;
    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << VTKOBJECT(this)
            << "InvalidateCachedSelection"
            << vtkClientServerStream::End;
    this->ExecuteStream(stream);
    }
}
