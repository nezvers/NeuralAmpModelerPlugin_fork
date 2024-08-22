#pragma once

#include "IControls.h"
#include "IPlugPluginBase.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class Caption : public ICaptionControl
{
public:
  Caption(const IRECT& bounds, int paramIdx, const IText& text, const IColor& bgColor, bool showParamLabel = false)
  : ICaptionControl(bounds, paramIdx, text, bgColor, showParamLabel){};

  void Draw(IGraphics& g) override;
  void OnResize() override;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
