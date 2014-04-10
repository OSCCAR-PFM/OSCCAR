/*=========================================================================

   Program: ParaView
   Module:    pqStandardViewFrameActionGroup.h

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
#ifndef __pqStandardViewFrameActionGroup_h 
#define __pqStandardViewFrameActionGroup_h

#include "pqViewFrameActionGroup.h"
#include "pqApplicationComponentsModule.h" // needed for export macros
#include <QPointer> // needed for QPointer 

class QWidget;
class pqViewFrame;
class QShortcut;

/// pqStandardViewFrameActionGroup is a pqViewFrameActionGroup subclass that
/// handles the buttons to be rendered on the left-side of the view-frame for
/// standard views in ParaView including the Render View and Chart views.
class PQAPPLICATIONCOMPONENTS_EXPORT pqStandardViewFrameActionGroup : public pqViewFrameActionGroup
{
  Q_OBJECT
  typedef pqViewFrameActionGroup Superclass;
public:
  pqStandardViewFrameActionGroup(QObject* parent=0);
  virtual ~pqStandardViewFrameActionGroup();

  /// Tries to add/remove this group's actions to/from the frame if the
  /// view type is supported. Returns whether or not they were.
  virtual bool connect(pqViewFrame *frame, pqView *view);
  virtual bool disconnect(pqViewFrame *frame, pqView *view);

protected slots:
  /// Called before the "Convert To" menu is shown. We populate the menu with
  /// actions for available view types.
  void aboutToShowConvertMenu();

  /// This slot is called either from an action in the "Convert To" menu, or from
  /// the buttons on an empty frame.
  void invoked();

  /// slots for various shortcuts.
  void selectSurfaceCellsTrigerred();
  void selectSurfacePointsTrigerred();
  void selectFrustumCellsTriggered();
  void selectFrustumPointsTriggered();
  void selectBlocksTriggered();

  /// If a QAction is added to an exclusive QActionGroup, then a checked action
  /// cannot be unchecked by clicking on it. We need that to work. Hence, we
  /// manually manage the exclusivity of the action group.
  void manageGroupExclusivity(QAction*);

protected:
  void setupEmptyFrame(QWidget* frame);
  void connectTitleBar(pqViewFrame *frame, pqView *view);


private:
  Q_DISABLE_COPY(pqStandardViewFrameActionGroup)

  QPointer<QShortcut> ShortCutSurfaceCells;
  QPointer<QShortcut> ShortCutSurfacePoints;
  QPointer<QShortcut> ShortCutFrustumCells;
  QPointer<QShortcut> ShortCutFrustumPoints;
  QPointer<QShortcut> ShortCutBlocks;
};

#endif
