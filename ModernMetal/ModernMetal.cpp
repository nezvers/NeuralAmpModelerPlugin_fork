#include <algorithm> // std::clamp, std::min
#include <cmath> // pow
#include <filesystem>
#include <iostream>
#include <utility>

#include "Colors.h"
#include "ModernMetalCore/NAM/activations.h"
#include "ModernMetalCore/NAM/get_dsp.h"
// clang-format off
// These includes need to happen in this order or else the latter won't know
// a bunch of stuff.
#include "ModernMetal.h"
#include "IPlug_include_in_plug_src.h"
// clang-format on
#include "architecture.hpp"

#include "ModernMetalControls.h"

// <-- NeZvers
#include "ui_elements.h"
#include <ModernMetalCore/NAM/lstm.h>
#include <ModernMetalCore/NAM/wavenet.h>
#include "mmfat_nam.h"
#include "mmtight_nam.h"
#include "rectal57.h"
const char* nam_list[] = {mmfat_nam, mmtight_nam};
// <--

using namespace iplug;
using namespace igraphics;

const double kDCBlockerFrequency = 5.0;

// Styles
const IVColorSpec colorSpec{
  DEFAULT_BGCOLOR, // Background
  PluginColors::NAM_THEMECOLOR, // Foreground
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.3f), // Pressed
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.4f), // Frame
  PluginColors::MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  IColor(255, 240, 40, 20), // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  PluginColors::NAM_THEMECOLOR.WithContrast(0.1f), // Extra 3
};

const IVStyle style =
  IVStyle{true, // Show label
          true, // Show value
          colorSpec,
          {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Middle, PluginColors::NAM_THEMEFONTCOLOR}, // Knob label text5
          {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Bottom, PluginColors::NAM_THEMEFONTCOLOR}, // Knob value text
          DEFAULT_HIDE_CURSOR,
          DEFAULT_DRAW_FRAME,
          false,
          DEFAULT_EMBOSS,
          0.2f,
          2.f,
          DEFAULT_SHADOW_OFFSET,
          DEFAULT_WIDGET_FRAC,
          DEFAULT_WIDGET_ANGLE};
const IVStyle titleStyle =
  DEFAULT_STYLE.WithValueText(IText(30, COLOR_WHITE, "Michroma-Regular")).WithDrawFrame(false).WithShadowOffset(2.f);
const IVStyle radioButtonStyle =
  style
    .WithColor(EVColor::kON, PluginColors::NAM_THEMECOLOR) // Pressed buttons and their labels
    .WithColor(EVColor::kOFF, PluginColors::NAM_THEMECOLOR.WithOpacity(0.1f)) // Unpressed buttons
    .WithColor(EVColor::kX1, PluginColors::NAM_THEMECOLOR.WithOpacity(0.6f)); // Unpressed buttons' labels

EMsgBoxResult _ShowMessageBox(iplug::igraphics::IGraphics* pGraphics, const char* str, const char* caption,
                              EMsgBoxType type)
{
#ifdef OS_MAC
  // macOS is backwards?
  return pGraphics->ShowMessageBox(caption, str, type);
#else
  return pGraphics->ShowMessageBox(str, caption, type);
#endif
}

const std::string kCalibrateInputParamName = "CalibrateInput";
const std::string kInputCalibrationLevelParamName = "InputCalibrationLevel";
const double kDefaultInputCalibrationLevel = 12.0;
const bool kDefaultCalibrateInput = false;

// <-- NeZvers
std::vector<float> GetWeights(nlohmann::json const& j)
{
  if (j.find("weights") != j.end())
  {
    auto weight_list = j["weights"];
    std::vector<float> weights;
    for (auto it = weight_list.begin(); it != weight_list.end(); ++it)
      weights.push_back(*it);
    return weights;
  }
  else
    throw std::runtime_error("Corrupted model file is missing weights.");
}

std::unique_ptr<nam::DSP> get_dsp_json(const char* jsonStr)
{
  nlohmann::json j = nlohmann::json::parse(jsonStr);
  nam::verify_config_version(j["version"]);

  auto architecture = j["architecture"];
  std::vector<float> weights = GetWeights(j);

  // Assign values to config
  nam::dspData config;
  config.version = j["version"];
  config.architecture = j["architecture"];
  config.config = j["config"];
  config.metadata = j["metadata"];
  config.weights = weights;
  if (j.find("sample_rate") != j.end())
    config.expected_sample_rate = j["sample_rate"];
  else
  {
    config.expected_sample_rate = -1.0;
  }

  /*Copy to a new dsp_config object for GetDSP below,
   since not sure if weights actually get modified as being non-const references on some
   model constructors inside GetDSP(dsp_config& conf).
   We need to return unmodified version of dsp_config via returnedConfig.*/
  nam::dspData conf = config;

  return get_dsp(conf);
}

