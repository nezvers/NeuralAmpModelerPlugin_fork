#include <algorithm> // std::clamp
#include <cmath> // pow
#include <filesystem>
#include <iostream>
#include <utility>

#include "Colors.h"
#include "NeuralAmpModelerCore/NAM/activations.h"
// clang-format off
// These includes need to happen in this order or else the latter won't know
// a bunch of stuff.
#include "NeuralAmpModeler.h"
#include "IPlug_include_in_plug_src.h"
// clang-format on
#include "architecture.hpp"

#include "NeuralAmpModelerControls.h"

// <---- _modified_
#include <NeuralAmpModelerCore/NAM/lstm.h>
#include <NeuralAmpModelerCore/NAM/wavenet.h>

#include "Caption.h"
#include "Widgets.h"
#include "wav_parser.h"
#include "mmfat_nam.h"
#include "mmtight_nam.h"
#include "rectal57.h"
#include "conversion_math.h"
const char* nam_list[] = {mmfat_nam, mmtight_nam};
// <----

using namespace iplug;
using namespace igraphics;

const double kDCBlockerFrequency = 5.0;

// Styles
const IVColorSpec colorSpec{
  DEFAULT_BGCOLOR, // Background
  PluginColors::NAM_THEMECOLOR.WithContrast(-0.15f), // Foreground
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.8f), // Pressed
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.4f), // Frame
  PluginColors::MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  PluginColors::NAM_THEMECOLOR, // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  PluginColors::NAM_THEMECOLOR.WithContrast(0.1f), // Extra 3
};

const IVColorSpec colorSpecSelected{
  DEFAULT_BGCOLOR, // Background
  PluginColors::NAM_THEMECOLOR.WithContrast(0.15f), // Foreground
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.8f), // Pressed
  PluginColors::NAM_THEMECOLOR.WithOpacity(0.4f), // Frame
  PluginColors::MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  PluginColors::NAM_THEMECOLOR, // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  PluginColors::NAM_THEMECOLOR.WithContrast(0.1f), // Extra 3
};

const iplug::igraphics::IColor COLOR_ACTIVE = IColor::FromColorCode(0x8b9fff);
const iplug::igraphics::IColor COLOR_ACTIVE_DARK = IColor::FromColorCode(0x392946);

