#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xdo.h>

#include "config.h"
#include "controller.h"
#include "log.h"
#include "mouse_buttons.h"
#include "utils.h"

#ifndef VERSION
#define VERSION "0.0.0"
#endif

/**
 * Macro that defines the command-line flags.
 * Each flag contains:
 * - A name used as a boolean property when arguments are pared and for the
 *   long version of the flag.
 * - A short name for the flag which must be a single character.
 * - A description of the flag as a string.
 */
#define ARGS_FLAGS                                             \
    FLAG(help, h, "show this help message and exit")           \
    FLAG(version, v, "show program's version number and exit") \
    FLAG(list, l, "list all available controllers and exit")

/**
 * Macro that defines the command-line parameters.
 * Each parameters contains:
 *  - A name used as a string property when arguments.
 *  - A name in capital case used in the help of the program as a string.
 *  - A description of the parameter as a string.
 */
#define ARGS_PARAMS                 \
    PARAM(controller, "CONTROLLER", \
          "the path to the controller to use (example: /dev/input/event20)")

/**
 * Structure containing the parsed arguments with the flags stored as boolean
 * and the parameters stored as string.
 */
typedef struct {
#define FLAG(name, short_name, description) bool name;
    ARGS_FLAGS
#undef FLAG
#define PARAM(name, param_name, description) char *name;
    ARGS_PARAMS
#undef PARAM
} Args;

/**
 * Should the main loop stop and the application quit.
 */
static bool app_quit = false;

/**
 * The mouse speed multiplier.
 */
static float mouse_speed = DEFAULT_MOUSE_SPEED;

/**
 * The controller used by the app.
 */
static Controller *controller = NULL;

static xdo_t *xdo = NULL;

/**
 * Print the usage of the program.
 *
 * \param program_name The name of the program being executed to print in the
 *                     usage.
 * \param stream The output stream.
 */
static void print_usage(const char *program_name, FILE *stream) {
    fprintf(
        stream,
        "usage: %s "
#define FLAG(name, short_name, description) "[-" #short_name "] "
    ARGS_FLAGS
#undef FLAG
#define PARAM(name, param_name, description) "[" param_name "] "
    ARGS_PARAMS
#undef PARAM
        "\n",
        program_name
    );
}

#define CHAR(c) #c[0]

/**
 * Parse command-line arguments.
 *
 * \param args The structure to store parsed arguments.
 * \param argv The arguments passed to the program.
 * \param program_name The name of the program being executed.
 *
 * \return true if the arguments were successfully parsed, or false on failure.
 */