int ModernMetal::_StageModelCustom(int modelIndex)
{
  try
  {

    std::unique_ptr<nam::DSP> model = get_dsp_json(nam_list[modelIndex]);
    std::unique_ptr<ResamplingNAM> temp = std::make_unique<ResamplingNAM>(std::move(model), GetSampleRate());
    temp->Reset(GetSampleRate(), GetBlockSize());
    mStagedModel = std::move(temp);
    // SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  }
  catch (std::runtime_error& e)
  {
    // SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);

    if (mStagedModel != nullptr)
    {
      mStagedModel = nullptr;
    }
    std::cerr << "Failed to read DSP module" << std::endl;
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}

dsp::wav::LoadReturnCode ModernMetal::_StageIRCustom(bool enabled)
{
  if (!enabled)
  {
    return dsp::wav::LoadReturnCode::ERROR_OTHER;
  }
  // FIXME it'd be better for the path to be "staged" as well. Just in case the
  // path and the model got caught on opposite sides of the fence...
  dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
  try
  {
    if (mStagedIR != nullptr)
    {
      wavState = dsp::wav::LoadReturnCode::SUCCESS;
    }
    else
    {
      dsp::ImpulseResponse::IRData irData;
      irData.mRawAudio = IrRawAudio;
      irData.mRawAudioSampleRate = IrWavFileData.header.sample_rate;

      const double sampleRate = GetSampleRate();
      mStagedIR = std::make_unique<dsp::ImpulseResponse>(irData, sampleRate); // <- Localize
      wavState = mStagedIR->GetWavState();
      std::cerr << "Successful load IR:" << std::endl;
    }
  }
  catch (std::runtime_error& e)
  {
    wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
    std::cerr << "Caught unhandled exception while attempting to load IR:" << std::endl;
    std::cerr << e.what() << std::endl;
  }

  if (wavState == dsp::wav::LoadReturnCode::SUCCESS)
  {
    // SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  }
  else
  {
    if (mStagedIR != nullptr)
    {
      mStagedIR = nullptr;
    }
    // SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  return wavState;
}

void ModernMetal::_UpdateCompensation()
{
  const iplug::sample peakTargetDb = -5.0;
  const iplug::sample peakMinDb = -40.0;
  iplug::sample peakMaxDb = 10.0 * log10((double)peakMax);
  if (peakMaxDb < peakMinDb)
  {
    // RESET
    peakMax = -120.0;
    GetParam(kInputCalibrationLevel)->Set(1.0);
    SendParameterValueFromDelegate(kInputCalibrationLevel, 1.0, false);
    DirtyParametersFromUI();
    return;
  }

  iplug::sample peakTarget = std::pow(10.0, 0.05 * peakTargetDb);
  iplug::sample value = peakTarget / peakMax;
  GetParam(kInputCalibrationLevel)->Set(value);
  SendParameterValueFromDelegate(kInputCalibrationLevel, value, false);
  DirtyParametersFromUI(); // updates the host with the new parameter values
}

void ModernMetal::_SkinSwitch() {
  LayoutUI(GetUI());
  //GetUI()->SetAllControlsDirty();
  SendControlValueFromDelegate(kCtrlTagInput, GetParam(kInputLevel)->GetNormalized());
  SendControlValueFromDelegate(kCtrlTagOutput, GetParam(kOutputLevel)->GetNormalized());
  SendControlValueFromDelegate(kCtrlTagGate, GetParam(kNoiseGateThreshold)->GetNormalized());
  SendControlValueFromDelegate(kCtrlTagProfile, GetParam(kModelIndex)->GetNormalized());
  SendControlValueFromDelegate(kCtrlTagIr, GetParam(kIRToggle)->GetNormalized());
  SendControlValueFromDelegate(kCtrlTagCalibrateInput, GetParam(kCalibrateInput)->GetNormalized());
}
  // <--


ModernMetal::ModernMetal(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // <-- NeZvers
  // Need to be initialized before changing parameters
  IrWavFileData = WAV_ParseFileData(RECTAL57);
  IrRawAudio = WAV_ExtractSamples(IrWavFileData);
  // <--
  _InitToneStack();
  nam::activations::Activation::enable_fast_tanh();
  GetParam(kInputLevel)->InitGain("Input", 0.0, -20.0, 20.0, 0.1);
  GetParam(kToneBass)->InitDouble("Bass", 5.0, 0.0, 10.0, 0.1);
  GetParam(kToneMid)->InitDouble("Middle", 5.0, 0.0, 10.0, 0.1);
  GetParam(kToneTreble)->InitDouble("Treble", 5.0, 0.0, 10.0, 0.1);
  GetParam(kOutputLevel)->InitGain("Output", 0.0, -40.0, 40.0, 0.1);
  GetParam(kNoiseGateThreshold)->InitGain("Threshold", -80.0, -100.0, 0.0, 0.1);
  GetParam(kNoiseGateActive)->InitBool("NoiseGateActive", true);
  GetParam(kEQActive)->InitBool("ToneStack", true);
  GetParam(kOutputMode)->InitEnum("OutputMode", 1, {"Raw", "Normalized", "Calibrated"}); // TODO DRY w/ control
  GetParam(kIRToggle)->InitBool("IRToggle", true);
  GetParam(kCalibrateInput)->InitBool("CalibrateInput", false);
  GetParam(kDarkMode)->InitBool("DarkMode", false);
  GetParam(kInputCalibrationLevel)
    ->InitDouble("InputCalibrationLevel", 1.0, 0.0, 150.0, 0.1, "");
  // <-- NeZvers
  GetParam(kModelIndex)->InitEnum("Model Index", 0, kModelCount, "", 0, "", MODEL_NAMES);
  // <--

  mNoiseGateTrigger.AddListener(&mNoiseGateGain);

  mMakeGraphicsFunc = [&]() {

#ifdef OS_IOS
    auto scaleFactor = GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT) * 0.85f;
#else
    auto scaleFactor = 1.0f;
#endif

    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, scaleFactor);
  };

  

  mLayoutFunc = [&](IGraphics* pGraphics) {
    // Learn Button click
    std::function<void(IControl*)> ClickCallback = [&](IControl* iControl) {
      mLearnInput = true;
      peakMax = -120.0;
      GetParam(kInputCalibrationLevel)->Set(1.0);
      SendParameterValueFromDelegate(kInputCalibrationLevel, 1.0, false);
      peakMax = 0.0;
    };
    // Learn button timeout
    std::function<void(IControl*)> TimeoutCallback = [&](IControl* iControl) {
      GetParam(kCalibrateInput)->Set(false);
      SendParameterValueFromDelegate(kCalibrateInput, 0.0, true);
      mLearnInput = false;
      _UpdateCompensation();      
    };
    std::function<void(IControl*)> darkModeCallback = [&](IControl* iControl) {
      bool isDarkmode = GetParam(kDarkMode)->Bool();
      GetParam(kDarkMode)->Set(!isDarkmode);
      SendParameterValueFromDelegate(kDarkMode, !isDarkmode, false);
      this->_SkinSwitch();
    };

    if (pGraphics->NControls())
    {
      pGraphics->RemoveAllControls();
    }

    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachTextEntryControl();
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableTooltips(true);
    pGraphics->EnableMultiTouch(true);

    const char* profileFont = "PlusJakartaSans-Bold";
    pGraphics->LoadFont(profileFont, JAKARTA_FN);

    IBitmap bigKnobBitmap;
    IBitmap knobBitmap;
    IBitmap dotBitmap_on;
    IBitmap dotBitmap_off;
    IBitmap profileBitmap;
    IBitmap triangleBitmap;
    IBitmap irButtonOnBitmap;
    IBitmap irButtonOffBitmap;
    IBitmap listeningBitmap;
    IBitmap logoBitmap;
    IBitmap logoHoverBitmap;
    IBitmap calibrateOnBitmap;
    IBitmap calibrateOffBitmap;

    bool isDarkmode = GetParam(kDarkMode)->Bool();
    if (isDarkmode)
    {
      pGraphics->AttachBackground(BACKGROUND_DARK_FN); // Actual background
      bigKnobBitmap = pGraphics->LoadBitmap(BIGKNOB_DARK_FN, 1, false, 1);
      knobBitmap = pGraphics->LoadBitmap(SMALLKNOB_DARK_FN, 1, false, 1);
      dotBitmap_on = pGraphics->LoadBitmap(PUMPA_ON_DARK_FN, 1, false, 1);
      dotBitmap_off = pGraphics->LoadBitmap(PUMPA_OFF_DARK_FN, 1, false, 1);
      profileBitmap = pGraphics->LoadBitmap(EKRANS_FN, 1, false, 1);
      triangleBitmap = pGraphics->LoadBitmap(BULTA_FN, 1, false, 1);
      irButtonOnBitmap = pGraphics->LoadBitmap(BUTTON_PRESS_DARK_FN, 1, false, 1);
      irButtonOffBitmap = pGraphics->LoadBitmap(BUTTON_DARK_FN, 1, false, 1);
      listeningBitmap = pGraphics->LoadBitmap(LISTENING_DARK_FN, 2, true, 1);
      logoBitmap = pGraphics->LoadBitmap(LOGO_DARK_FN, 1, false, 1);
      logoHoverBitmap = pGraphics->LoadBitmap(LOGO_HOVER_DARK_FN, 1, false, 1);
      calibrateOnBitmap = irButtonOnBitmap;
      calibrateOffBitmap = irButtonOffBitmap;
    }
    else
    {
      pGraphics->AttachBackground(BACKGROUND_LIGHT_FN); // Actual background
      bigKnobBitmap = pGraphics->LoadBitmap(BIGKNOB_LIGHT_FN, 1, false, 1);
      knobBitmap = pGraphics->LoadBitmap(SMALLKNOB_LIGHT_FN, 1, false, 1);
      dotBitmap_on = pGraphics->LoadBitmap(PUMPA_ON_LIGHT_FN, 1, false, 1);
      dotBitmap_off = pGraphics->LoadBitmap(PUMPA_OFF_LIGHT_FN, 1, false, 1);
      profileBitmap = pGraphics->LoadBitmap(EKRANS_FN, 1, false, 1);
      triangleBitmap = pGraphics->LoadBitmap(BULTA_FN, 1, false, 1);
      irButtonOnBitmap = pGraphics->LoadBitmap(BUTTON_PRESS_LIGHT_FN, 1, false, 1);
      irButtonOffBitmap = pGraphics->LoadBitmap(BUTTON_LIGHT_FN, 1, false, 1);
      listeningBitmap = pGraphics->LoadBitmap(LISTENING_LIGHT_FN, 2, true, 1);
      logoBitmap = pGraphics->LoadBitmap(LOGO_LIGHT_FN, 1, false, 1);
      logoHoverBitmap = pGraphics->LoadBitmap(LOGO_HOVER_LIGHT_FN, 1, false, 1);
      calibrateOnBitmap = irButtonOnBitmap;
      calibrateOffBitmap = irButtonOffBitmap;
    }


    const auto b = pGraphics->GetBounds();
    const IRECT dotRect = IRECT(0, 0, 9, 9);


    float x = 20;
    float y = 55;
    float w = 260;
    float h = 260;
    float angleMin = -149.0f;
    float angleMax = 149.0f;

    IVStyle knobStyle = style;
    knobStyle.showValue = false;
    knobStyle.showLabel = false;
    pGraphics->AttachControl(new BitmapKnob(IRECT(x, y, x + w, y + h), kInputLevel, "", knobStyle, bigKnobBitmap, 88.f,
                                            3.9f, angleMin, angleMax, dotBitmap_on, dotRect, 60.f),
                             kCtrlTagInput);

    x = 238;
    y = 23;
    w = 38;
    h = 48;
    pGraphics->AttachControl(
      new BitmapButton(IRECT(x, y, x + w, y + h), logoBitmap, logoHoverBitmap, darkModeCallback));

    x = 74;
    y = 311;
    w = 85;
    h = 85;
    float radius = 28.0f;
    float lineThickness = 2.9f;

    pGraphics->AttachControl(new BitmapKnob(IRECT(x, y, x + w, y + h), kNoiseGateThreshold, "", knobStyle, knobBitmap,
                                            radius, lineThickness, angleMin, angleMax, dotBitmap_on, dotRect, 12.f),
                             kCtrlTagGate);

    x = 208;
    pGraphics->AttachControl(new BitmapKnob(IRECT(x, y, x + w, y + h), kOutputLevel, "", knobStyle, knobBitmap, radius,
                                            lineThickness, angleMin, angleMax, dotBitmap_on, dotRect, 12.f),
                             kCtrlTagOutput);
    


    x = 25;
    y = 380;
    w = 49;
    h = 26;
    pGraphics->AttachControl(
      new IBitmapControl(IRECT(x, y, x + w, y + h), listeningBitmap, kCalibrateInput, EBlend::Default));

    x = 7;
    y = 311;
    w = 85;
    h = 85;
    IRECT dotPos1 = IRECT(45, 320, 54, 329);
    pGraphics->AttachControl(new BitmapSwitchTimer(IRECT(x, y, x + w, y + h), kCalibrateInput, calibrateOnBitmap,
                                                   calibrateOffBitmap, dotBitmap_on, dotBitmap_off,
                                                   dotPos1, ClickCallback,
                                                   TimeoutCallback, 10000),
      kCtrlTagCalibrateInput);
    

    x = 141;
    IRECT dotPos2 = IRECT(179, 320, 188, 329);
    pGraphics->AttachControl(new BitmapSwitch(IRECT(x, y, x + w, y + h), kIRToggle, irButtonOnBitmap, irButtonOffBitmap,
                                              dotBitmap_on, dotBitmap_off, dotPos2),
                             kCtrlTagIr);

    x = 115;
    y = 286;
    w = 70;
    h = 25;

    const iplug::igraphics::IColor PROFILE_COLOR = IColor::FromColorCode(0x501414);
    iplug::igraphics::IText PROFILE_TEXT = IText(14.f, PROFILE_COLOR, profileFont, EAlign::Center, EVAlign::Middle);
    
    float tX = x + 53.0f;
    float tY = y + 11.0f;
    float tW = 8.0f;
    float tH = 5.0f;
    pGraphics->AttachControl(new DropdownBitmap(IRECT(x, y, x + w, y + h), kModelIndex, PROFILE_TEXT, profileBitmap,
                                                true, IRECT(tX, tY, tX + tW, tY + tH), triangleBitmap), kCtrlTagProfile);

    /*
    pGraphics->AttachControl(
      new NAMKnobControl(bassKnobArea, kToneBass, "", style, knobBackgroundBitmap), -1, "EQ_KNOBS");
    pGraphics->AttachControl(
      new NAMKnobControl(midKnobArea, kToneMid, "", style, knobBackgroundBitmap), -1, "EQ_KNOBS");
    pGraphics->AttachControl(
      new NAMKnobControl(trebleKnobArea, kToneTreble, "", style, knobBackgroundBitmap), -1, "EQ_KNOBS");
    */
  };
}

ModernMetal::~ModernMetal()
{
  _DeallocateIOPointers();
}

void ModernMetal::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{
  const size_t numChannelsExternalIn = (size_t)NInChansConnected();
  const size_t numChannelsExternalOut = (size_t)NOutChansConnected();
  const size_t numChannelsInternal = kNumChannelsInternal;
  const size_t numFrames = (size_t)nFrames;
  const double sampleRate = GetSampleRate();

  // Disable floating point denormals
  std::fenv_t fe_state;
  std::feholdexcept(&fe_state);
  disable_denormals();

  _PrepareBuffers(numChannelsInternal, numFrames);
  // Input is collapsed to mono in preparation for the NAM.
  _ProcessInput(inputs, numFrames, numChannelsExternalIn, numChannelsInternal);
  _ApplyDSPStaging();
  const bool noiseGateActive = GetParam(kNoiseGateActive)->Value();
  const bool toneStackActive = GetParam(kEQActive)->Value();

  // Noise gate trigger
  sample** triggerOutput = mInputPointers;
  if (noiseGateActive)
  {
    const double time = 0.01;
    const double threshold = GetParam(kNoiseGateThreshold)->Value(); // GetParam...
    const double ratio = 0.1; // Quadratic...
    const double openTime = 0.005;
    const double holdTime = 0.01;
    const double closeTime = 0.05;
    const dsp::noise_gate::TriggerParams triggerParams(time, threshold, ratio, openTime, holdTime, closeTime);
    mNoiseGateTrigger.SetParams(triggerParams);
    mNoiseGateTrigger.SetSampleRate(sampleRate);
    triggerOutput = mNoiseGateTrigger.Process(mInputPointers, numChannelsInternal, numFrames);
  }

  if (mModel != nullptr)
  {
    mModel->process(triggerOutput[0], mOutputPointers[0], nFrames);
  }
  else
  {
    _FallbackDSP(triggerOutput, mOutputPointers, numChannelsInternal, numFrames);
  }
  // Apply the noise gate after the NAM
  sample** gateGainOutput =
    noiseGateActive ? mNoiseGateGain.Process(mOutputPointers, numChannelsInternal, numFrames) : mOutputPointers;

  sample** toneStackOutPointers = (toneStackActive && mToneStack != nullptr)
                                    ? mToneStack->Process(gateGainOutput, numChannelsInternal, numFrames)
                                    : gateGainOutput;

  sample** irPointers = toneStackOutPointers;
  if (mIR != nullptr && GetParam(kIRToggle)->Value())
    irPointers = mIR->Process(toneStackOutPointers, numChannelsInternal, numFrames);

  // And the HPF for DC offset (Issue 271)
  const double highPassCutoffFreq = kDCBlockerFrequency;
  // const double lowPassCutoffFreq = 20000.0;
  const recursive_linear_filter::HighPassParams highPassParams(sampleRate, highPassCutoffFreq);
  // const recursive_linear_filter::LowPassParams lowPassParams(sampleRate, lowPassCutoffFreq);
  mHighPass.SetParams(highPassParams);
  // mLowPass.SetParams(lowPassParams);
  sample** hpfPointers = mHighPass.Process(irPointers, numChannelsInternal, numFrames);
  // sample** lpfPointers = mLowPass.Process(hpfPointers, numChannelsInternal, numFrames);

  // restore previous floating point state
  std::feupdateenv(&fe_state);

  // Let's get outta here
  // This is where we exit mono for whatever the output requires.
  _ProcessOutput(hpfPointers, outputs, numFrames, numChannelsInternal, numChannelsExternalOut);
  // _ProcessOutput(lpfPointers, outputs, numFrames, numChannelsInternal, numChannelsExternalOut);
  // * Output of input leveling (inputs -> mInputPointers),
  // * Output of output leveling (mOutputPointers -> outputs)
  _UpdateMeters(mInputPointers, outputs, numFrames, numChannelsInternal, numChannelsExternalOut);
}

void ModernMetal::OnReset()
{
  const auto sampleRate = GetSampleRate();
  const int maxBlockSize = GetBlockSize();

  // Tail is because the HPF DC blocker has a decay.
  // 10 cycles should be enough to pass the VST3 tests checking tail behavior.
  // I'm ignoring the model & IR, but it's not the end of the world.
  const int tailCycles = 10;
  SetTailSize(tailCycles * (int)(sampleRate / kDCBlockerFrequency));
  /*mInputSender.Reset(sampleRate);
  mOutputSender.Reset(sampleRate);*/
  // If there is a model or IR loaded, they need to be checked for resampling.
  _ResetModelAndIR(sampleRate, GetBlockSize());
  mToneStack->Reset(sampleRate, maxBlockSize);
  _UpdateLatency();
}

void ModernMetal::OnIdle()
{
  /*mInputSender.TransmitData(*this);
  mOutputSender.TransmitData(*this);*/

  if (mNewModelLoadedInDSP)
  {
    if (auto* pGraphics = GetUI())
    {
      _UpdateControlsFromModel();
      mNewModelLoadedInDSP = false;
    }
  }
  if (mModelCleared)
  {
    if (auto* pGraphics = GetUI())
    {
      // FIXME -- need to disable only the "normalized" model
      // pGraphics->GetControlWithTag(kCtrlTagOutputMode)->SetDisabled(false);
      //static_cast<NAMSettingsPageControl*>(pGraphics->GetControlWithTag(kCtrlTagSettingsBox))->ClearModelInfo();
      mModelCleared = false;
    }
  }
}

bool ModernMetal::SerializeState(IByteChunk& chunk) const
{
  WDL_String version(PLUG_VERSION_STR);
  chunk.PutStr(version.Get());
  
  SerializeEditorState(chunk);
  return SerializeParams(chunk);
}

int ModernMetal::UnserializeState(const IByteChunk& chunk, int startPos)
{
  WDL_String version;
  startPos = chunk.GetStr(version, startPos);

  startPos = UnserializeEditorState(chunk, startPos);
  startPos = UnserializeParams(chunk, startPos);
  if (GetParam(kIRToggle)->Bool()) {
    _StageIRCustom((int)true);
  }
  return startPos;
}

void ModernMetal::OnUIOpen()
{
  Plugin::OnUIOpen();

  if (mModel != nullptr)
  {
    _UpdateControlsFromModel();
  }
}

void ModernMetal::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    // Changes to the input gain
    case kCalibrateInput:
    {
      bool active = GetParam(paramIdx)->Bool();
      break;
    }
    case kInputCalibrationLevel:
    case kInputLevel: _SetInputGain(); break;
    // Changes to the output gain
    case kOutputLevel:
    case kOutputMode: _SetOutputGain(); break;
    // Change profile
    case kModelIndex: _StageModelCustom((int)GetParam(kModelIndex)->Value()); break;
    // Tone stack:
    case kToneBass: mToneStack->SetParam("bass", GetParam(paramIdx)->Value()); break;
    case kToneMid: mToneStack->SetParam("middle", GetParam(paramIdx)->Value()); break;
    case kToneTreble: mToneStack->SetParam("treble", GetParam(paramIdx)->Value()); break;
    default: break;
  }
}

