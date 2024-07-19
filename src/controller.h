#pragma  once

#include <stddef.h>
#include <stdbool.h>

typedef struct _Controller Controller;

typedef enum {
    CONTROLLER_BUTTON_B     = 0,
    CONTROLLER_BUTTON_A     = 1,
    CONTROLLER_BUTTON_Y     = 2,
    CONTROLLER_BUTTON_X     = 3,
    CONTROLLER_BUTTON_L     = 4,
    CONTROLLER_BUTTON_R     = 5,
    CONTROLLER_BUTTON_MINUS = 6,
    CONTROLLER_BUTTON_PLUS  = 7 ,
    CONTROLLER_BUTTON_HOME  = 8 ,
    CONTROLLER_BUTTON_LPAD  = 9 ,
    CONTROLLER_BUTTON_RPAD  = 10,
    CONTROLLER_BUTTON_UP    = 11,
    CONTROLLER_BUTTON_DOWN  = 12,
    CONTROLLER_BUTTON_LEFT  = 13,
    CONTROLLER_BUTTON_RIGHT = 14,
    CONTROLLER_BUTTON_ZL    = 15,
    CONTROLLER_BUTTON_ZR    = 16,
} ControllerButton;

typedef enum {
    CONTROLLER_STICK_LEFT,
    CONTROLLER_STICK_RIGHT
} ControllerStick;

typedef void (*ControllerButtonEventCallBack)(const ControllerButton);

Controller *controller_from_device_path(const char *device_path);
Controller *controller_from_first(void);
void controller_destroy(Controller *controller);
bool controller_list(void);
bool controller_update(Controller *controller,
                       const ControllerButtonEventCallBack on_button_down,
                       const ControllerButtonEventCallBack on_button_up);
void controller_get_stick(const Controller *controller,
                          const ControllerStick stick, float *x, float *y);
bool controller_rumble(Controller *controller);
bool controller_get_grabbed(const Controller *controller);
bool controller_toggle_grabbed(Controller *controller);
