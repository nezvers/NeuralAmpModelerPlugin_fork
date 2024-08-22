#pragma once

#include "IControls.h"
#include "IPlugPluginBase.h"
#include "CustomButton.h"

using namespace iplug;
using namespace igraphics;

/** A "meta control" for a "preset manager" for "baked in" factory presets
 * It adds several child buttons
 * @ingroup IControls */
class PresetMenuDropdown : public IContainerBase
  , public IVectorBase
{
public:

  PresetMenuDropdown(const IRECT& bounds, const char* label = "",
    const IVStyle& style = DEFAULT_STYLE, std::function<void(int)> callback = nullptr)
    : IContainerBase(bounds)
    , IVectorBase(style)
  {
    AttachIControl(this, label);
    mIgnoreMouse = true;
    mRestorePresetCallback = callback;
  }

  void Draw(IGraphics& g) override {
    DrawLabel(g);
  }

  void RestorePreset(int presetIdx) {
    mRestorePresetCallback(presetIdx);
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override {
    if (pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();

      if (pItem)
      {
        RestorePreset(pSelectedMenu->GetChosenItemIdx());
      }
    }
  }

  void OnAttached() override {
    auto choosePresetFunc = [&](IControl* pCaller) {
      mMenu.Clear();

      iplug::IPluginBase* pluginBase = dynamic_cast<iplug::IPluginBase*>(pCaller->GetDelegate());

      int currentPresetIdx = pluginBase->GetCurrentPresetIdx();
      int nPresets = pluginBase->NPresets();

      for (int i = 0; i < nPresets; i++)
      {
        const char* str = pluginBase->GetPresetName(i);
        if (i == currentPresetIdx)
          mMenu.AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          mMenu.AddItem(str);
      }

      pCaller->GetUI()->CreatePopupMenu(*this, mMenu, pCaller->GetRECT());
      };
    mPresetNameButton = new CustomButton(IRECT(), SplashClickActionFunc, "Presets", mStyle);
    mPresetNameButton->SetAnimationEndActionFunction(choosePresetFunc);
    AddChildControl(mPresetNameButton);

    OnResize();
  }

  void OnResize() override
  {
    MakeRects(mRECT);

    ForAllChildrenFunc([&](int childIdx, IControl* pChild) {
      pChild->SetTargetAndDrawRECTs(mWidgetBounds);
      });
  }

private:
  IPopupMenu mMenu;
  CustomButton* mPresetNameButton = nullptr;
  std::function<void(int)> mRestorePresetCallback;
};

