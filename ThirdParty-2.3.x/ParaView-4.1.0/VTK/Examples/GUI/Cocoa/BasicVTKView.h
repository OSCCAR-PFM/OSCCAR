#import <Cocoa/Cocoa.h>

#import "vtkCocoaGLView.h"

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

@interface BasicVTKView : vtkCocoaGLView
{
@private
  vtkRenderer* renderer;
}

- (void)initializeVTKSupport;
- (void)cleanUpVTKSupport;

// Accessors
- (vtkRenderer*)getRenderer;
- (void)setRenderer:(vtkRenderer*)theRenderer;

@end
