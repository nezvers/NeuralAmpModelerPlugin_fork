#include "Caption.h"
#include "color_and_style.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

void Caption::Draw(IGraphics& g)
{
  const IParam* pParam = GetParam();

  if (pParam)
  {
    pParam->GetDisplay(mStr);

    if (mShowParamLabel)
    {
      mStr.Append(" ");
      mStr.Append(pParam->GetLabel());
    }
  }
  g.FillRoundRect(COLOR_BLACK, mRECT, 2.5);
  g.DrawRoundRect(mMouseIsOver ? mTriangleMouseOverColor : COLOR_ACTIVE, mRECT, 2.5);

  IRECT textRect = mRECT.GetPadded(-5.f, -2.f, -mText.mSize, -2.f);
  if (mStr.GetLength() && g.GetControlInTextEntry() != this)
    g.DrawText(mText, mStr.Get(), textRect, &mBlend);

  if (mTriangleRect.W() > 0.f)
  {
    g.FillTriangle(mMouseIsOver ? mTriangleMouseOverColor : COLOR_ACTIVE, mTriangleRect.L, mTriangleRect.T,
                   mTriangleRect.R, mTriangleRect.T, mTriangleRect.MW(), mTriangleRect.B,
                   GetMouseIsOver() ? 0 : &BLEND_50);
  }
}

void Caption::OnResize()
{
  const float textHeight = mText.mSize;
  const float rectW = textHeight;
  const float triSizeX = rectW * 0.5f;
  const float triSizeY = rectW * 0.33f;

  mTriangleRect = mRECT.GetFromRight(rectW).GetCentredInside(IRECT(0.f, 0.f, triSizeX, triSizeY));
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
