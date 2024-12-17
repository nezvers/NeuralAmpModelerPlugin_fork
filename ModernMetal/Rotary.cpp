#include "Rotary.h"
#include "color_and_style.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

void Rotary::DrawWidget(IGraphics& g)
{
  float widgetRadius = GetRadius() - 2;
  const float cx = mWidgetBounds.MW(), cy = mWidgetBounds.MH() + 2;
  IRECT knobHandleBounds =
    mWidgetBounds.GetCentredInside((widgetRadius - mTrackToHandleDistance - 2) * 2.f).GetVShifted(2);
  const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));
  DrawIndicatorTrack(g, angle, cx, cy, widgetRadius);
  g.FillEllipse(GetColor(kX3), knobHandleBounds);
  DrawPointer(g, angle, cx, cy, knobHandleBounds.W() / 2.f);
}

void Rotary::DrawIndicatorTrack(IGraphics& g, float angle, float cx, float cy, float radius)
{
  g.DrawArc(GetColor(kX2), cx, cy, radius, -135, 135, &mBlend, mTrackSize);
  if (mTrackSize > 0.f)
  {
    g.DrawArc(GetColor(kX1), cx, cy, radius,
              angle >= mAnchorAngle ? mAnchorAngle : mAnchorAngle - (mAnchorAngle - angle),
              angle >= mAnchorAngle ? angle : mAnchorAngle, &mBlend, mTrackSize);
  }
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
