#pragma once

#include "controller.h"
#include "mouse_buttons.h"

#define DEFAULT_MOUSE_SPEED 1.7f
#define PRECISION_MOUSE_SPEED 0.3f
#define SCROLL_MIN_SPEED 500  // ms
#define SCROLL_MAX_SPEED 30  // ms

#define GRAB_TOGGLE_BUTTON CONTROLLER_BUTTON_HOME
#define MOUSE_SPEED_BUTTON CONTROLLER_BUTTON_L

#define MAP_BUTTON_TO_MOUSE                   \
    MAP(CONTROLLER_BUTTON_A, MOUSE_LEFT)      \
    MAP(CONTROLLER_BUTTON_RPAD, MOUSE_MIDDLE) \
    MAP(CONTROLLER_BUTTON_Y, MOUSE_RIGHT)

#define MAP_BUTTON_TO_KEYS                               \
    MAP(CONTROLLER_BUTTON_B, "Escape")                   \
    MAP(CONTROLLER_BUTTON_X, "XF86AudioPlay")            \
    MAP(CONTROLLER_BUTTON_R, "Super+q")                  \
    MAP(CONTROLLER_BUTTON_LEFT, "Super+Control+h")       \
    MAP(CONTROLLER_BUTTON_RIGHT, "Super+Control+l")      \
    MAP(CONTROLLER_BUTTON_UP, "Super+f")                 \
    MAP(CONTROLLER_BUTTON_PLUS, "XF86AudioRaiseVolume")  \
    MAP(CONTROLLER_BUTTON_MINUS, "XF86AudioLowerVolume") \
    MAP(CONTROLLER_BUTTON_ZL, "Shift")                   \
    MAP(CONTROLLER_BUTTON_ZR, "Control")                 \
    MAP(CONTROLLER_BUTTON_LPAD, "Super+d")
