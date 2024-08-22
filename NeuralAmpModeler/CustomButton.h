#pragma once


#include "IControls.h"
#include "IPlugPluginBase.h"
#include "color_and_style.h"

using namespace iplug;
using namespace igraphics;

/** A vector button/momentary switch control. */
class CustomButton : public IButtonControlBase
  , public IVectorBase
{
public:
  /** Constructs a vector button control, with an action function
   * @param bounds The control's bounds
   * @param aF An action function to execute when a button is clicked \see IActionFunction
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param labelInButton if the label inside or outside the button
   * @param valueInButton if the value inside or outside the button
   * @param shape The shape of the button */
  CustomButton(const IRECT& bounds, IActionFunction aF = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool labelInButton = true, bool valueInButton = true, EVShape shape = EVShape::Rectangle) : IButtonControlBase(bounds, aF)
    , IVectorBase(style, labelInButton, valueInButton)
  {
    //mTri
    mText = style.valueText;
    mShape = shape;
    AttachIControl(this, label);
  }


  void Draw(IGraphics& g) override {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    DrawValue(g, false);
  }

  virtual void DrawWidget(IGraphics& g) override {
    //bool pressed = (bool)GetValue();
    //DrawPressableShape(g, mShape, mWidgetBounds, pressed, mMouseIsOver, IsDisabled());

    g.DrawRoundRect(COLOR_ACTIVE, mRECT, 2.5);

    if (mTri.W() > 0.f)
    {
      g.FillTriangle(COLOR_ACTIVE, mTri.L, mTri.T, mTri.R, mTri.T, mTri.MW(), mTri.B, GetMouseIsOver() ? 0 : &BLEND_50);
    }
  }

  void DrawLabel(IGraphics& g) override {
    if (mLabelBounds.H() && mStyle.showLabel)
    {
      IRECT textRect = mLabelBounds.GetPadded(-5.f, -2.f, -mText.mSize, -2.f);
      IBlend blend = mControl->GetBlend();
      g.DrawText(mStyle.labelText, mLabelStr.Get(), textRect, &blend);
    }
  }

  bool IsHit(float x, float y) const override {
    return mWidgetBounds.Contains(x, y);
  }

  void OnResize() override {
    SetTargetRECT(MakeRects(mRECT, true));
    const float textHeight = mText.mSize;
    const float rectW = textHeight;
    const float triSizeX = rectW * 0.5f;
    const float triSizeY = rectW * 0.33f;

    mTri = mRECT.GetFromRight(rectW).GetCentredInside(IRECT(0.f, 0.f, triSizeX, triSizeY));
    SetDirty(false);
  }

private:
  IRECT mTri;
};