const IVStyle style =
  IVStyle{true, // Show label
          true, // Show value
          colorSpec,
          {25.0, EVAlign::Middle, COLOR_ACTIVE}, // Knob label text5
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

const IVStyle labelStyle = DEFAULT_STYLE.WithValueText(IText(25, COLOR_WHITE, "Roboto-Regular")).WithDrawFrame(false).WithShadowOffset(0.f);
const IVStyle sliderStyle =
  DEFAULT_STYLE.WithValueText(IText(20, COLOR_WHITE, "Roboto-Regular")).WithDrawFrame(false).WithShadowOffset(0.f);
const IVStyle titleStyle =
  DEFAULT_STYLE.WithValueText(IText(60, COLOR_WHITE, "Michroma-Regular")).WithDrawFrame(false).WithShadowOffset(0.f);

const iplug::igraphics::IText TEXT_CAPTION = IText(25.f, COLOR_ACTIVE, "Roboto-Regular", EAlign::Center, EVAlign::Middle);

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

int NeuralAmpModeler::_StageModelCustom(int modelIndex)
{
  try
  {
    
    std::unique_ptr<nam::DSP> model = get_dsp_json(nam_list[modelIndex]);
    std::unique_ptr<ResamplingNAM> temp = std::make_unique<ResamplingNAM>(std::move(model), GetSampleRate());
    temp->Reset(GetSampleRate(), GetBlockSize());
    mStagedModel = std::move(temp);
    //SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  }
  catch (std::runtime_error& e)
  {
    //SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);

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

dsp::wav::LoadReturnCode NeuralAmpModeler::_StageIRCustom(bool enabled)
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
    //SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  }
  else
  {
    if (mStagedIR != nullptr)
    {
      mStagedIR = nullptr;
    }
    //SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  return wavState;
}

NeuralAmpModeler::NeuralAmpModeler(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  IrWavFileData = WAV_ParseFileData(RECTAL57);
  
  IrRawAudio = WAV_ExtractSamples(IrWavFileData);

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
  GetParam(kOutNorm)->InitBool("OutNorm", true);
  GetParam(kIRToggle)->InitBool("IRToggle", true);
  GetParam(kModelIndex)->InitEnum("Model Index", 0, kModelCount, "", 0, "", MODEL_NAMES);
  GetParam(kPeakTargetDb)->InitGain("DI Peak Target", -3.0, -60.0, 0.0, 0.1);
  GetParam(kPeakMinDb)->InitGain("DI Learn Min", -30.0, -60.0, 0.0, 0.1);
  GetParam(kPeakCompensation)->InitDouble("DI Compensation", 1.0, 0.0001, 1000.0, 0.1);


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
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachTextEntryControl();
    pGraphics->EnableMouseOver(true);
    pGraphics->EnableTooltips(true);
    pGraphics->EnableMultiTouch(true);

    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->LoadFont("Michroma-Regular", MICHROMA_FN);

    const auto helpSVG = pGraphics->LoadSVG(HELP_FN);
    const auto irButtonOnBitmap = pGraphics->LoadBitmap(IR_BUTTON_ON_FN);
    const auto irButtonOffBitmap = pGraphics->LoadBitmap(IR_BUTTON_OFF_FN);
    const auto learnButtonBitmap = pGraphics->LoadBitmap(LEARN_BUTTON_FN);

    const auto backgroundBitmap = pGraphics->LoadBitmap(BACKGROUND_FN);
    const auto knobBackgroundBitmap = pGraphics->LoadBitmap(KNOBBACKGROUND_FN);
    const auto meterBackgroundBitmap = pGraphics->LoadBitmap(METERBACKGROUND_FN);

    const auto b = pGraphics->GetBounds();


    // Learn Button
    std::function<void(IControl*)> ClickCallback = [&](IControl* iControl) {
      ((IVButtonControl*)iControl)->SetLabelStr("Listening...");
      mLearnInput = true;
      GetParam(kPeakCompensation)->Set(1.0);
      SendParameterValueFromDelegate(kPeakCompensation, 1.0, false);
      DirtyParametersFromUI();
      peakMax = 0.0;
    };
    std::function<void(IControl*)> TimeoutCallback = [&](IControl* iControl) {
      ((IVButtonControl*)iControl)->SetLabelStr("Auto Gain");
      mLearnInput = false;
      _UpdateCompensation();
    };

    float w = 400;
    float h = 80;
    float x = b.R * 0.5 - w * 0.5;
    float y = 10;
    pGraphics->AttachBackground(BACKGROUND_FN);
    // TITLE
    pGraphics->AttachControl(new IVLabelControl(IRECT(x, y, x + w, y + h), "Modern Metal", titleStyle));

    // GAIN
    x = 30;
    y = 60;
    w = 210;
    h = 210;
    IVStyle knobStyle = style;
    knobStyle.showValue = false;
    knobStyle.showLabel = false;
    pGraphics->AttachControl(
      new BitmapKnob(IRECT(x, y, x + w, y + h), kInputLevel, "", knobStyle, knobBackgroundBitmap));
    pGraphics->AttachControl(new IVLabelControl(IRECT(x, y, x + w, y + h), "GAIN", labelStyle));

    // PROFILE
    w = 150;
    h = 50;
    x = x + 210 + 40;
    y = y + 50;
    pGraphics->AttachControl(new CaptionBitmap(IRECT(x, y, x + w, y + h), kModelIndex, TEXT_CAPTION, COLOR_ACTIVE_DARK,&learnButtonBitmap, true),kCtrlTagProfile);

    // LEARN
    w = 150;
    h = 50;
    x = x;
    y = y + h + 10;
    pGraphics->AttachControl(new NEZButtonTimer(
      IRECT(x, y, x + w, y + h), ClickCallback, TimeoutCallback, 10000, "Auto Gain", style, &learnButtonBitmap));

    // IR
    w = 50;
    h = 50;
    x = b.R - w - 10;
    y = b.B - h - 10;
    pGraphics->AttachControl(
      new BitmapSwitch(IRECT(x, y, x + w, y + h), kIRToggle, irButtonOnBitmap, irButtonOffBitmap));
    

    // The meters
    h = 30;
    w = 150;
    x = x - w - 10;
    y = y + 10;
    //pGraphics->AttachControl(new NAMMeterControl(IRECT(x, y, x + w, y + h), meterBackgroundBitmap, style), kCtrlTagInputMeter);
    pGraphics->AttachControl(
      new LinePeakControl(IRECT(x, y, x + w, y + h), style, EDirection::Horizontal), kCtrlTagOutputMeter);
    pGraphics->AttachControl(
      new SliderHandle(IRECT(x, y, x + w, y + h), kOutputLevel, "", knobStyle, false, EDirection::Horizontal));

    w = 40;
    h = h;
    x = x - w;
    y = y;
    pGraphics->AttachControl(new IVLabelControl(IRECT(x, y, x + w, y + h), "OUT", sliderStyle));

    /*
    // Help/about box
    x = 470;
    y = 30;
    w = 30;
    h = 30;
    pGraphics->AttachControl(new NAMCircleButtonControl(
      IRECT(x, y, x + w, y + h),
      [pGraphics](IControl* pCaller) {
        pGraphics->GetControlWithTag(kCtrlTagAboutBox)->As<NAMAboutBoxControl>()->HideAnimated(false);
      },
      helpSVG));

    // ABOUT
    pGraphics->AttachControl(new NAMAboutBoxControl(b, backgroundBitmap, style), kCtrlTagAboutBox)->Hide(true);

    pGraphics->ForAllControlsFunc([](IControl* pControl) {
      pControl->SetMouseEventsWhenDisabled(true);
      pControl->SetMouseOverWhenDisabled(true);
    });
    */
  };
}

NeuralAmpModeler::~NeuralAmpModeler()
{
  _DeallocateIOPointers();
}

void NeuralAmpModeler::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
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
    // TODO multi-channel processing; Issue
    // Make sure it's multi-threaded or else this won't perform well!
    mModel->process(triggerOutput[0], mOutputPointers[0], nFrames);
    mModel->finalize_(nFrames);
    // Normalize loudness
    if (GetParam(kOutNorm)->Value())
    {
      _NormalizeModelOutput(mOutputPointers, numChannelsInternal, numFrames);
    }
  }
  else
  {
    _FallbackDSP(triggerOutput, mOutputPointers, numChannelsInternal, numFrames);
  }
  // Apply the noise gate
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

void NeuralAmpModeler::OnReset()
{
  const auto sampleRate = GetSampleRate();
  const int maxBlockSize = GetBlockSize();

  // Tail is because the HPF DC blocker has a decay.
  // 10 cycles should be enough to pass the VST3 tests checking tail behavior.
  // I'm ignoring the model & IR, but it's not the end of the world.
  const int tailCycles = 10;
  SetTailSize(tailCycles * (int)(sampleRate / kDCBlockerFrequency));
  mInputSender.Reset(sampleRate);
  mOutputSender.Reset(sampleRate);
  // If there is a model or IR loaded, they need to be checked for resampling.
  _ResetModelAndIR(sampleRate, GetBlockSize());
  mToneStack->Reset(sampleRate, maxBlockSize);
}

void NeuralAmpModeler::OnIdle()
{
  mInputSender.TransmitData(*this);
  mOutputSender.TransmitData(*this);

  /*if (mNewModelLoadedInDSP)
  {
    if (auto* pGraphics = GetUI())
      pGraphics->GetControlWithTag(kCtrlTagOutNorm)->SetDisabled(!mModel->HasLoudness());

    mNewModelLoadedInDSP = false;
  }*/
}

bool NeuralAmpModeler::SerializeState(IByteChunk& chunk) const
{
  // If this isn't here when unserializing, then we know we're dealing with something before v0.8.0.
  WDL_String header("###NeuralAmpModeler###"); // Don't change this!
  chunk.PutStr(header.Get());
  // Plugin version, so we can load legacy serialized states in the future!
  WDL_String version(PLUG_VERSION_STR);
  chunk.PutStr(version.Get());
  // Model directory (don't serialize the model itself; we'll just load it again
  // when we unserialize)
  //chunk.PutStr(mNAMPath.Get());
  //chunk.PutStr(mIRPath.Get());
  return SerializeParams(chunk);
}

int NeuralAmpModeler::UnserializeState(const IByteChunk& chunk, int startPos)
{
  WDL_String header;
  startPos = chunk.GetStr(header, startPos);
  // TODO: Handle legacy plugin serialized states.
  // if strncmp (header.Get(), "###NeuralAmpModeler###")
  //{
  //  return UnserializeStateLegacy(header, startPos);  // (We'll assume 0.7.9).
  //}
  WDL_String version;
  startPos = chunk.GetStr(version, startPos);
  // Version-specific loading here if needed.
  // ...

  // Current version loading:
  //startPos = chunk.GetStr(mNAMPath, startPos);
  //startPos = chunk.GetStr(mIRPath, startPos);
  int retcode = UnserializeParams(chunk, startPos);
  //if (mNAMPath.GetLength())
  //  _StageModel(mNAMPath);
  if (GetParam(kIRToggle)->Bool())
    _StageIRCustom((int)true);
  return retcode;
}

void NeuralAmpModeler::OnUIOpen()
{
  Plugin::OnUIOpen();

  //if (mNAMPath.GetLength())
  //{
  //  SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  //  // If it's not loaded yet, then mark as failed.
  //  // If it's yet to be loaded, then the completion handler will set us straight once it runs.
  //  if (mModel == nullptr && mStagedModel == nullptr)
  //    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);
  //}

  /*if (mIRPath.GetLength())
  {
    SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
    if (mIR == nullptr && mStagedIR == nullptr)
      SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }*/

  /*if (mModel != nullptr)
    GetUI()->GetControlWithTag(kCtrlTagOutNorm)->SetDisabled(!mModel->HasLoudness());*/
}

void NeuralAmpModeler::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
    case kToneBass: mToneStack->SetParam("bass", GetParam(paramIdx)->Value()); break;
    case kToneMid: mToneStack->SetParam("middle", GetParam(paramIdx)->Value()); break;
    case kToneTreble: mToneStack->SetParam("treble", GetParam(paramIdx)->Value()); break;
    case kModelIndex: _StageModelCustom((int)GetParam(kModelIndex)->Value()); break;
    default: break;
  }
}

