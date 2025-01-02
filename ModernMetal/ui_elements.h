#pragma once

#include "IControls.h"
#include "IPlugPluginBase.h"


BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class BitmapKnob : public IVKnobControl, public IBitmapBase
{
public:
  BitmapKnob(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style, const IBitmap& bitmap,
             float radius, float lineThickness, float angle1, float angle2, const IBitmap& dotBitmap,
             const IRECT& dotBounds, float dotRadius)
  : IVKnobControl(bounds, paramIdx, label, style, true)
  , IBitmapBase(bitmap)
  {
    //mInnerPointerFrac = 1.5f;
    mTrackSize = lineThickness;
    mLabelBounds = bounds;
    mRadius = radius;
    mAnchorAngle = angle1;
    mAngle1 = angle1;
    mAngle2 = angle2;
    mDotRadius = dotRadius;
    mDotBitmap = dotBitmap;
    mDotBounds = dotBounds;
  }

  void OnRescale() override { 
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
    mDotBitmap = GetUI()->GetScaledBitmap(mDotBitmap);
  }

  void DrawWidget(IGraphics& g) override
  {
    
    IRECT knobRect = mWidgetBounds.GetCentredInside(mWidgetBounds.W(), mWidgetBounds.W());
    const float cx = knobRect.MW(), cy = knobRect.MH();
    const float angle = mAngle1 + (static_cast<float>(GetValue()) * (mAngle2 - mAngle1));
    
    DrawIndicatorTrack(g, angle, cx, cy, mRadius);

    const float angleRadians1 = DegToRad(mAngle1 - 90.f);
    const float angleRadians2 = DegToRad(angle - 90.f);
    const float x1 = cx + mRadius * std::cos(angleRadians1);
    const float y1 = cy + mRadius * std::sin(angleRadians1);
    const float x2 = cx + mRadius * std::cos(angleRadians2);
    const float y2 = cy + mRadius * std::sin(angleRadians2);


    g.PathClear();
    g.PathEllipse(x1, y1, mTrackSize * 0.25f, mTrackSize * 0.25f, 0.0f);
    g.PathStroke(GetColor(kX1), mTrackSize * 0.5f, IStrokeOptions(), &mBlend);

    g.PathClear();
    g.PathEllipse(x2, y2, mTrackSize * 0.25f, mTrackSize * 0.25f, 0.0f);
    g.PathStroke(GetColor(kX1), mTrackSize * 0.5f, IStrokeOptions(), &mBlend);
    
    g.DrawFittedBitmap(mBitmap, knobRect);

    // DOT
    const float x3 = cx + mDotRadius * std::cos(angleRadians2) - mDotBounds.W() * 0.5f;
    const float y3 = cy + mDotRadius * std::sin(angleRadians2) - mDotBounds.H() * 0.5f;
    IRECT dotRect = IRECT(x3, y3, x3 + mDotBounds.W(), y3 + mDotBounds.H());
    g.DrawFittedBitmap(mDotBitmap, dotRect);
  }


protected:
  float mRadius;
  float mDotRadius;
  IBitmap mDotBitmap;
  IRECT mDotBounds;
};

class BitmapSwitch : public ISwitchControlBase
{
public:
  BitmapSwitch(const IRECT& bounds, int paramIdx, const IBitmap& bitmap_on, const IBitmap& bitmap_off,
               const IBitmap& dotBitmap_on, const IBitmap& dotBitmap_off, const IRECT& dotBounds)
  : ISwitchControlBase(bounds, paramIdx)
  {
    mBitmap_on = bitmap_on;
    mBitmap_off = bitmap_off;
    mDot_on = dotBitmap_on;
    mDot_off = dotBitmap_off;
    mDotPos = dotBounds;
  }

  void OnRescale() override { 
    mBitmap_on = GetUI()->GetScaledBitmap(mBitmap_on);
    mBitmap_off = GetUI()->GetScaledBitmap(mBitmap_off);
    mDot_on = GetUI()->GetScaledBitmap(mDot_on);
    mDot_off = GetUI()->GetScaledBitmap(mDot_off);
  }

  void Draw(IGraphics& g) override
  {
    if (GetSelectedIdx() == 0)
    {
      g.DrawFittedBitmap(mBitmap_off, GetRECT(), &mBlend);
      g.DrawFittedBitmap(mDot_off, mDotPos, &mBlend);
    }
    else
    {
      g.DrawFittedBitmap(mBitmap_on, GetRECT(), &mBlend);
      g.DrawFittedBitmap(mDot_on, mDotPos, &mBlend);
    }
  }

protected:
  IBitmap mBitmap_on;
  IBitmap mBitmap_off;
  IBitmap mDot_on;
  IBitmap mDot_off;
  IRECT mDotPos;
  IBlend mBlend = IBlend(EBlend::Default, 1.0f);
};