void ModernMetal::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (auto pGraphics = GetUI())
  {
    

    bool active = GetParam(paramIdx)->Bool();
    switch (paramIdx)
    {
      case kIRToggle:
      {
        _StageIRCustom(active);
        break;
      }
      /*case kNoiseGateActive: pGraphics->GetControlWithParamIdx(kNoiseGateThreshold)->SetDisabled(!active); break;
      case kEQActive:
        pGraphics->ForControlInGroup("EQ_KNOBS", [active](IControl* pControl) { pControl->SetDisabled(!active); });
        break;
      case kIRToggle: pGraphics->GetControlWithTag(kCtrlTagIRFileBrowser)->SetDisabled(!active); break;*/
      default: break;
    }
  }
}

bool ModernMetal::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  switch (msgTag)
  {
    case kMsgTagClearModel: mShouldRemoveModel = true; return true;
    case kMsgTagClearIR: mShouldRemoveIR = true; return true;
    /*case kMsgTagHighlightColor:
    {
      mHighLightColor.Set((const char*)pData);

      if (GetUI())
      {
        GetUI()->ForStandardControlsFunc([&](IControl* pControl) {
          if (auto* pVectorBase = pControl->As<IVectorBase>())
          {
            IColor color = IColor::FromColorCodeStr(mHighLightColor.Get());

            pVectorBase->SetColor(kX1, color);
            pVectorBase->SetColor(kPR, color.WithOpacity(0.3f));
            pVectorBase->SetColor(kFR, color.WithOpacity(0.4f));
            pVectorBase->SetColor(kX3, color.WithContrast(0.1f));
          }
          pControl->GetUI()->SetAllControlsDirty();
        });
      }

      return true;
    }*/
    default: return false;
  }
}

