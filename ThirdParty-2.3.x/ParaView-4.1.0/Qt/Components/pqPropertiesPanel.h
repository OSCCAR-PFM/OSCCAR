/*=========================================================================

   Program: ParaView
   Module:  pqPropertiesPanel.h

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
#ifndef __pqPropertiesPanel_h
#define __pqPropertiesPanel_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqDataRepresentation;
class pqOutputPort;
class pqPipelineSource;
class pqPropertyWidget;
class pqView;
class vtkSMProperty;
class vtkSMProxy;

/// pqPropertiesPanel is the default panel used by paraview to edit source
/// properties and display properties for pipeline objects. pqPropertiesPanel
/// supports auto-generating widgets for properties of the proxy as well as a
/// mechanism to provide custom widgets/panels for the proxy or its
/// representations. pqPropertiesPanel uses pqProxyWidget to create and manage
/// the widgets for the source and representation proxies.
class PQCOMPONENTS_EXPORT pqPropertiesPanel : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqPropertiesPanel(QWidget *parent = 0);
  virtual ~pqPropertiesPanel();

  /// Enable/disable auto-apply.
  static void setAutoApply(bool enabled);

  /// Returns \c true if auto-apply is enabled.
  static bool autoApply();

  /// Sets the delay for auto-apply to \p delay (in msec).
  static void setAutoApplyDelay(int delay);

  /// Returns the delay for the auto-apply (in msec).
  static int autoApplyDelay();

  /// Returns the current view, if any.
  pqView* view() const;

  /// methods used to obtain the recommended spacing and margins to be used for
  /// widgets.
  static int suggestedMargin() { return 0; }
  static int suggestedHorizontalSpacing() { return 4; }
  static int suggestedVerticalSpacing() { return 4; }

public slots:
  /// Apply the changes properties to the proxies.
  ///
  /// This is triggered when the user clicks the "Apply" button on the
  /// properties panel.
  void apply();

  /// Reset the changes made.
  ///
  /// This is triggered when the user clicks the "Reset" button on the
  /// properties panel.
  void reset();

  /// Deletes the current proxy.
  ///
  /// This is triggered when the user clicks the "Delete" button on the
  /// properties panel.
  void deleteProxy();

  /// Shows the help dialog.
  ///
  /// This is triggered when the user clicks the "?" button on the
  /// properties panel.
  void showHelp();

signals:
  /// This signal is emitted after the user clicks the apply button.
  void applied();

  /// This signal is emitted when the current view changes.
  void viewChanged(pqView*);

  /// This signal is emitted when the user clicks the help button.
  void helpRequested(const QString &groupname, const QString &proxyType);

private slots:
  void setView(pqView*);
  void setOutputPort(pqOutputPort*);
  void setRepresentation(pqDataRepresentation*);

  /// slot gets called when a proxy is deleted.
  void proxyDeleted(pqPipelineSource*);

  /// Updates the entire panel (properties+display) using the current
  /// port/representation.
  void updatePanel();

  /// Updates the display part of the panel alone, unlike updatePanel().
  void updateDisplayPanel();

  /// renders the view, if any.
  void renderActiveView();

  /// Called when a property on the current proxy changes.
  void sourcePropertyChanged(bool change_finished=true);
  void sourcePropertyChangeAvailable()
    { this->sourcePropertyChanged(false); }

  /// Updates the state of all the buttons, apply/reset/delete.
  void updateButtonState();

protected:
  /// Update the panel to show the widgets for the given pair.
  void updatePanel(pqOutputPort* port);
  void updatePropertiesPanel(pqPipelineSource* source);
  void updateDisplayPanel(pqDataRepresentation* repr);

private:
  static bool AutoApply;
  static int AutoApplyDelay;

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;

  Q_DISABLE_COPY(pqPropertiesPanel)
};

#endif