class BitmapSwitchTimer : public ISwitchControlBase
{
public:
  BitmapSwitchTimer(const IRECT& bounds, int paramIdx, const IBitmap& bitmap_on, const IBitmap& bitmap_off,
                    const IBitmap& dotBitmap_on, const IBitmap& dotBitmap_off, const IRECT& dotBounds,
                    IActionFunction clickCallback = SplashClickActionFunc,
                    IActionFunction timeoutCallback = SplashClickActionFunc, uint32_t wait_time = 1000)
  : ISwitchControlBase(bounds, paramIdx)
  {
    mBitmap_on = bitmap_on;
    mBitmap_off = bitmap_off;
    mDot_on = dotBitmap_on;
    mDot_off = dotBitmap_off;
    mDotPos = dotBounds;
    
    mWaitTime = wait_time;
    mClickCallback = clickCallback;
    mTimeoutCallback = timeoutCallback;
    mClickEvent = [&](IControl* iControl) {
      if (GetSelectedIdx() == 1) {
        mClickCallback(this);
        mTimer = std::unique_ptr<Timer>(Timer::Create([&](Timer& t) { 
            mTimeoutCallback(this); }, mWaitTime));
      }
      else {
        mTimer = nullptr;
      }
    };
    SetActionFunction(mClickEvent);
  }


  void OnRescale() override
  {
    mBitmap_on = GetUI()->GetScaledBitmap(mBitmap_on);
    mBitmap_off = GetUI()->GetScaledBitmap(mBitmap_off);
    mDot_on = GetUI()->GetScaledBitmap(mDot_on);
    mDot_off = GetUI()->GetScaledBitmap(mDot_off);
  }

  void Draw(IGraphics& g) override
  {
    if (GetSelectedIdx() == 0)
    {
      g.DrawFittedBitmap(mBitmap_off, GetRECT(), &mBlend);
      g.DrawFittedBitmap(mDot_off, mDotPos, &mBlend);
    }
    else
    {
      g.DrawFittedBitmap(mBitmap_on, GetRECT(), &mBlend);
      g.DrawFittedBitmap(mDot_on, mDotPos, &mBlend);
    }
  }

protected:
  IBitmap mBitmap_on;
  IBitmap mBitmap_off;
  IBitmap mDot_on;
  IBitmap mDot_off;
  IRECT mDotPos;
  std::unique_ptr<Timer> mTimer;
  std::function<void(IControl*)> mClickEvent;
  std::function<void(IControl*)> mClickCallback;
  std::function<void(IControl*)> mTimeoutCallback;
  float mWaitTime;
  IBlend mBlend = IBlend(EBlend::Default, 1.0f);
};

class DropdownBitmap : public ICaptionControl
{
public:
  DropdownBitmap(const IRECT& bounds, int paramIdx, const IText& text, const IBitmap& bitmap,
                 bool showParamLabel = false, const IRECT& triangleRect = IRECT(),
                 const IBitmap& triangleBitmap = IBitmap())
  : ICaptionControl(bounds, paramIdx, text, {0}, showParamLabel)
  {
    mBitmap = bitmap;
    mTriangleBitmap = triangleBitmap;
    mTriangleRect = triangleRect;
  };

  void Draw(IGraphics& g) override
  {
    g.DrawFittedBitmap(mBitmap, GetRECT());
    g.DrawFittedBitmap(mTriangleBitmap, mTriangleRect);

    if (!mShowParamLabel) {
      return;
    }

    const IParam* pParam = GetParam();
    if (pParam)
    {
      pParam->GetDisplay(mStr);

      mStr.Append(" ");
      mStr.Append(pParam->GetLabel());
    }

    IRECT textRect = mRECT.GetPadded(-10.f, -2.f, -mText.mSize, -2.f);
    textRect.R = mTriangleRect.L;
    if (mStr.GetLength() && g.GetControlInTextEntry() != this) {
      g.DrawText(mText, mStr.Get(), textRect, &mBlend);
    }

  }

  void OnResize() override
  { 
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
    mTriangleBitmap = GetUI()->GetScaledBitmap(mTriangleBitmap);
  }

protected:
  IBitmap mBitmap;
  IBitmap mTriangleBitmap;
  IRECT mTriangleRect;
};

/** A bitmap button/momentary switch control. */
class BitmapButton : public IButtonControlBase
{
public:

  BitmapButton(const IRECT& bounds, const IBitmap& bitmap, const IBitmap& hoverBitmap,
               IActionFunction aF)
  : IButtonControlBase(bounds, aF)
  {
    mBitmap = bitmap;
    mHoverBitmap = hoverBitmap;
  }

  void Draw(IGraphics& g) override {
    if (mMouseIsOver) {
      g.DrawFittedBitmap(mHoverBitmap, GetRECT());

    }
    else {
      g.DrawFittedBitmap(mBitmap, GetRECT());
    }
  }
  void OnRescale() override { 
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
    mHoverBitmap = GetUI()->GetScaledBitmap(mHoverBitmap); 
  }

protected:
  IBitmap mBitmap;
  IBitmap mHoverBitmap;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE