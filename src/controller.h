#pragma  once

/**
 * Simple interface for handling controllers.
 */

#include <stddef.h>
#include <stdbool.h>

/**
 * Represents a controller device.
 */
typedef struct _Controller Controller;

/**
 * Enum representing the buttons available on a controller.
 */
typedef enum {
    // Real buttons
    CONTROLLER_BUTTON_B     = 0,
    CONTROLLER_BUTTON_A     = 1,
    CONTROLLER_BUTTON_Y     = 2,
    CONTROLLER_BUTTON_X     = 3,
    CONTROLLER_BUTTON_L     = 4,
    CONTROLLER_BUTTON_R     = 5,
    CONTROLLER_BUTTON_MINUS = 6,
    CONTROLLER_BUTTON_PLUS  = 7,
    CONTROLLER_BUTTON_HOME  = 8,
    CONTROLLER_BUTTON_LPAD  = 9,
    CONTROLLER_BUTTON_RPAD  = 10,

    // Hat buttons
    CONTROLLER_BUTTON_UP    = 11,
    CONTROLLER_BUTTON_DOWN  = 12,
    CONTROLLER_BUTTON_LEFT  = 13,
    CONTROLLER_BUTTON_RIGHT = 14,

    // Trigger buttns
    CONTROLLER_BUTTON_ZL    = 15,
    CONTROLLER_BUTTON_ZR    = 16,
} ControllerButton;

/**
 * Enum representing the sticks available on a controller.
 */
typedef enum {
    CONTROLLER_STICK_LEFT,
    CONTROLLER_STICK_RIGHT
} ControllerStick;

/**
 * Callback function called when the state of a button changes.
 */
typedef void (*ControllerButtonEventCallBack)(const ControllerButton);

/**
 * Initialize a controller from a device path. The controller need to closed
 * with controller_destroy().
 *
 * \param device_path The path to the device path (example: /dev/input/event20).
 *
 * \returns a pointer to the controller or NULL on failure.
 */
Controller *controller_from_device_path(const char *device_path);

/**
 * Initialize a controller from the first device that match the requirement. The
 * controller need to closed with controller_destroy().
 *
 * \returns a pointer to the controller or NULL if no controller is found or on
 *          failure.
 */
Controller *controller_from_first(void);

/**
 * Destroy a controller created by controller_from_device_path() and
 * controller_from_first().
 *
 * \param controller The pointer of the controller object to destroy.
 */
void controller_destroy(Controller *controller);

/**
 * List all the available controller to stdout.
 *
 * \returns true on success, false on failure.
 */
bool controller_list(void);

/**
 * Update the state of a controller and triggers the appropriate callback when
 * the state of any buttons changes.
 *
 * \param controller The pointer to the controller object to update.
 * \param on_button_down A callback function that is invoked when a button
 *                       is pressed down.
 * \param on_button_up A callback function that is invoked when a button
 *                     is released.
 *
 * \returns true on success, false on failure.
 */
bool controller_update(Controller *controller,
                       const ControllerButtonEventCallBack on_button_down,
                       const ControllerButtonEventCallBack on_button_up);

/**
 * Retrieves the current position of the specified analog stick (left or right)
 * on the controller.
 *
 * The function normalizes the raw axis values of the stick to a floating-point
 * range between -1.0 and 1.0, where -1.0 represents the minimum position, 1.0
 * represents the maximum position, and 0.0 represents the center.
 *
 * \param controller A pointer to the controller object from which to retrieve
 *                   the stick position.
 * \param stick The stick to retrieve the position for. This should be either
 *              CONTROLLER_STICK_LEFT or CONTROLLER_STICK_RIGHT.
 * \param x A pointer to a float where the normalized X-axis value will be
 *          stored.
 * \param y A pointer to a float where the normalized Y-axis value will be
 *          stored.
 */
void controller_get_stick(const Controller *controller,
                          const ControllerStick stick, float *x, float *y);

/**
 * Start a rumble effect on the controller.
 *
 * \param controller A pointer to the controller object on which to activate the
 *                   rumble effect.
 *
 * \returns true on success, or false if there is an error starting the rumble.
 */
bool controller_rumble(Controller *controller);

/**
 * Checks if the controller is currently grabbed.
 *
 * \param controller A pointer to the controller object.
 *
 * \returns true if the controller is grabbed, false otherwise.
 */
bool controller_get_grabbed(const Controller *controller);

/**
 * Toggles the grabbed state of the controller.
 *
 * This function either grabs or ungrabs the controller device based on its
 * current state.
 *
 * When a controller is grabbed, it prevent other program to receiving events
 * from this controller.
 *
 * \param controller A pointer to the controller object.
 *
 * \returns true on success, false on failure.
 */
bool controller_toggle_grabbed(Controller *controller);
