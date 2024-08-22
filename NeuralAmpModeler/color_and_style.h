#pragma once

#include "IGraphicsStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

const struct iplug::igraphics::IColor THEME_BLUE(255, 80, 133, 232);
const iplug::igraphics::IColor THEME_RED(255, 220, 50, 47);
const iplug::igraphics::IColor THEME_GREEN(255, 133, 153, 0);
const iplug::igraphics::IColor MOUSEOVER = THEME_BLUE.WithOpacity(0.1f);
const iplug::igraphics::IColor THEMEFONTCOLOR(255, 242, 242, 242);

const iplug::igraphics::IColor COLOR_BG = IColor::FromColorCode(0x050914);
const iplug::igraphics::IColor COLOR_ACTIVE = IColor::FromColorCode(0x488BD4);
const iplug::igraphics::IColor COLOR_ACTIVE_LIGHT = IColor::FromColorCode(0x10908E);
const iplug::igraphics::IColor COLOR_ACTIVE_DARK = IColor::FromColorCode(0x392946);
const iplug::igraphics::IColor COLOR_SEEK = IColor::FromColorCode(0x80FFFF);
const iplug::igraphics::IColor COLOR_TITLE = IColor::FromColorCode(0xff417d);

const iplug::igraphics::IText TEXT_TITLE =
  IText(36.0, COLOR_TITLE, "Montserrat-Regular", EAlign::Near, EVAlign::Bottom);
const iplug::igraphics::IText TEXT_LABEL = IText(16, COLOR_ACTIVE_LIGHT, "Roboto-Bold", EAlign::Near, EVAlign::Middle);
const iplug::igraphics::IText TEXT_CAPTION = IText(16.f, COLOR_ACTIVE, "Roboto-Bold", EAlign::Near, EVAlign::Middle);

// Styles
const IVColorSpec colorSpec{
  DEFAULT_BGCOLOR, // Background
  THEME_BLUE.WithContrast(-0.15f), // Foreground
  THEME_BLUE.WithOpacity(0.8f), // Pressed
  THEME_BLUE.WithOpacity(0.4f), // Frame
  MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  THEME_BLUE, // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  THEME_BLUE.WithContrast(0.1f), // Extra 3
};

const IVColorSpec colorPhaseOff{
  DEFAULT_BGCOLOR, // Background
  THEME_GREEN.WithContrast(0.15f), // Foreground
  THEME_GREEN.WithOpacity(0.8f), // Pressed
  THEME_GREEN.WithOpacity(0.4f), // Frame
  MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  THEME_GREEN, // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  THEME_GREEN.WithContrast(0.1f), // Extra 3
};

const IVColorSpec colorPhaseOn{
  DEFAULT_BGCOLOR, // Background
  THEME_RED.WithContrast(0.15f), // Foreground
  THEME_RED.WithOpacity(0.8f), // Pressed
  THEME_RED.WithOpacity(0.4f), // Frame
  MOUSEOVER, // Highlight
  DEFAULT_SHCOLOR, // Shadow
  THEME_RED, // Extra 1
  COLOR_RED, // Extra 2 --> color for clipping in meters
  THEME_RED.WithContrast(0.1f), // Extra 3
};

const IVColorSpec rotaryColors{
  COLOR_TRANSPARENT, COLOR_BG, COLOR_BG, COLOR_ACTIVE, COLOR_BG, COLOR_BG, COLOR_ACTIVE,
};

const IVStyle THEME_STYLE = IVStyle{true, // Show label
                                    true, // Show value
                                    colorSpec,
                                    {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Middle, THEMEFONTCOLOR}, // Knob label text5
                                    {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Bottom, THEMEFONTCOLOR}, // Knob value text
                                    DEFAULT_HIDE_CURSOR,
                                    DEFAULT_DRAW_FRAME,
                                    false,
                                    DEFAULT_EMBOSS,
                                    0.2f,
                                    2.f,
                                    DEFAULT_SHADOW_OFFSET,
                                    DEFAULT_WIDGET_FRAC,
                                    DEFAULT_WIDGET_ANGLE};

const IVStyle style = IVStyle{true, // Show label
                              true, // Show value
                              colorSpec,
                              {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Middle, THEMEFONTCOLOR}, // Knob label text5
                              {DEFAULT_TEXT_SIZE + 3.f, EVAlign::Bottom, THEMEFONTCOLOR}, // Knob value text
                              DEFAULT_HIDE_CURSOR,
                              DEFAULT_DRAW_FRAME,
                              false,
                              DEFAULT_EMBOSS,
                              0.2f,
                              2.f,
                              DEFAULT_SHADOW_OFFSET,
                              DEFAULT_WIDGET_FRAC,
                              DEFAULT_WIDGET_ANGLE};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
