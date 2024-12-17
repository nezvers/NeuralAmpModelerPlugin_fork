#include "Widgets.h"
#include <string>

#include "color_and_style.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE


void Button::DrawWidget(IGraphics& g)
{
  if (GetValue() > 0.5)
    g.FillRoundRect(COLOR_ACTIVE, mWidgetBounds, 2.5);
  else
    g.DrawRoundRect(COLOR_ACTIVE, mWidgetBounds, 2.5);
}

void Button::DrawValue(IGraphics& g, bool mouseOver)
{
  if (GetValue() > 0.5)
  {
    mStyle.valueText.mFGColor = COLOR_BG;
    g.DrawText(mStyle.valueText, mOnText.Get(), mValueBounds, &mBlend);
  }
  else
  {
    mStyle.valueText.mFGColor = COLOR_ACTIVE;
    g.DrawText(mStyle.valueText, mOffText.Get(), mValueBounds, &mBlend);
  }
}

void PatternSwitches::DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment,
                                 bool disabled)
{
  DrawPressableRectangle(g, r, pressed, mouseOver, disabled, segment == ETabSegment::Start, segment == ETabSegment::End,
                         segment == ETabSegment::Start, segment == ETabSegment::End);
}

void PlayButton::Draw(IGraphics& g)
{
  IRECT r = mRECT.GetPadded(-2);
  if (GetValue() > 0.5)
  {
    g.FillRect(COLOR_ACTIVE, r);
  }
  else
  {
    g.FillTriangle(COLOR_ACTIVE, r.L, r.T, r.R, (r.T + r.B) / 2, r.L, r.B);
  }
}

void PlayButton::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  SetDirty(true);
}

void ButtonDropdown::Draw(IGraphics& g)
{
  // DrawBackground(g, mRECT);
  DrawWidget(g);
  DrawLabel(g);
  DrawValue(g, false);

  g.DrawRoundRect(COLOR_ACTIVE, mRECT, 2.5);


  if (mTri.W() > 0.f)
  {
    g.FillTriangle(COLOR_ACTIVE, mTri.L, mTri.T, mTri.R, mTri.T, mTri.MW(), mTri.B, GetMouseIsOver() ? 0 : &BLEND_50);
  }
}


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
