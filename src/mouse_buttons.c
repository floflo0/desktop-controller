#include <assert.h>
#include <stdbool.h>

#include "mouse_buttons.h"

char *mouse_button_to_string(const MouseButton mouse_button) {
#define MOUSE_BUTTON(name, _) if (mouse_button == name) return #name;
    MOUSE_BUTTONS
#undef MOUSE_BUTTON
    assert(false && "unreachable");
    return "";
};
