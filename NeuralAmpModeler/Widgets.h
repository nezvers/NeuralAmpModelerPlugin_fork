#pragma once

#include "IControls.h"
#include "IPlugPluginBase.h"


BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class NumberControl : public IVNumberBoxControl
{
public:
  NumberControl(const IRECT& bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr,
                const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool buttons = false,
                double defaultValue = 50.f, double minValue = 1.f, double maxValue = 100.f,
                const char* fmtStr = "%0.0f", bool drawTriangle = true)
  : IVNumberBoxControl(
    bounds, paramIdx, actionFunc, label, style, buttons, defaultValue, minValue, maxValue, fmtStr, drawTriangle)
  {
    mLargeIncrement = 0.1f;
  };
};

class Button : public IVToggleControl
{
public:
  Button(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, const char* offText,
         const char* onText)
  : IVToggleControl(bounds, paramIdx, label, style, offText, onText){};

  Button(const IRECT& bounds, IActionFunction aF, const char* label, const IVStyle& style, const char* offText,
         const char* onText)
  : IVToggleControl(bounds, aF, label, style, offText, onText){};

  void DrawWidget(IGraphics& g) override;
  void DrawValue(IGraphics& g, bool mouseOver) override;
};

class ButtonDropdown : public IVButtonControl
{
public:
  ButtonDropdown(const IRECT& bounds = IRECT(), IActionFunction aF = nullptr, const char* label = "",
                 const IVStyle& style = DEFAULT_STYLE, bool labelInButton = true, bool valueInButton = false,
                 EVShape shape = EVShape::Rectangle)
  : IVButtonControl(bounds, nullptr, label, style, labelInButton, valueInButton, shape){};

  void Draw(IGraphics& g) override;


private:
  IRECT mTri;
};

class PatternSwitches : public IVTabSwitchControl
{
public:
  PatternSwitches(const IRECT& bounds, int paramIdx, const std::vector<const char*>& options, const char* label,
                  const IVStyle& style, EVShape shape, EDirection direction)
  : IVTabSwitchControl(bounds, paramIdx, options, label, style, shape, direction){};

  void DrawButton(IGraphics& g, const IRECT& r, bool pressed, bool mouseOver, ETabSegment segment,
                  bool disabled) override;
};

class PlayButton : public IControl
{
public:
  PlayButton(const IRECT& bounds, IActionFunction af)
  : IControl(bounds, af){};
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void Draw(IGraphics& g) override;
};

class BitmapSwitch : public ISwitchControlBase
{
public:
  BitmapSwitch(const IRECT& bounds, int paramIdx, const IBitmap& bitmap_on, const IBitmap& bitmap_off)
  : ISwitchControlBase(bounds, paramIdx)
  {
    mBitmap_on = bitmap_on;
    mBitmap_off = bitmap_off;
  }

  void Draw(IGraphics& g) override
  {
    if (GetSelectedIdx() == 0) {
      g.DrawFittedBitmap(mBitmap_off, GetRECT());
    }
    else {
      g.DrawFittedBitmap(mBitmap_on, GetRECT());
    }
  }

protected:
  IBitmap mBitmap_on;
  IBitmap mBitmap_off;
};

class BitmapKnob : public IVKnobControl, public IBitmapBase
{
public:
  BitmapKnob(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, IBitmap bitmap)
  : IVKnobControl(bounds, paramIdx, label, style, true)
  , IBitmapBase(bitmap)
  {
    mInnerPointerFrac = 0.5f;
    mLabelBounds = bounds;
  }

  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }

  void DrawWidget(IGraphics& g) override
  {
    float widgetRadius = GetRadius(); // * 0.73;
    auto knobRect = mWidgetBounds.GetCentredInside(mWidgetBounds.W(), mWidgetBounds.W());
    const float cx = knobRect.MW(), cy = knobRect.MH();
    const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));
    g.DrawFittedBitmap(mBitmap, knobRect);
    
    mInnerPointerFrac = 0.43f;
    mTrackSize = 2.0;
    float valueRadius = 0.615;
    g.DrawArc(COLOR_BLACK, cx, cy, widgetRadius * valueRadius, 0.0, 360.0, &mBlend, mTrackSize * 3.5);
    DrawIndicatorTrack(g, angle, cx + 0.5, cy, widgetRadius * valueRadius);

    float data[2][2];
    RadialPoints(angle, cx, cy, mInnerPointerFrac * widgetRadius, mInnerPointerFrac * widgetRadius, 2, data);
    g.PathCircle(data[1][0], data[1][1], 3);
    g.PathFill(IPattern::CreateRadialGradient(data[1][0], data[1][1], 4.0f,
                                              {{GetColor(mMouseIsOver ? kX3 : kX1), 0.f},
                                               {GetColor(mMouseIsOver ? kX3 : kX1), 0.8f},
                                               {COLOR_TRANSPARENT, 1.0f}}),
               {}, &mBlend);
    g.DrawCircle(COLOR_BLACK.WithOpacity(0.5f), data[1][0], data[1][1], 3, &mBlend);
  }
};