// Private methods ============================================================

void ModernMetal::_AllocateIOPointers(const size_t nChans)
{
  if (mInputPointers != nullptr)
    throw std::runtime_error("Tried to re-allocate mInputPointers without freeing");
  mInputPointers = new sample*[nChans];
  if (mInputPointers == nullptr)
    throw std::runtime_error("Failed to allocate pointer to input buffer!\n");
  if (mOutputPointers != nullptr)
    throw std::runtime_error("Tried to re-allocate mOutputPointers without freeing");
  mOutputPointers = new sample*[nChans];
  if (mOutputPointers == nullptr)
    throw std::runtime_error("Failed to allocate pointer to output buffer!\n");
}

void ModernMetal::_ApplyDSPStaging()
{
  // Remove marked modules
  if (mShouldRemoveModel)
  {
    mModel = nullptr;
    mNAMPath.Set("");
    mShouldRemoveModel = false;
    mModelCleared = true;
    _UpdateLatency();
    _SetInputGain();
    _SetOutputGain();
  }
  if (mShouldRemoveIR)
  {
    mIR = nullptr;
    mIRPath.Set("");
    mShouldRemoveIR = false;
  }
  // Move things from staged to live
  if (mStagedModel != nullptr)
  {
    mModel = std::move(mStagedModel);
    mStagedModel = nullptr;
    mNewModelLoadedInDSP = true;
    _UpdateLatency();
    _SetInputGain();
    _SetOutputGain();
  }
  if (mStagedIR != nullptr)
  {
    mIR = std::move(mStagedIR);
    mStagedIR = nullptr;
  }
}

