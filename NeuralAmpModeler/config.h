#define PLUG_NAME "Modern Metal"
#define PLUG_MFR "NamePending"
#define PLUG_VERSION_HEX 0x00000709
#define PLUG_VERSION_STR "0.0.0"
#define PLUG_UNIQUE_ID '1YEo'
#define PLUG_MFR_ID 'NaPd'
#define PLUG_URL_STR "https://github.com/nezvers"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2024 NamePending"
#define PLUG_CLASS_NAME NeuralAmpModeler
#define BUNDLE_NAME "Modern Metal"
#define BUNDLE_MFR "NamePending"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "NeuralAmpModeler"

#ifdef APP_API
  #define PLUG_CHANNEL_IO "1-2"
#else
  #define PLUG_CHANNEL_IO "1-1 1-2 2-2"
#endif

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 500
#define PLUG_HEIGHT 300
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0
#define PLUG_MAX_WIDTH PLUG_WIDTH * 4
#define PLUG_MAX_HEIGHT PLUG_HEIGHT * 4

#define AUV2_ENTRY NeuralAmpModeler_Entry
#define AUV2_ENTRY_STR "NeuralAmpModeler_Entry"
#define AUV2_FACTORY NeuralAmpModeler_Factory
#define AUV2_VIEW_CLASS NeuralAmpModeler_View
#define AUV2_VIEW_CLASS_STR "NeuralAmpModeler_View"

#define AAX_TYPE_IDS 'ITP1'
#define AAX_TYPE_IDS_AUDIOSUITE 'ITA1'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "NeuralAmpModeler\nIPEF"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
#define MICHROMA_FN "Michroma-Regular.ttf"

#define HELP_FN "Help.svg"
#define FILE_FN "File.svg"
#define CLOSE_BUTTON_FN "Cross.svg"
#define LEFT_ARROW_FN "ArrowLeft.svg"
#define RIGHT_ARROW_FN "ArrowRight.svg"
#define MODEL_ICON_FN "ModelIcon.svg"
#define IR_BUTTON_ON_FN "IR_button_on.png"
#define IR_BUTTON_ON2X_FN "IR_button_on@2x.png"
#define IR_BUTTON_ON3X_FN "IR_button_on@3x.png"
#define IR_BUTTON_OFF_FN "IR_button_off.png"
#define IR_BUTTON_OFF2X_FN "IR_button_off@2x.png"
#define IR_BUTTON_OFF3X_FN "IR_button_off@3x.png"
#define LEARN_BUTTON_FN "learn_button.png"
#define LEARN_BUTTON2X_FN "learn_button@2x.png"
#define LEARN_BUTTON3X_FN "learn_button@3x.png"

#define BACKGROUND_FN "Background.png"
#define BACKGROUND2X_FN "Background@2x.png"
#define BACKGROUND3X_FN "Background@3x.png"
#define KNOBBACKGROUND_FN "KnobBackground.png"
#define KNOBBACKGROUND2X_FN "KnobBackground@2x.png"
#define KNOBBACKGROUND3X_FN "KnobBackground@3x.png"
#define FILEBACKGROUND_FN "FileBackground.png"
#define FILEBACKGROUND2X_FN "FileBackground@2x.png"
#define FILEBACKGROUND3X_FN "FileBackground@3x.png"
#define LINES_FN "Lines.png"
#define LINES2X_FN "Lines@2x.png"
#define LINES3X_FN "Lines@3x.png"
#define SLIDESWITCHHANDLE_FN "SlideSwitchHandle.png"
#define SLIDESWITCHHANDLE2X_FN "SlideSwitchHandle@2x.png"
#define SLIDESWITCHHANDLE3X_FN "SlideSwitchHandle@3x.png"

#define METERBACKGROUND_FN "MeterBackground.png"
#define METERBACKGROUND2X_FN "MeterBackground@2x.png"
#define METERBACKGROUND3X_FN "MeterBackground@3x.png"

// Issue 291
// On the macOS standalone, we might not have permissions to traverse the file directory, so we have the app ask the
// user to pick a directory instead of the file in the directory.
// Everyone else is fine though.
#if defined(APP_API) && defined(__APPLE__)
  #define NAM_PICK_DIRECTORY
#endif
