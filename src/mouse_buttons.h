#pragma once

#define MOUSE_BUTTONS                 \
    MOUSE_BUTTON(MOUSE_LEFT, 1)       \
    MOUSE_BUTTON(MOUSE_MIDDLE, 2)     \
    MOUSE_BUTTON(MOUSE_RIGHT, 3)      \
    MOUSE_BUTTON(MOUSE_WHEEL_UP, 4)   \
    MOUSE_BUTTON(MOUSE_WHEEL_DOWN, 5) \
    MOUSE_BUTTON(MOUSE_WHEEL_LEFT, 6) \
    MOUSE_BUTTON(MOUSE_WHEEL_RIGHT, 7)

/**
 * Enum representing the buttons available on a mouse.
 */
typedef enum {
#define MOUSE_BUTTON(name, value) name = value,
    MOUSE_BUTTONS
#undef MOUSE_BUTTON
} MouseButton;

/**
 * Converts a MouseButton enum value to its corresponding string representation.
 *
 * \param mouse_button The MouseButton enum value to convert to a string.
 *
 * \returns the name of the mouse button as a string.
 */
char *mouse_button_to_string(const MouseButton mouse_button);