void ModernMetal::_DeallocateIOPointers()
{
  if (mInputPointers != nullptr)
  {
    delete[] mInputPointers;
    mInputPointers = nullptr;
  }
  if (mInputPointers != nullptr)
    throw std::runtime_error("Failed to deallocate pointer to input buffer!\n");
  if (mOutputPointers != nullptr)
  {
    delete[] mOutputPointers;
    mOutputPointers = nullptr;
  }
  if (mOutputPointers != nullptr)
    throw std::runtime_error("Failed to deallocate pointer to output buffer!\n");
}

void ModernMetal::_FallbackDSP(iplug::sample** inputs, iplug::sample** outputs, const size_t numChannels,
                                    const size_t numFrames)
{
  for (auto c = 0; c < numChannels; c++)
    for (auto s = 0; s < numFrames; s++)
      mOutputArray[c][s] = mInputArray[c][s];
}

void ModernMetal::_ResetModelAndIR(const double sampleRate, const int maxBlockSize)
{
  // Model
  if (mStagedModel != nullptr)
  {
    mStagedModel->Reset(sampleRate, maxBlockSize);
  }
  else if (mModel != nullptr)
  {
    mModel->Reset(sampleRate, maxBlockSize);
  }

  // IR
  if (mStagedIR != nullptr)
  {
    const double irSampleRate = mStagedIR->GetSampleRate();
    if (irSampleRate != sampleRate)
    {
      const auto irData = mStagedIR->GetData();
      mStagedIR = std::make_unique<dsp::ImpulseResponse>(irData, sampleRate);
    }
  }
  else if (mIR != nullptr)
  {
    const double irSampleRate = mIR->GetSampleRate();
    if (irSampleRate != sampleRate)
    {
      const auto irData = mIR->GetData();
      mStagedIR = std::make_unique<dsp::ImpulseResponse>(irData, sampleRate);
    }
  }
}