void NeuralAmpModeler::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (auto pGraphics = GetUI())
  {
    bool active = GetParam(paramIdx)->Bool();

    switch (paramIdx)
    {
      case kIRToggle:
      {
        _StageIRCustom((int)active);
        break;
      }
      default: break;
    }
  }
}

bool NeuralAmpModeler::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  switch (msgTag)
  {
    case kMsgTagClearModel: mShouldRemoveModel = true; return true;
    case kMsgTagClearIR: mShouldRemoveIR = true; return true;
    case kMsgTagHighlightColor:
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
    }
    default: return false;
  }
}

// Private methods ============================================================

void NeuralAmpModeler::_AllocateIOPointers(const size_t nChans)
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

void NeuralAmpModeler::_ApplyDSPStaging()
{
  // Remove marked modules
  if (mShouldRemoveModel)
  {
    mModel = nullptr;
    mShouldRemoveModel = false;
    SetLatency(0);
  }
  if (mShouldRemoveIR)
  {
    mIR = nullptr;
    //mIRPath.Set("");
    mShouldRemoveIR = false;
  }
  // Move things from staged to live
  if (mStagedModel != nullptr)
  {
    // Move from staged to active DSP
    mModel = std::move(mStagedModel);
    mStagedModel = nullptr;
    mNewModelLoadedInDSP = true;
    SetLatency(mModel->GetLatency());
  }
  if (mStagedIR != nullptr)
  {
    mIR = std::move(mStagedIR);
    mStagedIR = nullptr;
  }
}

