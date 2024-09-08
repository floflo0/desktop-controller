#pragma once

/**
 * Config file of the application.
 */

#include "controller.h"
#include "mouse_buttons.h"

/**
 * Default mouse speed multiplier.
 */
#define DEFAULT_MOUSE_SPEED 1.7f

/**
 * Mouse speed multiplier used to slow down the mouse in presicion mode.
 */
#define PRECISION_MOUSE_SPEED 0.3f

/**
 * Delay in miliseconds between two mouse wheel presses to control the minimum
 * scroll speed.
 */
#define SCROLL_MIN_SPEED 500  // ms

/**
 * Delay in miliseconds between two mouse wheel presses to control the maximum
 * scroll speed.
 */
#define SCROLL_MAX_SPEED 30  // ms

/**
 * A ControllerButton used to toggle the controller state between grabbed and
 * not grabbed. When grabbed only the application can receive event from the
 * controller.
 */
#define GRAB_TOGGLE_BUTTON CONTROLLER_BUTTON_HOME

/**
 * A ControllerButton used to slow down the mouse speed while pressed using
 * PRECISION_MOUSE_SPEED.
 */
#define MOUSE_SPEED_BUTTON CONTROLLER_BUTTON_L

/**
 * Map buttons of the controller to a mouse button.
 *
 * The first parameter is a ControllerButton.
 * The second parameter is a MouseButton.
 */
#define MAP_BUTTON_TO_MOUSE                   \
    MAP(CONTROLLER_BUTTON_A, MOUSE_LEFT)      \
    MAP(CONTROLLER_BUTTON_RPAD, MOUSE_MIDDLE) \
    MAP(CONTROLLER_BUTTON_Y, MOUSE_RIGHT)

/**
 * Map buttons of the controller to a keyboard key.
 *
 * The first parameter is a ControllerButton.
 * The second parameter is a string that contain an key name or a keyboard
 * shortcut. The syntax of the keyboard shortcut is the same as xdotool.
 */
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