// TODO: add compensation multiplier
void ModernMetal::_SetInputGain()
{
  mInputGain = DBToAmp(GetParam(kInputLevel)->Value());

  /*
  iplug::sample inputGainDB = GetParam(kInputLevel)->Value();
  // Input calibration
  if ((mModel != nullptr) && (mModel->HasInputLevel()) && GetParam(kCalibrateInput)->Bool())
  {
    inputGainDB += GetParam(kInputCalibrationLevel)->Value() - mModel->GetInputLevel();
  }
  mInputGain = DBToAmp(inputGainDB);
  */
}

// TODO: bypass
void ModernMetal::_SetOutputGain()
{
  mOutputGain = DBToAmp(GetParam(kOutputLevel)->Value());

  /*
  double gainDB = GetParam(kOutputLevel)->Value();
  if (mModel != nullptr)
  {
    const int outputMode = GetParam(kOutputMode)->Int();
    switch (outputMode)
    {
      case 1: // Normalized
        if (mModel->HasLoudness())
        {
          const double loudness = mModel->GetLoudness();
          const double targetLoudness = -18.0;
          gainDB += (targetLoudness - loudness);
        }
        break;
      case 2: // Calibrated
        if (mModel->HasOutputLevel())
        {
          const double inputLevel = GetParam(kInputCalibrationLevel)->Value();
          const double outputLevel = mModel->GetOutputLevel();
          gainDB += (outputLevel - inputLevel);
        }
        break;
      case 0: // Raw
      default: break;
    }
  }
  mOutputGain = DBToAmp(gainDB);
  */
}

