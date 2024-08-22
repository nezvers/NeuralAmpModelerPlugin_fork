#pragma once

#include "IControls.h"
#include "IPlugPluginBase.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class Rotary : public IVKnobControl
{
public:
  Rotary(const IRECT& bounds, int paramIdx, const char* label = "", const IVStyle& style = DEFAULT_STYLE,
         bool valueIsEditable = false, bool valueInWidget = false, float a1 = -135.f, float a2 = 135.f,
         float aAnchor = -135.f, EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING,
         float trackSize = 2.f)
  : IVKnobControl(
    bounds, paramIdx, label, style, valueIsEditable, valueInWidget, a1, a2, aAnchor, direction, gearing, trackSize){};

  void DrawWidget(IGraphics& g) override;
  void DrawIndicatorTrack(IGraphics& g, float angle, float cx, float cy, float radius) override;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