static bool args_parse(Args *args, char *argv[], const char *program_name) {
    for (; *argv; ++argv) {
        if ((*argv)[0] == '-' && (*argv)[1]) {
            if ((*argv)[1] == '-') {
#define FLAG(name, short_name, description) \
        if (streq(*argv, "--" #name)) {     \
            args->name = true;              \
            continue;                       \
        }
    ARGS_FLAGS
#undef FLAG
            } else {
                for (size_t i = 1; (*argv)[i]; ++i) {
#define FLAG(name, short_name, description) \
    if ((*argv)[i] == CHAR(short_name)) {   \
        args->name = true;                  \
        continue;                           \
    }
    ARGS_FLAGS
#undef FLAG

                    print_usage(program_name, stderr);
                    log_errorf("unrecognized arguments: '-%c'", (*argv)[i]);
                    return false;
                }
                continue;
            }
        }

        print_usage(program_name, stderr);
        log_errorf("unrecognized arguments: '%s'", *argv);
        return false;
    }

    return true;
}

/**
 * Print the help message for the program.
 *
 * \param program_name The name of the program being executed.
 */
static void print_help(const char *program_name) {
    print_usage(program_name, stdout);
    printf(
        "\n"
        "Control your desktop with a controller.\n"
        "\n"
        "positional arguments:\n"
#define PARAM(name, param_name, description) "    %-21s " description "\n"
    ARGS_PARAMS
#undef PARAM
        "\n"
        "options:\n"
#define FLAG(name, short_name, description) \
    "    -" #short_name ", --%-15s " description "\n"
    ARGS_FLAGS
#undef FLAG
#define PARAM(name, param_name, description) , param_name
    ARGS_PARAMS
#undef PARAM
#define FLAG(name, short_name, description) , #name
    ARGS_FLAGS
#undef FLAG
    );
}

/**
 * Handles the press of a buttons on the controller.
 *
 * \param button The button that was pressed.
 */
static void handle_button_down(const ControllerButton button) {
    assert(xdo && "xdo isn't initialized");

    if (button == GRAB_TOGGLE_BUTTON) {
        assert(controller && "controller isn't initialized");
        if (!controller_toggle_grabbed(controller)) {
            exit(EXIT_FAILURE);
        }
        if (!controller_rumble(controller)) {
            exit(EXIT_FAILURE);
        }
    }

    if (!controller_get_grabbed(controller)) return;

    if (button == MOUSE_SPEED_BUTTON) {
        mouse_speed = PRECISION_MOUSE_SPEED;
        log_debugf("set mouse speed to precision");
        return;
    }

#define MAP(controller_button, mouse_button)                    \
    if (button == controller_button) {                          \
        if (xdo_mouse_down(xdo, CURRENTWINDOW, mouse_button)) { \
            log_errorf("failed to set mouse button %s down",    \
                       mouse_button_to_string(mouse_button));   \
            exit(EXIT_FAILURE);                                 \
        };                                                      \
        log_debugf("mouse button %s down",                      \
                   mouse_button_to_string(mouse_button));       \
        return;                                                 \
    }

    MAP_BUTTON_TO_MOUSE

#undef MAP

#define MAP(controller_button, keys)                                         \
    if (button == controller_button) {                                       \
        if (xdo_send_keysequence_window_down(xdo, CURRENTWINDOW, keys, 0)) { \
            log_errorf("failed to set keys down: '" keys "'");               \
            exit(EXIT_FAILURE);                                              \
        }                                                                    \
        log_debugf("keys down: '" keys "'");                                 \
        return;                                                              \
    }

    MAP_BUTTON_TO_KEYS

#undef MAP
}

/**
 * Handle the release of a button on the controller.
 *
 * \param button The button that was released.
 */
static void handle_button_up(const ControllerButton button) {
    assert(xdo && "xdo isn't initialized");

    if (!controller_get_grabbed(controller)) return;

    if (button == MOUSE_SPEED_BUTTON) {
        mouse_speed = DEFAULT_MOUSE_SPEED;
        log_debugf("set mouse speed to default");
        return;
    }

#define MAP(controller_button, mouse_button)                  \
    if (button == controller_button) {                        \
        if (xdo_mouse_up(xdo, CURRENTWINDOW, mouse_button)) { \
            log_errorf("failed to set mouse button %s up",    \
                       mouse_button_to_string(mouse_button)); \
            exit(EXIT_FAILURE);                               \
        }                                                     \
        log_debugf("mouse button %s up",                      \
                   mouse_button_to_string(mouse_button));     \
        return;                                               \
    }

    MAP_BUTTON_TO_MOUSE

#undef MAP

#define MAP(controller_button, keys)                                       \
    if (button == controller_button) {                                     \
        if (xdo_send_keysequence_window_up(xdo, CURRENTWINDOW, keys, 0)) { \
            log_errorf("faield to set keys up: '" keys "'");               \
            exit(EXIT_FAILURE);                                            \
        }                                                                  \
        log_debugf("keys down: '" keys "'");                               \
        return;                                                            \
    }

    MAP_BUTTON_TO_KEYS

#undef MAP
}

/**
 * Determine the scroll speed based on the stick input value.
 *
 * \param v The input value from the controller's stick between -1.0 and 1.0.
 * \return The scroll speed corresponding to the input value.
 */
static float get_scroll_speed(const float v) {
    return SCROLL_MIN_SPEED * (fabs(v) - 1.0f) * (fabs(v) - 1.0f) +
        SCROLL_MAX_SPEED;
}

/**
 * Handle the SIGINT signal (Ctrl+C) and set the application to quit.
 */
static void hanlde_sigint(const int _) {
    (void)_;
    app_quit = true;
    log_debugf("quiting...");
}

int main(const int argc, char *argv[]) {
    (void)argc;

    log_debugf("test");

    assert(*argv && "no program name");
    const char *program_name = basename(*argv++);
    if (!log_init(program_name)) return EXIT_FAILURE;

    Args args = {0};
    if (!args_parse(&args, argv, program_name)) {
        return false;
    }

    if (args.help) {
        print_help(program_name);
        return EXIT_SUCCESS;
    }

    if (args.version) {
        printf("%s " VERSION "\n", program_name);
        return EXIT_SUCCESS;
    }

    if (args.list) {
        if (!controller_list()) return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }

    if (args.controller) {
        controller = controller_from_device_path(args.controller);
    } else {
        controller = controller_from_first();
    }
    if (!controller) return EXIT_FAILURE;

    xdo = xdo_new(NULL);
    if (!xdo) {
        log_errorf("failed to initialize xdo");
        return EXIT_FAILURE;
    }
    log_debugf("libxdo %s", xdo_version());

    uint64_t loop_start = get_time_ms();
    uint64_t delta_time = 0;

    uint64_t last_scroll_x = 0;
    uint64_t last_scroll_y = 0;

    float mouse_movement_x = 0.0f;
    float mouse_movement_y = 0.0f;

    if (signal(SIGINT, hanlde_sigint) == SIG_ERR) {
        log_errorf("failed to setup SIGINT handler: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    log_debugf("app ready");
    while (!app_quit) {
        if (!controller_update(controller, handle_button_down,
                               handle_button_up)) {
            return EXIT_FAILURE;
        }

        if (controller_get_grabbed(controller)) {
            float lx, ly;
            controller_get_stick(controller, CONTROLLER_STICK_LEFT, &lx, &ly);
            mouse_movement_x += lx * mouse_speed * delta_time;
            mouse_movement_y += ly * mouse_speed * delta_time;
            if (fabsf(mouse_movement_x) >= 1.0f ||
                fabsf(mouse_movement_y) >= 1.0f) {
                const int mouse_movement_x_int = (int)mouse_movement_x;
                const int mouse_movement_y_int = (int)mouse_movement_y;
                mouse_movement_x -= mouse_movement_x_int;
                mouse_movement_y -= mouse_movement_y_int;
                if (xdo_move_mouse_relative(xdo, mouse_movement_x_int,
                                            mouse_movement_y_int)) {
                    log_errorf("failed to move mouse");
                }
                log_debugf("move mouse: dx=%d dy=%d", mouse_movement_x_int,
                           mouse_movement_y_int);
            }

            float rx, ry;
            controller_get_stick(controller, CONTROLLER_STICK_RIGHT, &rx, &ry);
            if (get_time_ms() - last_scroll_y > get_scroll_speed(ry)) {
                if (ry < 0.0f) {
                    if (xdo_click_window(xdo, CURRENTWINDOW, MOUSE_WHEEL_UP)) {
                        log_errorf("failed to scroll up");
                        return EXIT_FAILURE;
                    }
                    log_debugf("scroll up");
                    last_scroll_y = get_time_ms();
                } else if (ry > 0.0f) {
                    if (xdo_click_window(xdo, CURRENTWINDOW, MOUSE_WHEEL_DOWN))
                    {
                        log_errorf("failed to scroll down");
                        return EXIT_FAILURE;
                    }
                    log_debugf("scroll down");
                    last_scroll_y = get_time_ms();
                }
            }
            if (get_time_ms() - last_scroll_x > get_scroll_speed(rx)) {
                if (rx < 0.0f) {
                    if (xdo_click_window(xdo, CURRENTWINDOW, MOUSE_WHEEL_LEFT))
                    {
                        log_errorf("failed to scroll left");
                        return EXIT_FAILURE;
                    }
                    log_debugf("scroll left");
                    last_scroll_x = get_time_ms();
                } else if (rx > 0.0f) {
                    if (xdo_click_window(xdo, CURRENTWINDOW, MOUSE_WHEEL_RIGHT))
                    {
                        log_errorf("failed to scroll right");
                        return EXIT_FAILURE;
                    }
                    log_debugf("scroll right");
                    last_scroll_x = get_time_ms();
                }
            }
        }

        const uint64_t loop_end = get_time_ms();
        delta_time = loop_end - loop_start;
        loop_start = loop_end;
    }

    xdo_free(xdo);
    controller_destroy(controller);

    log_debugf("quit");

    log_quit();

    return EXIT_SUCCESS;
}