std::string ModernMetal::_StageModel(const WDL_String& modelPath)
{
  WDL_String previousNAMPath = mNAMPath;
  try
  {
    auto dspPath = std::filesystem::u8path(modelPath.Get());
    std::unique_ptr<nam::DSP> model = nam::get_dsp(dspPath);
    std::unique_ptr<ResamplingNAM> temp = std::make_unique<ResamplingNAM>(std::move(model), GetSampleRate());
    temp->Reset(GetSampleRate(), GetBlockSize());
    mStagedModel = std::move(temp);
    mNAMPath = modelPath;
    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  }
  catch (std::runtime_error& e)
  {
    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);

    if (mStagedModel != nullptr)
    {
      mStagedModel = nullptr;
    }
    mNAMPath = previousNAMPath;
    std::cerr << "Failed to read DSP module" << std::endl;
    std::cerr << e.what() << std::endl;
    return e.what();
  }
  return "";
}

dsp::wav::LoadReturnCode ModernMetal::_StageIR(const WDL_String& irPath)
{
  // FIXME it'd be better for the path to be "staged" as well. Just in case the
  // path and the model got caught on opposite sides of the fence...
  WDL_String previousIRPath = mIRPath;
  const double sampleRate = GetSampleRate();
  dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
  try
  {
    auto irPathU8 = std::filesystem::u8path(irPath.Get());
    mStagedIR = std::make_unique<dsp::ImpulseResponse>(irPathU8.string().c_str(), sampleRate);
    wavState = mStagedIR->GetWavState();
  }
  catch (std::runtime_error& e)
  {
    wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
    std::cerr << "Caught unhandled exception while attempting to load IR:" << std::endl;
    std::cerr << e.what() << std::endl;
  }

  if (wavState == dsp::wav::LoadReturnCode::SUCCESS)
  {
    mIRPath = irPath;
    SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  }
  else
  {
    if (mStagedIR != nullptr)
    {
      mStagedIR = nullptr;
    }
    mIRPath = previousIRPath;
    SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  return wavState;
}

size_t ModernMetal::_GetBufferNumChannels() const
{
  // Assumes input=output (no mono->stereo effects)
  return mInputArray.size();
}

size_t ModernMetal::_GetBufferNumFrames() const
{
  if (_GetBufferNumChannels() == 0)
    return 0;
  return mInputArray[0].size();
}

void ModernMetal::_InitToneStack()
{
  // If you want to customize the tone stack, then put it here!
  mToneStack = std::make_unique<dsp::tone_stack::BasicNamToneStack>();
}
void ModernMetal::_PrepareBuffers(const size_t numChannels, const size_t numFrames)
{
  const bool updateChannels = numChannels != _GetBufferNumChannels();
  const bool updateFrames = updateChannels || (_GetBufferNumFrames() != numFrames);
  //  if (!updateChannels && !updateFrames)  // Could we do this?
  //    return;

  if (updateChannels)
  {
    _PrepareIOPointers(numChannels);
    mInputArray.resize(numChannels);
    mOutputArray.resize(numChannels);
  }
  if (updateFrames)
  {
    for (auto c = 0; c < mInputArray.size(); c++)
    {
      mInputArray[c].resize(numFrames);
      std::fill(mInputArray[c].begin(), mInputArray[c].end(), 0.0);
    }
    for (auto c = 0; c < mOutputArray.size(); c++)
    {
      mOutputArray[c].resize(numFrames);
      std::fill(mOutputArray[c].begin(), mOutputArray[c].end(), 0.0);
    }
  }
  // Would these ever get changed by something?
  for (auto c = 0; c < mInputArray.size(); c++)
    mInputPointers[c] = mInputArray[c].data();
  for (auto c = 0; c < mOutputArray.size(); c++)
    mOutputPointers[c] = mOutputArray[c].data();
}

void ModernMetal::_PrepareIOPointers(const size_t numChannels)
{
  _DeallocateIOPointers();
  _AllocateIOPointers(numChannels);
}

// TODO: calculate compensation
void ModernMetal::_ProcessInput(iplug::sample** inputs, const size_t nFrames, const size_t nChansIn,
                                     const size_t nChansOut)
{
  // We'll assume that the main processing is mono for now. We'll handle dual amps later.
  if (nChansOut != 1)
  {
    std::stringstream ss;
    ss << "Expected mono output, but " << nChansOut << " output channels are requested!";
    throw std::runtime_error(ss.str());
  }

  // On the standalone, we can probably assume that the user has plugged into only one input and they expect it to be
  // carried straight through. Don't apply any division over nChansIn because we're just "catching anything out there."
  // However, in a DAW, it's probably something providing stereo, and we want to take the average in order to avoid
  // doubling the loudness. (This would change w/ double mono processing)
  double compensationMultiplier = GetParam(kInputCalibrationLevel)->Value();
  double gain = mInputGain * compensationMultiplier;
#ifndef APP_API
  gain /= (float)nChansIn;
#endif
  // Assume _PrepareBuffers() was already called
  for (size_t c = 0; c < nChansIn; c++) {
    for (size_t s = 0; s < nFrames; s++) {
      if (c == 0) {
        if (mLearnInput) {
          double sIn = std::abs(inputs[c][s]);
          peakMax = std::max(peakMax, sIn);
        }
        mInputArray[0][s] = gain * inputs[c][s];
      }
      else {
        mInputArray[0][s] += gain * inputs[c][s];
      }
    }
  }
}

void ModernMetal::_ProcessOutput(iplug::sample** inputs, iplug::sample** outputs, const size_t nFrames,
                                      const size_t nChansIn, const size_t nChansOut)
{
  const double gain = mOutputGain;
  // Assume _PrepareBuffers() was already called
  if (nChansIn != 1)
    throw std::runtime_error("Plugin is supposed to process in mono.");
  // Broadcast the internal mono stream to all output channels.
  const size_t cin = 0;
  for (auto cout = 0; cout < nChansOut; cout++)
    for (auto s = 0; s < nFrames; s++)
#ifdef APP_API // Ensure valid output to interface
      outputs[cout][s] = std::clamp(gain * inputs[cin][s], -1.0, 1.0);
#else // In a DAW, other things may come next and should be able to handle large
      // values.
      outputs[cout][s] = gain * inputs[cin][s];
#endif
}

void ModernMetal::_UpdateControlsFromModel()
{
  /*
  if (mModel == nullptr)
  {
    return;
  }
  if (auto* pGraphics = GetUI())
  {
    ModelInfo modelInfo;
    modelInfo.sampleRate.known = true;
    modelInfo.sampleRate.value = mModel->GetEncapsulatedSampleRate();
    modelInfo.inputCalibrationLevel.known = mModel->HasInputLevel();
    modelInfo.inputCalibrationLevel.value = mModel->HasInputLevel() ? mModel->GetInputLevel() : 0.0;
    modelInfo.outputCalibrationLevel.known = mModel->HasOutputLevel();
    modelInfo.outputCalibrationLevel.value = mModel->HasOutputLevel() ? mModel->GetOutputLevel() : 0.0;

    //static_cast<NAMSettingsPageControl*>(pGraphics->GetControlWithTag(kCtrlTagSettingsBox))->SetModelInfo(modelInfo);

    //const bool disableInputCalibrationControls = !mModel->HasInputLevel();
    //pGraphics->GetControlWithTag(kCtrlTagCalibrateInput)->SetDisabled(disableInputCalibrationControls);
    //pGraphics->GetControlWithTag(kCtrlTagInputCalibrationLevel)->SetDisabled(disableInputCalibrationControls);
    {
      auto* c = static_cast<OutputModeControl*>(pGraphics->GetControlWithTag(kCtrlTagOutputMode));
      c->SetNormalizedDisable(!mModel->HasLoudness());
      c->SetCalibratedDisable(!mModel->HasOutputLevel());
    }
  }
  */
}

void ModernMetal::_UpdateLatency()
{
  int latency = 0;
  if (mModel)
  {
    latency += mModel->GetLatency();
  }
  // Other things that add latency here...

  // Feels weird to have to do this.
  if (GetLatency() != latency)
  {
    SetLatency(latency);
  }
}

void ModernMetal::_UpdateMeters(sample** inputPointer, sample** outputPointer, const size_t nFrames,
                                     const size_t nChansIn, const size_t nChansOut)
{
  // Right now, we didn't specify MAXNC when we initialized these, so it's 1.
  const int nChansHack = 1;
  /*mInputSender.ProcessBlock(inputPointer, (int)nFrames, kCtrlTagInputMeter, nChansHack);
  mOutputSender.ProcessBlock(outputPointer, (int)nFrames, kCtrlTagOutputMeter, nChansHack);*/
}

// HACK
#include "Unserialization.cpp"