void NeuralAmpModeler::_DeallocateIOPointers()
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

void NeuralAmpModeler::_FallbackDSP(iplug::sample** inputs, iplug::sample** outputs, const size_t numChannels,
                                    const size_t numFrames)
{
  for (auto c = 0; c < numChannels; c++)
    for (auto s = 0; s < numFrames; s++)
      mOutputArray[c][s] = mInputArray[c][s];
}

void NeuralAmpModeler::_NormalizeModelOutput(iplug::sample** buffer, const size_t numChannels, const size_t numFrames)
{
  if (!mModel)
    return;
  if (!mModel->HasLoudness())
    return;
  const double loudness = mModel->GetLoudness();
  const double targetLoudness = -18.0;
  const double gain = pow(10.0, (targetLoudness - loudness) / 20.0);
  for (size_t c = 0; c < numChannels; c++)
  {
    for (size_t f = 0; f < numFrames; f++)
    {
      buffer[c][f] *= gain;
    }
  }
}

void NeuralAmpModeler::_ResetModelAndIR(const double sampleRate, const int maxBlockSize)
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

std::string NeuralAmpModeler::_StageModel(const WDL_String& modelPath)
{
  //WDL_String previousNAMPath = mNAMPath;
  try
  {
    //auto dspPath = std::filesystem::u8path(modelPath.Get());
    //std::unique_ptr<nam::DSP> model = nam::get_dsp(dspPath);
    std::unique_ptr<nam::DSP> model = get_dsp_json(mmtight_nam);
    std::unique_ptr<ResamplingNAM> temp = std::make_unique<ResamplingNAM>(std::move(model), GetSampleRate());
    temp->Reset(GetSampleRate(), GetBlockSize());
    mStagedModel = std::move(temp);
    //mNAMPath = modelPath;
    //SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadedModel, mNAMPath.GetLength(), mNAMPath.Get());
  }
  catch (std::runtime_error& e)
  {
    SendControlMsgFromDelegate(kCtrlTagModelFileBrowser, kMsgTagLoadFailed);

    if (mStagedModel != nullptr)
    {
      mStagedModel = nullptr;
    }
    //mNAMPath = previousNAMPath;
    std::cerr << "Failed to read DSP module" << std::endl;
    std::cerr << e.what() << std::endl;
    return e.what();
  }
  return "";
}