class LinePeakControl : public IVPeakAvgMeterControl<>
{
  static constexpr float KMeterMin = -70.0f;
  static constexpr float KMeterMax = -0.01f;
  static constexpr float KLineThickness = 1.0f;

public:
  LinePeakControl(const IRECT& bounds, const IVStyle& style, const EDirection direction = EDirection::Vertical)
  : IVPeakAvgMeterControl<>(bounds, "", style.WithShowValue(false).WithDrawFrame(false).WithWidgetFrac(1.0), direction,
                            {}, 0, KMeterMin, KMeterMax, {})
  {
    SetPeakSize(5.0f);
  }

  virtual void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    if (mDirection == EDirection::Vertical) {
      mWidgetBounds = mWidgetBounds.GetMidHPadded(KLineThickness); // line thickness
    }
    else {
      mWidgetBounds = mWidgetBounds.GetMidVPadded(KLineThickness);
    }
    MakeTrackRects(mWidgetBounds);
    //MakeStepRects(mWidgetBounds, mNSteps);
    SetDirty(false);
  }

  void DrawBackground(IGraphics& g, const IRECT& r) override {
    IRECT rect;
    const IColor colorBackground = COLOR_BLACK.WithOpacity(0.5f);
    if (mDirection == EDirection::Vertical)
    {
      rect = r.GetMidHPadded(KLineThickness); // line thickness
    }
    else
    {
      rect = r.GetMidVPadded(KLineThickness);
    }
    g.FillRect(colorBackground, rect, &mBlend);
  }

  void DrawTrackHandle(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    g.FillRect(GetColor(kX1), r, &mBlend);
  }

  void DrawPeak(IGraphics& g, const IRECT& r, int chIdx, bool aboveBaseValue) override
  {
    //g.DrawGrid(COLOR_BLACK, mTrackBounds.Get()[chIdx], 10, 2);
    //g.FillRect(GetColor(kX3), r, &mBlend);
  }
};

class SliderHandle : public IVSliderControl
{
public:
  SliderHandle(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "",
               const IVStyle& style = DEFAULT_STYLE, bool valueIsEditable = false,
               EDirection dir = EDirection::Vertical, double gearing = DEFAULT_GEARING, float handleSize = 8.f,
               float trackSize = 2.f, bool handleInsideTrack = false, float handleXOffset = 0.f,
               float handleYOffset = 0.f)
  : IVSliderControl(bounds, paramIdx, label, style, valueIsEditable, dir, gearing, handleSize, trackSize, handleInsideTrack, handleXOffset, handleYOffset)
  {  
  }
  void Draw(IGraphics& g) override {
    IRECT filledTrack = mTrackBounds.FracRect(mDirection, (float)GetValue());
    float cx, cy;

    const float offset =
      (mStyle.drawShadows && mShape != EVShape::Ellipse /* TODO? */) ? mStyle.shadowOffset * 0.5f : 0.f;

    if (mDirection == EDirection::Vertical)
    {
      cx = filledTrack.MW() + offset;
      cy = filledTrack.T;
    }
    else
    {
      cx = filledTrack.R;
      cy = filledTrack.MH() + offset;
    }

    if (mHandleSize > 0.f)
    {
      IRECT r = {cx + mHandleXOffset - mHandleSize, cy + mHandleYOffset - mHandleSize,
                     cx + mHandleXOffset + mHandleSize, cy + mHandleYOffset + mHandleSize};
      const float contrast = IsDisabled() ? -GRAYED_ALPHA : 0.f;
      const IBlend blend = mControl->GetBlend();

      g.FillEllipse(GetColor(kFG).WithContrast(contrast), r /*, &blend*/);

      // Shade when hovered
      if (mMouseIsOver)
        g.FillEllipse(GetColor(kHL), r, &blend);
    }
  }

};


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
