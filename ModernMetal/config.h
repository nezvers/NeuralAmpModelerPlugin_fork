#define PLUG_NAME "ModernMetal"
#define PLUG_MFR "Wide Fox"
#define PLUG_VERSION_HEX 0x00000000
#define PLUG_VERSION_STR "0.0.00"
#define PLUG_UNIQUE_ID 'wfmm'
#define PLUG_MFR_ID 'WiFo'
#define PLUG_URL_STR "https://github.com/sdatkinson/ModernMetalPlugin"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2024 Wide Fox"
#define PLUG_CLASS_NAME ModernMetal
#define BUNDLE_NAME "ModernMetal"
#define BUNDLE_MFR "WideFox"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "ModernMetal"

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
#define PLUG_WIDTH 300
#define PLUG_HEIGHT 420
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0
#define PLUG_MAX_WIDTH PLUG_WIDTH * 4
#define PLUG_MAX_HEIGHT PLUG_HEIGHT * 4

#define AUV2_ENTRY ModernMetal_Entry
#define AUV2_ENTRY_STR "ModernMetal_Entry"
#define AUV2_FACTORY ModernMetal_Factory
#define AUV2_VIEW_CLASS ModernMetal_View
#define AUV2_VIEW_CLASS_STR "ModernMetal_View"

#define AAX_TYPE_IDS 'ITP1'
#define AAX_TYPE_IDS_AUDIOSUITE 'ITA1'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "ModernMetal\nIPEF"
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
#define JAKARTA_FN "PlusJakartaSans-Bold.ttf"

#define BACKGROUND_DARK_FN "background_dark_.png"
#define BACKGROUND_DARK2X_FN "background_dark_@2x.png"
#define BACKGROUND_DARK3X_FN "background_dark_@3x.png"
#define BACKGROUND_LIGHT_FN "background_light_.png"
#define BACKGROUND_LIGHT2X_FN "background_light_@2x.png"
#define BACKGROUND_LIGHT3X_FN "background_light_@3x.png"
#define BIGKNOB_DARK_FN "bigknob_dark_.png"
#define BIGKNOB_DARK2X_FN "bigknob_dark_@2x.png"
#define BIGKNOB_DARK3X_FN "bigknob_dark_@3x.png"
#define BIGKNOB_LIGHT_FN "bigknob_light_.png"
#define BIGKNOB_LIGHT2X_FN "bigknob_light_@2x.png"
#define BIGKNOB_LIGHT3X_FN "bigknob_light_@3x.png"
#define BUTTON_DARK_FN "button_dark_.png"
#define BUTTON_DARK2X_FN "button_dark_@2x.png"
#define BUTTON_DARK3X_FN "button_dark_@3x.png"
#define BUTTON_LIGHT_FN "button_light_.png"
#define BUTTON_LIGHT2X_FN "button_light_@2x.png"
#define BUTTON_LIGHT3X_FN "button_light_@3x.png"
#define BUTTON_PRESS_DARK_FN "button_press_dark_.png"
#define BUTTON_PRESS_DARK2X_FN "button_press_dark_@2x.png"
#define BUTTON_PRESS_DARK3X_FN "button_press_dark_@3x.png"
#define BUTTON_PRESS_LIGHT_FN "button_press_light_.png"
#define BUTTON_PRESS_LIGHT2X_FN "button_press_light_@2x.png"
#define BUTTON_PRESS_LIGHT3X_FN "button_press_light_@3x.png"
#define EKRANS_FN "ekrans_.png"
#define EKRANS2X_FN "ekrans_@2x.png"
#define EKRANS3X_FN "ekrans_@3x.png"
#define LISTENING_DARK_FN "listening_dark_.png"
#define LISTENING_DARK2X_FN "listening_dark_@2x.png"
#define LISTENING_DARK3X_FN "listening_dark_@3x.png"
#define LISTENING_LIGHT_FN "listening_light_.png"
#define LISTENING_LIGHT2X_FN "listening_light_@2x.png"
#define LISTENING_LIGHT3X_FN "listening_light_@3x.png"
#define LOGO_DARK_FN "logo_dark_.png"
#define LOGO_DARK2X_FN "logo_dark_@2x.png"
#define LOGO_DARK3X_FN "logo_dark_@3x.png"
#define LOGO_LIGHT_FN "logo_light_.png"
#define LOGO_LIGHT2X_FN "logo_light_@2x.png"
#define LOGO_LIGHT3X_FN "logo_light_@3x.png"
#define LOGO_HOVER_DARK_FN "logo_hover_dark_.png"
#define LOGO_HOVER_DARK2X_FN "logo_hover_dark_@2x.png"
#define LOGO_HOVER_DARK3X_FN "logo_hover_dark_@3x.png"
#define LOGO_HOVER_LIGHT_FN "logo_hover_light_.png"
#define LOGO_HOVER_LIGHT2X_FN "logo_hover_light_@2x.png"
#define LOGO_HOVER_LIGHT3X_FN "logo_hover_light_@3x.png"
#define PUMPA_OFF_DARK_FN "pumpa_off_dark_.png"
#define PUMPA_OFF_DARK2X_FN "pumpa_off_dark_@2x.png"
#define PUMPA_OFF_DARK3X_FN "pumpa_off_dark_@3x.png"
#define PUMPA_OFF_LIGHT_FN "pumpa_off_light_.png"
#define PUMPA_OFF_LIGHT2X_FN "pumpa_off_light_@2x.png"
#define PUMPA_OFF_LIGHT3X_FN "pumpa_off_light_@3x.png"
#define PUMPA_ON_DARK_FN "pumpa_on_dark_.png"
#define PUMPA_ON_DARK2X_FN "pumpa_on_dark_@2x.png"
#define PUMPA_ON_DARK3X_FN "pumpa_on_dark_@3x.png"
#define PUMPA_ON_LIGHT_FN "pumpa_on_light_.png"
#define PUMPA_ON_LIGHT2X_FN "pumpa_on_light_@2x.png"
#define PUMPA_ON_LIGHT3X_FN "pumpa_on_light_@3x.png"
#define SMALLKNOB_DARK_FN "smallknob_dark_.png"
#define SMALLKNOB_DARK2X_FN "smallknob_dark_@2x.png"
#define SMALLKNOB_DARK3X_FN "smallknob_dark_@3x.png"
#define SMALLKNOB_LIGHT_FN "smallknob_light_.png"
#define SMALLKNOB_LIGHT2X_FN "smallknob_light_@2x.png"
#define SMALLKNOB_LIGHT3X_FN "smallknob_light_@3x.png"
#define BULTA_FN "bulta.png"
#define BULTA2X_FN "bulta@2x.png"
#define BULTA3X_FN "bulta@3x.png"


// Issue 291
// On the macOS standalone, we might not have permissions to traverse the file directory, so we have the app ask the
// user to pick a directory instead of the file in the directory.
// Everyone else is fine though.
#if defined(APP_API) && defined(__APPLE__)
  #define NAM_PICK_DIRECTORY
#endif