dsp::wav::LoadReturnCode NeuralAmpModeler::_StageIR(const WDL_String& irPath)
{
  // FIXME it'd be better for the path to be "staged" as well. Just in case the
  // path and the model got caught on opposite sides of the fence...
  //WDL_String previousIRPath = mIRPath;
  const double sampleRate = GetSampleRate();
  dsp::wav::LoadReturnCode wavState = dsp::wav::LoadReturnCode::ERROR_OTHER;
  try
  {
    auto irPathU8 = std::filesystem::u8path(irPath.Get());
    mStagedIR = std::make_unique<dsp::ImpulseResponse>(irPathU8.string().c_str(), sampleRate); // <- Localize
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
    //mIRPath = irPath;
    //SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadedIR, mIRPath.GetLength(), mIRPath.Get());
  }
  else
  {
    if (mStagedIR != nullptr)
    {
      mStagedIR = nullptr;
    }
    //mIRPath = previousIRPath;
    //SendControlMsgFromDelegate(kCtrlTagIRFileBrowser, kMsgTagLoadFailed);
  }

  return wavState;
}

size_t NeuralAmpModeler::_GetBufferNumChannels() const
{
  // Assumes input=output (no mono->stereo effects)
  return mInputArray.size();
}

size_t NeuralAmpModeler::_GetBufferNumFrames() const
{
  if (_GetBufferNumChannels() == 0)
    return 0;
  return mInputArray[0].size();
}

