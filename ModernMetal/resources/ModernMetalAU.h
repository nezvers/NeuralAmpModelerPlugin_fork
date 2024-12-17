
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1
  #import <UIKit/UIKit.h>
#else
  #import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vModernMetal
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vModernMetal
#import <ModernMetalAU/IPlugAUAudioUnit.h>
#import <ModernMetalAU/IPlugAUViewController.h>

//! Project version number for ModernMetalAU.
FOUNDATION_EXPORT double ModernMetalAUVersionNumber;

//! Project version string for ModernMetalAU.
FOUNDATION_EXPORT const unsigned char ModernMetalAUVersionString[];

@class IPlugAUViewController_vModernMetal;
