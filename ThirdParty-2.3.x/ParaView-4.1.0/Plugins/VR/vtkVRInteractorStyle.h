/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef __vtkVRInteractorStyle_h
#define __vtkVRInteractorStyle_h

#include <vtkObject.h>
#include <vtkStdString.h> // For vtkStdString

#include <map>
#include <vector>

class vtkPVXMLElement;
class vtkSMProxyLocator;
class vtkSMProxy;
class vtkSMDoubleVectorProperty;
class vtkStringList;
struct vtkVREventData;

class vtkVRInteractorStyle : public vtkObject
{
public:
  static vtkVRInteractorStyle *New();
  vtkTypeMacro(vtkVRInteractorStyle, vtkObject)
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get the vector size of the controlled property this style expects, e.g. a
  // 4x4 matrix will be 16, a 3D vector will be 3, etc. This is used to limit
  // the number of options presented to the user when prompting for a property.
  // This is NOT checked internally by SetControlledPropertyName.
  //
  // A value of -1 means no filtering will be done, and all available properties
  // will be shown.
  virtual int GetControlledPropertySize() { return -1; }

  virtual void SetControlledProxy(vtkSMProxy *);
  vtkGetObjectMacro(ControlledProxy, vtkSMProxy)

  vtkSetStringMacro(ControlledPropertyName)
  vtkGetStringMacro(ControlledPropertyName)

  virtual bool HandleEvent(const vtkVREventData& data);
  virtual bool Update();

  // Description:
  // Get a list of defined roles for each output type.
  void GetAnalogRoles(vtkStringList *);
  void GetButtonRoles(vtkStringList *);
  void GetTrackerRoles(vtkStringList *);

  // Description:
  // Get the number of roles defined for each output type.
  int GetNumberOfAnalogRoles();
  int GetNumberOfButtonRoles();
  int GetNumberOfTrackerRoles();

  // Description:
  // Get the role of the input with the given name. If the name is not
  // set or recognized, an empty string is returned.
  vtkStdString GetAnalogRole(const vtkStdString &name);
  vtkStdString GetButtonRole(const vtkStdString &name);
  vtkStdString GetTrackerRole(const vtkStdString &name);

  // Description:
  // Set/Get the name of the input that fulfills the specified role.
  bool SetAnalogName(const vtkStdString &role, const vtkStdString &name);
  vtkStdString GetAnalogName(const vtkStdString &role);
  bool SetButtonName(const vtkStdString &role, const vtkStdString &name);
  vtkStdString GetButtonName(const vtkStdString &role);
  bool SetTrackerName(const vtkStdString &role, const vtkStdString &name);
  vtkStdString GetTrackerName(const vtkStdString &role);

  /// Load state for the style from XML.
  virtual bool Configure(vtkPVXMLElement* child, vtkSMProxyLocator*);

  /// Save state to xml.
  virtual vtkPVXMLElement* SaveConfiguration() const;

protected:
  vtkVRInteractorStyle();
  virtual ~vtkVRInteractorStyle();

  virtual void HandleButton ( const vtkVREventData& data );
  virtual void HandleAnalog ( const vtkVREventData& data );
  virtual void HandleTracker( const vtkVREventData& data );

  static std::vector<std::string> Tokenize( std::string input);

  vtkSMProxy *ControlledProxy;
  char *ControlledPropertyName;

  // Description:
  // Add a new input role to the interactor style.
  void AddAnalogRole(const vtkStdString &role);
  void AddButtonRole(const vtkStdString &role);
  void AddTrackerRole(const vtkStdString &role);

  typedef std::map<std::string, std::string> StringMap;
  StringMap Analogs;
  StringMap Buttons;
  StringMap Trackers;
  void MapKeysToStringList(const StringMap &source,
                           vtkStringList *target);
  bool SetValueInMap(StringMap &map_,
                     const vtkStdString &key, const vtkStdString &value);
  vtkStdString GetValueInMap(const StringMap &map_,
                             const vtkStdString &key);
  vtkStdString GetKeyInMap(const StringMap &map_,
                           const vtkStdString &value);

private:
  vtkVRInteractorStyle(const vtkVRInteractorStyle&); // Not implemented.
  void operator=(const vtkVRInteractorStyle&); // Not implemented.
};

#endif