void NeuralAmpModeler::_InitToneStack()
{
  // If you want to customize the tone stack, then put it here!
  mToneStack = std::make_unique<dsp::tone_stack::BasicNamToneStack>();
}

void NeuralAmpModeler::_PrepareBuffers(const size_t numChannels, const size_t numFrames)
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

void NeuralAmpModeler::_PrepareIOPointers(const size_t numChannels)
{
  _DeallocateIOPointers();
  _AllocateIOPointers(numChannels);
}

void NeuralAmpModeler::_ProcessInput(iplug::sample** inputs, const size_t nFrames, const size_t nChansIn,
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
  // carried straight through. Don't apply any division over nCahnsIn because we're just "catching anything out there."
  // However, in a DAW, it's probably something providing stereo, and we want to take the average in order to avoid
  // doubling the loudness.
#ifdef APP_API
  const double gain = pow(10.0, GetParam(kInputLevel)->Value() / 20.0);
#else
  const double gain = pow(10.0, GetParam(kInputLevel)->Value() / 20.0) / (float)nChansIn;
#endif
  // Assume _PrepareBuffers() was already called
  for (size_t c = 0; c < nChansIn; c++)
    for (size_t s = 0; s < nFrames; s++)
      if (c == 0)
        mInputArray[0][s] = gain * inputs[c][s] * GetParam(kPeakCompensation)->Value();
      else
        mInputArray[0][s] += gain * inputs[c][s] * GetParam(kPeakCompensation)->Value();
}

void NeuralAmpModeler::_ProcessOutput(iplug::sample** inputs, iplug::sample** outputs, const size_t nFrames,
                                      const size_t nChansIn, const size_t nChansOut)
{
  const double gain = db_to_volume(GetParam(kOutputLevel)->Value()); //pow(10.0, GetParam(kOutputLevel)->Value() / 20.0);

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

void NeuralAmpModeler::_UpdateMeters(sample** inputPointer, sample** outputPointer, const size_t nFrames,
                                     const size_t nChansIn, const size_t nChansOut)
{
  // Right now, we didn't specify MAXNC when we initialized these, so it's 1.
  const int nChansHack = 1;
  //mInputSender.ProcessBlock(inputPointer, (int)nFrames, kCtrlTagInputMeter, nChansHack);
  mOutputSender.ProcessBlock(outputPointer, (int)nFrames, kCtrlTagOutputMeter, nChansHack);
  if (mLearnInput) {
    _LearnMaxPeak(inputPointer, (int)nFrames, nChansHack);
  }
}

void NeuralAmpModeler::_UpdateCompensation() {
  double peakMaxDb = volume_to_db(peakMax);
  if (peakMaxDb < GetParam(kPeakMinDb)->Value())
  {
    GetParam(kPeakCompensation)->Set(1.0);
    SendParameterValueFromDelegate(kPeakCompensation, 1.0, false);
    DirtyParametersFromUI();
    return;
  }
  double peakTarget = db_to_volume(GetParam(kPeakTargetDb)->Value());
  double value = peakTarget / peakMax;
  GetParam(kPeakCompensation)->Set(value);
  SendParameterValueFromDelegate(kPeakCompensation, value, false);
  DirtyParametersFromUI(); // updates the host with the new parameter values
}

void NeuralAmpModeler::_LearnMaxPeak(iplug::sample** inputs, int nFrames, const size_t nChansIn) {
  for (auto cin = 0; cin < nChansIn; cin++)  {
    for (auto s = 0; s < nFrames; s++) {
      double sIn = std::abs(inputs[cin][s]);
      peakMax = std::max(peakMax, sIn);
    }
  }
}
