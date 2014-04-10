/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateVersionController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateVersionController.h"

#include "vtkCollection.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"

#include <vector>
#include <set>
#include <string>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMStateVersionController);
//----------------------------------------------------------------------------
vtkSMStateVersionController::vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
vtkSMStateVersionController::~vtkSMStateVersionController()
{
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process(vtkPVXMLElement* parent)
{
  vtkPVXMLElement* root = parent;
  if (parent && strcmp(parent->GetName(), "ServerManagerState") != 0)
    {
    root = root->FindNestedElementByName("ServerManagerState");
    }

  if (!root || strcmp(root->GetName(), "ServerManagerState") != 0)
    {
    vtkErrorMacro("Invalid root element. Expected \"ServerManagerState\"");
    return false;
    }

  int version[3] = {0, 0, 0};
  bool status = true;
  this->ReadVersion(root, version);

  if (this->GetMajor(version) < 3)
    {
    vtkWarningMacro("State file version is less than 3.0.0, "
      "these states may not work correctly.");

    int updated_version[3] = {3, 0, 0};
    this->UpdateVersion(version, updated_version);
    }

  // Note that we apply to 3.1 changes from 3.0 to 3.2. Odd versions
  // are floating versions that are somewhere between two releases.
  // They may or may not have the backwards compatibility issue of
  // the previous release. We assume that they do because updating
  // them should not break anything even if they do not.
  if (this->GetMajor(version)==3 &&
      (this->GetMinor(version)==0 || this->GetMinor(version)==1))
    {
    if (this->GetMinor(version)==0 && this->GetPatch(version) < 2)
      {
      vtkWarningMacro("Due to fundamental changes in the parallel rendering framework "
        "it is not possible to load states with volume rendering correctly "
        "for versions less than 3.0.2.");
      }
    status = status && this->Process_3_0_To_3_2(root) ;

    // Since now the state file has been updated to version 3.2.0, we must update
    // the version number to reflect that.
    int updated_version[3] = {3, 2, 0};
    this->UpdateVersion(version, updated_version);
    }

  if (this->GetMajor(version)==3 &&
      (this->GetMinor(version)==2 || this->GetMinor(version)==3))
    {
    status = status && this->Process_3_2_To_3_4(root) ;

    // Since now the state file has been updated to version 3.4.0, we must update
    // the version number to reflect that.
    int updated_version[3] = {3, 4, 0};
    this->UpdateVersion(version, updated_version);
    }

  if (this->GetMajor(version)==3 &&
    this->GetMinor(version) < 6)
    {
    status = status && this->Process_3_4_to_3_6(root);
    // Since now the state file has been updated to version 3.6.0, we must update
    // the version number to reflect that.
    int updated_version[3] = {3, 6, 0};
    this->UpdateVersion(version, updated_version);
    }

  if ( this->GetMajor( version ) == 3 && this->GetMinor( version ) < 8 )
    {
    status = status && this->Process_3_6_to_3_8( root );
    // Since now the state file has been updated to version 3.8.0, we must update
    // the version number to reflect that.
    int updated_version[3] = { 3, 8, 0 };
    this->UpdateVersion( version, updated_version );
    }

  if ( this->GetMajor( version ) == 3 && this->GetMinor( version ) < 10 )
    {
    status = status && this->Process_3_8_to_3_10( root );
    // Since now the state file has been updated to version 3.10.0, we must update
    // the version number to reflect that.
    int updated_version[3] = { 3, 10, 0 };
    this->UpdateVersion( version, updated_version );
    }

  if ( this->GetMajor(version) == 3 && this->GetMinor(version) < 11 )
    {
    status = status && this->Process_3_10_to_3_12(root);
    // Since now the state file has been updated to version 3.12.0, we must update
    // the version number to reflect that.
    int updated_version[3] = { 3, 12, 0 };
    this->UpdateVersion( version, updated_version );
    }

  if (this->GetMajor(version) == 3 && this->GetMajor(version) < 14)
    {
    status = status && this->Process_3_12_to_3_14(root, parent);

    // Since now the state file has been updated to version 3.14.0, we must update
    // the version number to reflect that.
    int updated_version[3] = { 3, 14, 0 };
    this->UpdateVersion( version, updated_version );
    }

  if (this->GetMajor(version)==3 && this->GetMinor(version) <= 14)
    {
    status = status && this->Process_3_14_to_4_0(root);

    // Since now the state file has been updated to version 4.0, we must update
    // the version number to reflect that.
    int updated_version[3] = { 4, 0, 1 };
    this->UpdateVersion( version, updated_version );
    }

  if (this->GetMajor(version) == 4 && this->GetMinor(version) <= 0)
    {
    status = status && this->Process_4_0_to_next(root);
    }

  return status;
}

namespace // namespace begin
{

//----------------------------------------------------------------------------
bool ConvertViewModulesToViews(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertViewModulesToViews(root);
}

//----------------------------------------------------------------------------
bool ConvertLegacyReader(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertLegacyReader(root);
}

//----------------------------------------------------------------------------
// Called for every data-object display. We will update scalar color
// properties since those changed.
bool ConvertDataDisplaysToRepresentations(vtkPVXMLElement* root,
  void* vtkNotUsed(callData))
{
  // Change proxy type based on the presence of "VolumePipelineType" hint.
  vtkPVXMLElement* typeHint = root->FindNestedElementByName("VolumePipelineType");
  const char* type = "GeometryRepresentation";
  if (typeHint)
    {
    const char* hinttype = typeHint->GetAttribute("type");
    if (hinttype)
      {
      if (strcmp(hinttype, "IMAGE_DATA")==0)
        {
        type = "UniformGridRepresentation";
        }
      else if (strcmp(hinttype, "UNSTRUCTURED_GRID")==0)
        {
        type ="UnstructuredGridRepresentation";
        }
      }
    }
  root->SetAttribute("type", type);
  root->SetAttribute("group", "representations");

  // ScalarMode --> ColorAttributeType
  //  vals: 0/1/2/3 ---> 0 (point-data)
  //      : 4       ---> 1 (cell-data)
  // ColorArray --> ColorArrayName
  //  val remains same.
  unsigned int max = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Property")==0)
      {
      const char* pname = child->GetAttribute("name");
      if (pname && strcmp(pname, "ColorArray")==0)
        {
        child->SetAttribute("name", "ColorArrayName");
        }
      else if (pname && strcmp(pname, "ScalarMode")==0)
        {
        child->SetAttribute("name", "ColorAttributeType");
        vtkPVXMLElement* valueElement =
          child->FindNestedElementByName("Element");
        if (valueElement)
          {
          int oldValue = 0;
          valueElement->GetScalarAttribute("value", &oldValue);
          int newValue = (oldValue<=3)? 0 : 1;
          vtksys_ios::ostringstream valueStr;
          valueStr << newValue << ends;
          valueElement->SetAttribute("value", valueStr.str().c_str());
          }
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
// Called for every XYPlotDisplay2. The point and cell array status
// arrays changed order and added elements.
bool ConvertLineSeriesArrayStatus(vtkPVXMLElement* root,
  void* vtkNotUsed(callData))
{
  // If there are less than 5 child elements, there is nothing to change.
  unsigned int max = root->GetNumberOfNestedElements();
  for(unsigned int i = 0; i < max; i++)
    {
    vtkPVXMLElement *child = root->GetNestedElement(i);
    const char *name = child ? child->GetName() : 0;
    if(!name || strcmp(name, "Property") != 0)
      {
      continue;
      }

    name = child->GetAttribute("name");
    if(name && (strcmp(name, "YCellArrayStatus") == 0 ||
        strcmp(name, "YPointArrayStatus") == 0))
      {
      unsigned int total = child->GetNumberOfNestedElements();
      if(total <= 1)
        {
        continue;
        }

      // Find the domain element. It should be last.
      vtkSmartPointer<vtkPVXMLElement> domainElement =
          child->GetNestedElement(total - 1);
      name = domainElement ? domainElement->GetName() : 0;
      if(!name || strcmp(name, "Domain") != 0)
        {
        continue;
        }

      // Remove the domain element from the list.
      child->RemoveNestedElement(domainElement);

      // Add elements for the new array entries.
      total -= 1;
      unsigned int newTotal = total * 2; //(total / 5) * 10;
      vtkPVXMLElement *elem = 0;
      for(unsigned int index = total; index < newTotal; index++)
        {
        elem = vtkPVXMLElement::New();
        elem->SetName("Element");
        elem->AddAttribute("index", index);
        elem->AddAttribute("value", "");
        child->AddNestedElement(elem);
        elem->Delete();
        }

      // Add the domain element back in.
      child->AddNestedElement(domainElement);
      domainElement = 0;

      // Move original data at old index to new index:
      // 0 --> 4
      // 1 --> 5
      // 2 --> 6
      // 3 --> 2
      // 4 --> 0
      // 4 --> 1
      //
      // Fill in the rest with new data:
      // inlegend(new index 3) = 1 (int)
      // thickness(new index 7) = 1 (int)
      // linestyle(new index 8) = 1 (int)
      // axesindex(new index 9) = 0 (int)
      //
      // Start at the end where there is space.
      int j = total - 5;
      int k = newTotal - 10;
      vtkPVXMLElement *elem2 = 0;
      for(; j >= 0 && k >= 0; j -= 5, k -= 10)
        {
        // Fill in the new options.
        elem = child->GetNestedElement(k + 7);
        elem->SetAttribute("value", "1");
        elem = child->GetNestedElement(k + 8);
        elem->SetAttribute("value", "1");
        elem = child->GetNestedElement(k + 9);
        elem->SetAttribute("value", "0");

        // Move the green and blue from 1,2 to 5,6.
        elem = child->GetNestedElement(j + 1);
        elem2 = child->GetNestedElement(k + 5);
        elem2->SetAttribute("value", elem->GetAttribute("value"));
        elem = child->GetNestedElement(j + 2);
        elem2 = child->GetNestedElement(k + 6);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Move the array/legend name to from 4 to 1.
        elem = child->GetNestedElement(j + 4);
        elem2 = child->GetNestedElement(k + 1);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Move the red from 0 to 4.
        elem = child->GetNestedElement(j + 0);
        elem2 = child->GetNestedElement(k + 4);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Copy the array/legend name to 0.
        elem = child->GetNestedElement(k + 1);
        elem2 = child->GetNestedElement(k + 0);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Copy the enabled flag from 3 to 2.
        elem = child->GetNestedElement(j + 3);
        elem2 = child->GetNestedElement(k + 2);
        elem2->SetAttribute("value", elem->GetAttribute("value"));

        // Set the in legend flag to true.
        elem = child->GetNestedElement(k + 3);
        elem->SetAttribute("value", "1");
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool ConvertPVAnimationSceneToAnimationScene(
  vtkPVXMLElement* root, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  return self->ConvertPVAnimationSceneToAnimationScene(root);
}

//----------------------------------------------------------------------------
bool ConvertStreamTracer( vtkPVXMLElement * root, void * callData )
{
  vtkSMStateVersionController * self
    = reinterpret_cast< vtkSMStateVersionController * >( callData );
  return self->ConvertStreamTracer( root );
}

//----------------------------------------------------------------------------
bool ConvertExodusIIReader( vtkPVXMLElement * root, void * callData )
{
  vtkSMStateVersionController * self
    = reinterpret_cast< vtkSMStateVersionController * >( callData );
  return self->ConvertExodusIIReader( root );
}

}; // namespace end


//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_0_To_3_2(vtkPVXMLElement* root)
{
    {
    // Remove all MultiViewRenderModule elements.
    const char* attrs[] = {
      "group","rendermodules",
      "type", "MultiViewRenderModule", 0};
    this->SelectAndRemove( root, "Proxy",attrs);
    }

    {
    // Replace all requests for proxies in the group "rendermodules" by
    // "views".
    const char* attrs[] = {
      "group", "rendermodules", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "RenderView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "plotmodules",
      "type", "BarChartViewModule", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "BarChartView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert different kinds of old-views to new-views.
    const char* attrs[] = {
      "group", "plotmodules",
      "type", "XYPlotViewModule", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "XYPlotView", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertViewModulesToViews,
      this);
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

    {
    // Remove element inspector view.
    const char* attrs[] = {
      "group", "views",
      "type", "ElementInspectorView", 0};
    const char* newAttrs[] = {
      "group", "views",
      "type", "ElementInspectorView", 0};
    this->SelectAndRemove(root, "Proxy", attrs);
    this->SelectAndRemove(root, "Proxy", newAttrs);
    }

    {
    // Convert all geometry displays to representations.
    const char* lodAttrs[] = {
      "type", "LODDisplay", 0};
    const char* compositeAttrs[] = {
      "type", "CompositeDisplay", 0};
    const char* multiAttrs[] = {
      "type", "MultiDisplay", 0};

    this->Select(root, "Proxy", lodAttrs,
      &ConvertDataDisplaysToRepresentations, this);
    this->Select(root, "Proxy", compositeAttrs,
      &ConvertDataDisplaysToRepresentations, this);
    this->Select(root, "Proxy", multiAttrs,
      &ConvertDataDisplaysToRepresentations, this);
    }

    {
    // Convert registration groups from "displays" to
    // "representations"
    const char* attrs[] = {
      "name", "displays", 0};
    const char* newAttrs[] = {
      "name", "representations", 0};
    this->SelectAndSetAttributes(root, "ProxyCollection", attrs, newAttrs);
    }

    {
    // Convert registration groups from "view_modules" to
    // "views"
    const char* attrs[] = {
      "name", "view_modules", 0};
    const char* newAttrs[] = {
      "name", "views", 0};
    this->SelectAndSetAttributes(root, "ProxyCollection", attrs, newAttrs);
    }

    {
    // Convert all text widget displays to representations.
    const char* attrs[] = {
      "type", "TextWidgetDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "TextWidgetRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all GenericViewDisplay displays to representations.
    const char* attrs[] = {
      "type", "GenericViewDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ClientDeliveryRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "BarChartDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "BarChartRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "XYPlotDisplay2", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "XYPlotRepresentation", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertLineSeriesArrayStatus, this);
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all displays to representations.
    const char* attrs[] = {
      "type", "ScalarBarWidget", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ScalarBarWidgetRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Remove element inspector representations.
    const char* attrs[] = {
      "type", "ElementInspectorDisplay", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "ElementInspectorRepresentation", 0};
    this->SelectAndRemove(root, "Proxy", attrs);
    this->SelectAndRemove(root, "Proxy", newAttrs);
    }

    {
    // Convert Axes to AxesRepresentation.
    const char* attrs[] = {
      "group", "axes",
      "type", "Axes", 0};
    const char* newAttrs[] = {
      "group", "representations",
      "type", "AxesRepresentation", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Delete all obsolete display types.
    const char* attrsCAD[] = {
      "type", "CubeAxesDisplay",0};
    const char* attrsPLD[] = {
      "type", "PointLabelDisplay",0};

    this->SelectAndRemove(root, "Proxy", attrsCAD);
    this->SelectAndRemove(root, "Proxy", attrsPLD);
    }


    {
    // Select all LegacyVTKFileReader elements
    const char* attrs[] = {
      "type", "legacyreader", 0};
    this->Select( root, "Proxy", attrs,
      &::ConvertLegacyReader, this);
    }

    {
    // Convert all legacyreader proxies to LegacyVTKFileReader
    const char* attrs[] = {
      "type", "legacyreader", 0};
    const char* newAttrs[] = {
      "type", "LegacyVTKFileReader", 0};
    this->SelectAndSetAttributes(
      root, "Proxy", attrs, newAttrs);
    }

    {
    // Convert all PVAnimationScene proxies to AnimationScene proxies.
    const char* attrs[] = {
      "type", "PVAnimationScene", 0};
    this->Select(
      root, "Proxy", attrs,
      &::ConvertPVAnimationSceneToAnimationScene, this);
    }


  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertViewModulesToViews(
  vtkPVXMLElement* parent)
{
  const char* attrs[] = {
    "name", "Displays", 0 };
  const char* newAttrs[] = {
    "name", "Representations", 0};
  // Replace the "Displays" property with "Representations".
  this->SelectAndSetAttributes(
    parent,
    "Property", attrs, newAttrs);

  // Convert property RenderWindowSize to ViewSize.
  const char* propAttrs[] = {
    "name", "RenderWindowSize", 0};
  const char* propNewAttrs[] = {
    "name", "ViewSize", 0};
  this->SelectAndSetAttributes(
    parent,
    "Property", propAttrs, propNewAttrs);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertLegacyReader(
  vtkPVXMLElement* parent)
{
  const char* attrs[] = {
    "name", "FileName", 0 };
  const char* newAttrs[] = {
    "name", "FileNames", 0};
  // Replace the "FileName" property with "FileNames".
  this->SelectAndSetAttributes(
    parent,
   "Property", attrs, newAttrs);

  return true;
}


//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertPVAnimationSceneToAnimationScene(
  vtkPVXMLElement* parent)
{
  parent->SetAttribute("type", "AnimationScene");

  // ClockTimeRange property changed to "StartTime" and "EndTime"
  vtksys_ios::ostringstream idStr;
  idStr << parent->GetAttribute("id") << ".ClockTimeRange";
  vtkPVXMLElement* ctRange = parent->FindNestedElement(idStr.str().c_str());
  vtkSmartPointer<vtkCollection> elements = vtkSmartPointer<vtkCollection>::New();
  if (ctRange)
    {
    ctRange->GetElementsByName("Element", elements);
    }
  if (elements->GetNumberOfItems() == 2)
    {
    vtkPVXMLElement* startTime = vtkPVXMLElement::New();
    startTime->SetName("Property");
    startTime->SetAttribute("name", "StartTime");
    startTime->SetAttribute("number_of_elements", "1");
    vtksys_ios::ostringstream idST;
    idST << parent->GetAttribute("id") << ".StartTime";
    startTime->SetAttribute("id", idST.str().c_str());

    vtkPVXMLElement* element =
      vtkPVXMLElement::SafeDownCast(elements->GetItemAsObject(0));
    ctRange->RemoveNestedElement(element);
    startTime->AddNestedElement(element);

    parent->AddNestedElement(startTime);
    startTime->Delete();

    vtkPVXMLElement* endTime = vtkPVXMLElement::New();
    endTime->SetName("Property");
    endTime->SetAttribute("name", "EndTime");
    endTime->SetAttribute("number_of_elements", "1");
    vtksys_ios::ostringstream idET;
    idET << parent->GetAttribute("id") << ".EndTime";
    endTime->SetAttribute("id", idET.str().c_str());

    element = vtkPVXMLElement::SafeDownCast(elements->GetItemAsObject(1));
    ctRange->RemoveNestedElement(element);
    element->SetAttribute("index", "0");
    endTime->AddNestedElement(element);

    parent->AddNestedElement(endTime);
    endTime->Delete();

    parent->RemoveNestedElement(ctRange);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertExodusIIReader(vtkPVXMLElement* root)
{
  unsigned int numElements = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElements; cc++)
    {
    vtkPVXMLElement* prop = root->GetNestedElement(cc);
    for (unsigned int dd=0; dd < prop->GetNumberOfNestedElements(); dd++)
      {
      vtkPVXMLElement* blockProp = prop->GetNestedElement(dd);
      const char* blockPropName = blockProp->GetAttribute("name");
      if(blockPropName && strcmp(blockPropName,"array_list")==0)
        {
        for (unsigned int ee=0; ee < blockProp->GetNumberOfNestedElements(); ee++)
          {
          vtkPVXMLElement* listProp = blockProp->GetNestedElement(ee);
          const char* value = listProp->GetAttribute("text");
          if(value)
            {
            vtkStdString bname(value);
            size_t i = bname.find(" Size: ");
            if(i!= vtkStdString::npos)
              {
              bname.erase(i);
              listProp->SetAttribute("value",bname);
              }
            }
          }
        }
      else
        {
        const char* value = blockProp->GetAttribute("value");
        if(value)
          {
          vtkStdString bname(value);
          size_t i = bname.find(" Size: ");
          if(i!= vtkStdString::npos)
            {
            bname.erase(i);
            blockProp->SetAttribute("value",bname);
            }
          }
        }
      }
    }
  return true;
}

namespace
{
//----------------------------------------------------------------------------
bool ConvertTemporalShiftScale(
  vtkPVXMLElement* parent, void* callData)
{
  vtkSMStateVersionController* self = reinterpret_cast<
    vtkSMStateVersionController*>(callData);
  const char* attrs[] = {
    "name", "Shift", 0 };
  const char* newAttrs[] = {
    "name", "PostShift", 0};
  // Replace the "FileName" property with "FileNames".
  self->SelectAndSetAttributes(
    parent,
   "Property", attrs, newAttrs);

  return true;
}
};

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_2_To_3_4(vtkPVXMLElement* root)
{
  {
  // Select all LegacyVTKFileReader elements
  const char* attrs[] = {
    "type", "TemporalShiftScale", 0};
  this->Select( root, "Proxy", attrs,
    &::ConvertTemporalShiftScale, this);
  }

  return true;
}

bool ElementFound(
  vtkPVXMLElement* vtkNotUsed(parent), void* callData)
{
  *(reinterpret_cast<bool*>(callData)) = true;
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_4_to_3_6(vtkPVXMLElement* root)
{
    {
    // If there's a "CSVReader", it's an irrecoverable error since the output
    // format for CSVReader changed in 3.6 to be true table so old rectilinear
    // grid pipelines won't really work.
    const char* attrs[] = {"type", "CSVReader", 0};
    bool found = false;
    this->Select(root, "Proxy", attrs, &::ElementFound, &found);
    if (found)
      {
      vtkErrorMacro("Your state file uses a \"CSVReader\"."
        "The CSVReader has undergone major changes in 3.6 and hence this state"
        " file is not recoverable.");
      return false;
      }
    }

    {
    // "CTHFragmentConnect" was removed from the open source version due to
    // export control issues.
    const char* attrs[] = {"type", "CTHFragmentConnect", 0};
    bool found = false;
    this->Select(root, "Proxy", attrs, &::ElementFound, &found);
    if (found)
      {
      vtkErrorMacro("Your state file uses a \"CTHFragmentConnect\"."
        "CTHFragmentConnect is no longer available in ParaView.");
      return false;
      }
    }

    {
    // "CTHFragmentIntersect" was removed from the open source version due to
    // export control issues.
    const char* attrs[] = {"type", "CTHFragmentIntersect", 0};
    bool found = false;
    this->Select(root, "Proxy", attrs, &::ElementFound, &found);
    if (found)
      {
      vtkErrorMacro("Your state file uses a \"CTHFragmentIntersect\"."
        "CTHFragmentIntersect is no longer available in ParaView.");
      return false;
      }
    }

    {
    // If Plot Views are present, warn about them. Suggest that they may have to
    // recreate the view.
    const char* attrs0[] = {"type", "XYPlotView", 0};
    const char* attrs1[] = {"type", "BarChartView", 0};
    bool found = false;
    this->Select(root, "Proxy", attrs0, &::ElementFound, &found);
    if (!found)
      {
      this->Select(root, "Proxy", attrs1, &::ElementFound, &found);
      }
    if (found)
      {
      vtkWarningMacro("Your state file uses plot views. "
        "Plot views have undergone considerable changes in 3.6 and it's"
        " possible that the state may not be loaded correctly. "
        "In that case, simply close the plot views, and recreate them.");
      }
    }

    {
    // If "ExodusReader" is present, we give up since legacy exodus is no longer
    // supported.
    const char* attrs[] = {"type", "ExodusReader", 0};
    bool found = false;
    this->Select(root, "Proxy", attrs, &::ElementFound, &found);
    if (found)
      {
      vtkErrorMacro("Your state file uses a \"ExodusReader\"."
        " ExodusReader was replaced by ExodusIIReader in 3.6."
        " We are unable to support legacy exodus state files.");
      return false;
      }
    }

    {
    // Convert "Programmable Filter" to "ProgrammableFilter".
    const char* attrs[] = {"type", "Programmable Filter", 0};
    const char* newAttrs[] = {"type", "ProgrammableFilter", 0};
    this->SelectAndSetAttributes(root, "Proxy", attrs, newAttrs);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertStreamTracer(vtkPVXMLElement* parent)
{
  // Replace property "InitialIntegrationStepUnit" with property
  // "IntegrationStepUnit", ignoring properties "MinimumIntegrationStepUnit",
  // "MaximumIntegrationStepUnit", and "MaximumPropagationUnit".
  const char* attrs[]    = { "name", "InitialIntegrationStepUnit", 0 };
  const char* newAttrs[] = { "name", "IntegrationStepUnit",        0 };
  this->SelectAndSetAttributes( parent, "Property", attrs, newAttrs );
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_6_to_3_8( vtkPVXMLElement * root )
{
    {
    // Upon any occurrence of StreamTracer or ArbitrarySourceStreamTracer, a
    // warning message pops up, indicating the changes to vtkStreamTracer
    bool found0 = false;
    bool found1 = false;
    const char * attrs0[] = { "type", "StreamTracer",                0 };
    const char * attrs1[] = { "type", "ArbitrarySourceStreamTracer", 0 };
    this->Select( root, "Proxy", attrs0, &::ElementFound, &found0 );
    this->Select( root, "Proxy", attrs1, &::ElementFound, &found1 );

    if ( found0 || found1 )
      {
      vtkWarningMacro( "Your state file uses (vtk)StreamTracer. "
        "vtkStreamTracer has undergone considerable changes in 3.8 and it's"
        " possible that the state may not be loaded correctly or some of"
        " the settings may be converted to values other than specified." );
      }
    }

    {
    // Replace abandoned properties of vtkStreamTracer with new ones
    const char* attrs0[] = { "type", "StreamTracer",                0 };
    const char* attrs1[] = { "type", "ArbitrarySourceStreamTracer", 0 };
    this->Select( root, "Proxy", attrs0, &::ConvertStreamTracer, this );
    this->Select( root, "Proxy", attrs1, &::ConvertStreamTracer, this );
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_8_to_3_10( vtkPVXMLElement * root )
{
    {
    // Remove all AxesRepresentation elements.
    const char* attrs[] = {
      "group","representations",
      "type", "AxesRepresentation", 0};
    this->SelectAndRemove( root, "Proxy",attrs);
    }
  return true;
}

namespace
{
  bool ConvertRepresentationProperty(vtkPVXMLElement* element, void* callData)
    {
    vtkSMStateVersionController * self
      = reinterpret_cast< vtkSMStateVersionController * >( callData );
    return self->ConvertRepresentationProperty(element);
    }
};

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_10_to_3_12( vtkPVXMLElement * root )
{
    {
    // Change all "Representation" properties from ints to string.
    const char* attrs[] = {
      "group","representations",
      "type", "GeometryRepresentation", 0};
    this->Select( root, "Proxy",attrs, &::ConvertRepresentationProperty, this);

    const char* attrs2[] = {
      "group","representations",
      "type", "UnstructuredGridRepresentation", 0};
    this->Select( root, "Proxy",attrs2, &::ConvertRepresentationProperty, this);

    const char* attrs3[] = {
      "group","representations",
      "type", "UniformGridRepresentation", 0};
    this->Select( root, "Proxy",attrs3, &::ConvertRepresentationProperty, this);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_12_to_3_14(
  vtkPVXMLElement* smroot, vtkPVXMLElement* pvroot)
{
  // Handle <ViewManager/> element. Convert it to a "ViewLayout" proxy state.
  {
  vtkPVXMLElement* viewManager = pvroot->FindNestedElementByName("ViewManager");
  if (viewManager)
    {
    vtkPVXMLElement* viewLayout = this->ConvertMultiViewLayout(viewManager);
    smroot->AddNestedElement(viewLayout);
    viewLayout->Delete();

    vtkPVXMLElement* item = vtkPVXMLElement::New();
    item->SetName("Item");
    item->AddAttribute("id", "1");
    item->AddAttribute("name", "ViewLayout1");

    vtkPVXMLElement* layouts = vtkPVXMLElement::New();
    layouts->SetName("ProxyCollection");
    layouts->AddAttribute("name", "layouts");
    layouts->AddNestedElement(item);
    item->Delete();

    smroot->AddNestedElement(layouts);
    layouts->Delete();
    }
  }
  return true;
}

namespace
{
  vtkPVXMLElement* FindNestedSplitterAtIndex(vtkPVXMLElement* node, int index)
    {
    for (unsigned int cc=0;
        node != NULL && cc < node->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* child = node->GetNestedElement(cc);
      int cur_index;
      if (child && child->GetScalarAttribute("index", &cur_index) &&
        cur_index == index)
        {
        return child;
        }
      }
    return NULL;
    }

  void HandleSplitterElements(int index, vtkPVXMLElement* node,
    std::vector<vtkSmartPointer<vtkPVXMLElement> > &items)
    {
    if (static_cast<int>(items.size()) < index+1)
      {
      items.resize(index+1);
      }
    items[index] = vtkSmartPointer<vtkPVXMLElement>::New();
    items[index]->SetName("Item");

    if (node && strcmp(node->GetName(), "Splitter") == 0)
      {
      // if splitter has just 1 view, then we aren't really splitting anything.
      // Handle that first.
      int count;
      if (node->GetScalarAttribute("count", &count) && count==1)
        {
        HandleSplitterElements(index, NULL, items);
        return;
        }

      if (node->GetAttribute("orientation") &&
        strcmp(node->GetAttribute("orientation"), "Horizontal")==0)
        {
        items[index]->SetAttribute("direction", "2");
        }
      else
        {
        items[index]->SetAttribute("direction", "1");
        }
      if (node->GetAttribute("sizes"))
        {
        vtksys::RegularExpression reg_ex("([0-9]+).([0-9]+)");
        if (reg_ex.find(node->GetAttribute("sizes")))
          {
          int x1 = atoi(reg_ex.match(1).c_str());
          int x2 =  atoi(reg_ex.match(2).c_str());
          double fraction = static_cast<double>(x1)/(x1 + x2);
          items[index]->AddAttribute("fraction", fraction);
          }
        }
      items[index]->AddAttribute("view", "0");
      // find nest element with index 0 and 1.
      HandleSplitterElements(2*index + 1, FindNestedSplitterAtIndex(node, 0), items);
      HandleSplitterElements(2*index + 2, FindNestedSplitterAtIndex(node, 1), items);
      }
    else
      {
      items[index]->AddAttribute("direction", "0");
      items[index]->AddAttribute("fraction", "0.5");
      items[index]->AddAttribute("view", "0");
      }
    }
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMStateVersionController::ConvertMultiViewLayout(
  vtkPVXMLElement* viewManager)
{
  vtkPVXMLElement* multiView = viewManager->FindNestedElementByName("MultiView");
  std::vector<vtkSmartPointer<vtkPVXMLElement> > items;
  HandleSplitterElements(0, multiView->FindNestedElementByName("Splitter"), items);

  // now handle "Frame elements;
  for (unsigned int cc=0; cc < viewManager->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* child = viewManager->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Frame") == 0)
      {
      std::vector<vtksys::String> indexes = vtksys::SystemTools::SplitString(
        child->GetAttribute("index"), '.');
      int index = 0;
      for (size_t kk=0; kk < indexes.size(); kk++)
        {
        int val = atoi(indexes[kk].c_str());
        index = val==0? (2*index + 1) : (2*index+2);
        }
      // handle no-split explicitly.
      if (index==1 && indexes.size() == 1 && items.size() == 1)
        {
        index=0;
        }
      items[index]->SetAttribute("view", child->GetAttribute("view_module"));
      }
    }

  vtkPVXMLElement* layout = vtkPVXMLElement::New();
  layout->SetName("Layout");
  layout->AddAttribute("number_of_elements", static_cast<int>(items.size()));
  for (size_t cc=0; cc < items.size(); cc++)
    {
    if (items[cc])
      {
      layout->AddNestedElement(items[cc]);
      }
    else
      {
      vtkPVXMLElement* item = vtkPVXMLElement::New();
      item->SetName("Item");
      item->AddAttribute("direction","0");
      item->AddAttribute("fraction", "0.5");
      item->AddAttribute("view", "0");
      layout->AddNestedElement(item);
      item->Delete();
      }
    }
  //layout->PrintXML();

  vtkPVXMLElement* proxy = vtkPVXMLElement::New();
  proxy->SetName("Proxy");
  proxy->AddNestedElement(layout);
  layout->Delete();

  proxy->AddAttribute("group", "misc");
  proxy->AddAttribute("type", "ViewLayout");
  proxy->AddAttribute("id", "1");
  proxy->AddAttribute("servers", "5");
  return proxy;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertRepresentationProperty(
  vtkPVXMLElement* element)
{
  if (element->GetName() && strcmp(element->GetName(), "Proxy") ==0)
    {
    const char* attrs[] = {"name", "Representation", 0};
    this->Select(element, "Property", attrs, &::ConvertRepresentationProperty,
      this);
    return true;
    }
  else if (element->GetName() && strcmp(element->GetName(), "Property") == 0)
    {
    std::string newtext;
    int value = 0;
    vtkPVXMLElement* valueElement = NULL;
    vtkPVXMLElement* domainElement = NULL;
    for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* child = element->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "Element")==0)
        {
        if (!child->GetScalarAttribute("value", &value))
          {
          // the value is already a string. State must have been saved between
          // 3.10 and 4.0. All's well.
          return true;
          }
        valueElement = child;
        }
      else if (child && child->GetName() && strcmp(child->GetName(), "Domain")==0)
        {
        domainElement = child;
        }
      }
    if (!domainElement || !valueElement)
      {
      return false;
      }
    for (unsigned int cc=0; cc < domainElement->GetNumberOfNestedElements(); cc++)
      {
      vtkPVXMLElement* child = domainElement->GetNestedElement(cc);
      if (child && child->GetName() && strcmp(child->GetName(), "Entry")==0)
        {
        int dvalue=-1;
        if (child->GetAttribute("text") &&
          child->GetScalarAttribute("value", &dvalue) &&
          dvalue == value)
          {
          newtext = child->GetAttribute("text");
          break;
          }
        }
      }
    valueElement->SetAttribute("value", newtext.c_str());
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_3_14_to_4_0(vtkPVXMLElement* root)
{
  // For 2D views we simply remove the InteractionMode property element to rely
  // on the default value of the proxy definition itself
  if (root)
    {
    const char* twoDRenderType = "2DRenderView";
    const char* interactionModeName = "InteractionMode";
    const char* currentType;
    const char* currentName;
    unsigned int nbElements = root->GetNumberOfNestedElements();
    for(unsigned int elemIndex = 0; elemIndex < nbElements; elemIndex++)
      {
      vtkPVXMLElement* proxy = root->GetNestedElement(elemIndex);
      if( (currentType = proxy->GetAttribute("type")) &&
        !strcmp(twoDRenderType, currentType))
        {
        for( unsigned int propIndex = 0;
          propIndex < proxy->GetNumberOfNestedElements();
          propIndex++)
          {
          vtkPVXMLElement* property = proxy->GetNestedElement(propIndex);
          if( (currentName = property->GetAttribute("name")) &&
            !strcmp(interactionModeName, currentName))
            {
            proxy->RemoveNestedElement(property);
            break;
            }
          }
        }
      }
    }

  // ExodusIIReader now uses shorter name without "Size: ... "
  // This deals with old state files
  if(root)
    {
    const char* attrs[] = {"group","sources","type","ExodusIIReader", 0};
    this->Select( root, "Proxy",attrs, &::ConvertExodusIIReader, this);
    }
  return true;
}

namespace
{
  bool ConvertCTHPartProperties(vtkPVXMLElement* element, void* callData)
    {
    vtkSMStateVersionController * self
      = reinterpret_cast< vtkSMStateVersionController * >( callData );
    return self->ConvertCTHPartProperties(element);
    }
};

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::ConvertCTHPartProperties(vtkPVXMLElement* root)
{
  // find property elements for AddFloatVolumeArrayName,
  // AddDoubleVolumeArrayName, AddUnsignedCharVolumeArrayName.
  std::set<std::string> names;
  std::vector<vtkPVXMLElement*> to_remove;
  for (unsigned int cc=0; cc < root->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child->GetName() && strcmp(child->GetName(), "Property")==0 &&
      child->GetAttribute("name") != 0 &&
      (strcmp(child->GetAttribute("name"), "AddFloatVolumeArrayName") == 0 ||
       strcmp(child->GetAttribute("name"), "AddDoubleVolumeArrayName") == 0 ||
       strcmp(child->GetAttribute("name"), "AddUnsignedCharVolumeArrayName") == 0))
      {
      to_remove.push_back(child);

      for (unsigned int kk=0; kk < child->GetNumberOfNestedElements(); kk++)
        {
        vtkPVXMLElement* element = child->GetNestedElement(kk);
        if (element->GetName() && strcmp(element->GetName(), "Element") == 0 &&
          element->GetAttribute("value") != 0)
          {
          names.insert(element->GetAttribute("value"));
          }
        }
      }
    }
  for (size_t cc=0; cc < to_remove.size(); cc++)
    {
    root->RemoveNestedElement(to_remove[cc]);
    }
  to_remove.clear();

  if (names.size() > 0)
    {
    vtkNew<vtkPVXMLElement> child;
    child->SetName("Property");
    child->SetAttribute("name", "VolumeArrays");
    int index = 0;
    for (std::set<std::string>::iterator iter=names.begin();
      iter != names.end(); ++iter, ++index)
      {
      vtkNew<vtkPVXMLElement> element;
      element->SetName("Element");
      element->AddAttribute("index", index);
      element->AddAttribute("value", iter->c_str());
      child->AddNestedElement(element.GetPointer());
      }
    root->AddNestedElement(child.GetPointer());
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMStateVersionController::Process_4_0_to_next(vtkPVXMLElement* root)
{
  if (!root)
    {
    return false;
    }

  // For CTHPart filter, we need to convert AddDoubleVolumeArrayName,
  // AddUnsignedCharVolumeArrayName and AddFloatVolumeArrayName to a single
  // VolumeArrays property. The type specific separation is no longer
  // needed.

  const char* attrs[] = {
    "group","filters",
    "type", "CTHPart", 0};
  this->Select( root, "Proxy",attrs, &::ConvertCTHPartProperties, this);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
