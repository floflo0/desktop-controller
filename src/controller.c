#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libevdev/libevdev.h>

#include "controller.h"
#include "log.h"
#include "utils.h"

#define CONTROLLER_AXIS_MAX 32767
#define CONTROLLER_AXIS_MIN -32768
#define CONTROLLER_AXIS_ROUND 0.01f
#define CONTROLLER_RUMBLE_DURATION 500  // ms

#define DEVICE_PATH_SIZE 27

#define CONTROLLER_BUTTONS                      \
    BUTTON(CONTROLLER_BUTTON_A, BTN_EAST)       \
    BUTTON(CONTROLLER_BUTTON_B, BTN_SOUTH)      \
    BUTTON(CONTROLLER_BUTTON_Y, BTN_NORTH)      \
    BUTTON(CONTROLLER_BUTTON_X, BTN_WEST)       \
    BUTTON(CONTROLLER_BUTTON_L, BTN_TL)         \
    BUTTON(CONTROLLER_BUTTON_R, BTN_TR)         \
    BUTTON(CONTROLLER_BUTTON_MINUS, BTN_SELECT) \
    BUTTON(CONTROLLER_BUTTON_PLUS, BTN_START)   \
    BUTTON(CONTROLLER_BUTTON_HOME, BTN_MODE)    \
    BUTTON(CONTROLLER_BUTTON_LPAD, BTN_THUMBL)  \
    BUTTON(CONTROLLER_BUTTON_RPAD, BTN_THUMBR)

struct _Controller {
    int fd;
    struct libevdev *dev;
    float hat_state[4];
    bool grabbed;
    bool is_rumbling;
    int16_t rumble_effect_id;
    uint64_t rumble_start;
};

static void controller_log_func(const struct libevdev *dev,
                                enum libevdev_log_priority priority,
                                void *data, const char *file,
                                int line, const char *func,
                                const char *format, va_list args) {
    (void)dev;
    (void)data;
#ifdef PROD
    (void)file;
    (void)line;
#endif

    if (priority == LIBEVDEV_LOG_ERROR) {
#ifndef PROD
        fprintf(stderr, "%s:%d: ", file, line);
#endif
        fprintf(stderr, "%s: error: ", func),
        vfprintf(stderr, format, args);
        return;
    }

#ifndef PROD
    if (priority == LIBEVDEV_LOG_DEBUG) {
        printf("%s:%d: %s: debug: ", file, line, func);
        vprintf(format, args);
        return;
    }
#endif

#ifndef PROD
    printf("%s:%d: ", file, line);
#endif
    printf("%s: info: ", func);
    vprintf(format, args);
    return;
}

static bool controller_init_libevdev(const int fd, struct libevdev **dev) {
    *dev = libevdev_new();
    libevdev_set_device_log_function(
        *dev,
        controller_log_func,
#ifndef PROD
        LIBEVDEV_LOG_DEBUG,
#else
        LIBEVDEV_LOG_INFO,
#endif
        NULL
    );

    const int err = libevdev_set_fd(*dev, fd);
    if (err < 0) {
        log_errorf("failed to initialize libevdev device: %s", strerror(-err));
        return false;
    }

    return true;
}

static bool is_controller(struct libevdev *dev) {
    return (
        libevdev_has_event_type(dev, EV_ABS) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_X) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_Y) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_Z) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_RX) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_RY) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_RZ) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_HAT0X) &&
        libevdev_has_event_code(dev, EV_ABS, ABS_HAT0Y) &&
        libevdev_has_event_type(dev, EV_FF) &&
        libevdev_has_event_code(dev, EV_FF, FF_RUMBLE) &&
        libevdev_has_event_type(dev, EV_KEY)
#define BUTTON(controller_button, button_code) \
    && libevdev_has_event_code(dev, EV_KEY, button_code)
    CONTROLLER_BUTTONS
#undef BUTTON
    );
}

#ifndef PROD
#define controller_dump_info(controller) _controller_dump_info(controller)
static void _controller_dump_info(const Controller *controller) {
    log_debugf("controller name: '%s'", libevdev_get_name(controller->dev));
    const char *phys = libevdev_get_phys(controller->dev);
    if (phys) log_debugf("controller physical location: '%s'", phys);
    log_debugf("controller vendor: %d",
               libevdev_get_id_vendor(controller->dev));
    log_debugf("controller product: %d",
               libevdev_get_id_product(controller->dev));
}
#else
#define controller_dump_info(controller)
#endif

static Controller *controller_from_fd_and_dev(const int fd,
                                              struct libevdev *dev) {

    Controller *controller = malloc(sizeof(*controller));
    if (!controller) {
        log_errorf("failed to allocate memory: %s", strerror(errno));
        return NULL;
    }

    controller->fd = fd;
    controller->dev = dev;

    struct ff_effect effect;
    memset(&effect, 0, sizeof(effect));
    effect.type = FF_RUMBLE;
    effect.id = -1;
    effect.replay.length = CONTROLLER_RUMBLE_DURATION;
    effect.u.rumble.strong_magnitude = 0xffff;
    effect.u.rumble.weak_magnitude = 0xffff;
    if (ioctl(controller->fd, EVIOCSFF, &effect) < 0) {
        log_errorf("failed to upload rumble effect: %s", strerror(errno));
        controller_destroy(controller);
        return NULL;
    }
    controller->is_rumbling = false;
    controller->rumble_effect_id = effect.id;
    controller->rumble_start = 0;
    log_debugf("uploaded rumble effect");

    const int err = libevdev_grab(controller->dev, LIBEVDEV_GRAB);
    if (err < 0) {
        log_errorf("failed to grab controller: %s", strerror(-err));
        controller_destroy(controller);
        return NULL;
    }
    controller->grabbed = true;
    log_debugf("grab controller");

    controller_dump_info(controller);

    if (!controller_rumble(controller)) {
        controller_destroy(controller);
        return NULL;
    }

    memset(controller->hat_state, false, sizeof(controller->hat_state));

    return controller;
}

Controller *controller_from_device_path(const char *device_path) {
    const int fd = open(device_path, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        log_errorf("failed to open %s: %s", device_path, strerror(errno));
        return NULL;
    }

    struct libevdev *dev;
    if (!controller_init_libevdev(fd, &dev)) {
        close(fd);
        return NULL;
    }

    if (!is_controller(dev)) {
        log_errorf("failed to connect to device %s: not a controller",
                   device_path);
        libevdev_free(dev);
        close(fd);
        return NULL;
    }

    log_debugf("connect to controller %s", device_path);

    return controller_from_fd_and_dev(fd, dev);
}

Controller *controller_from_first(void) {
    char *device_path = malloc(DEVICE_PATH_SIZE);
    if (!device_path) {
        log_errorf("failed to allocate memory: %s", strerror(errno));
        return NULL;
    }
    Controller *controller = NULL;
    for (uint32_t i = 0;; ++i) {
        snprintf(device_path, DEVICE_PATH_SIZE, "/dev/input/event%d", i);
        const int fd = open(device_path, O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            if (errno == ENOENT) {  // no more device
                log_errorf("no controller found");
                break;
            }
            if (errno == EACCES) continue;  // we can't access this device

            log_errorf("failed to open %s: %s", device_path,
                       strerror(errno));
            break;
        }
        struct libevdev *dev;
        if (!controller_init_libevdev(fd, &dev)) {
            close(fd);
            break;
        }

        if (is_controller(dev)) {
            log_debugf("connect to controller %s", device_path);
            controller = controller_from_fd_and_dev(fd, dev);
            break;
        }

        libevdev_free(dev);
        close(fd);
    }
    free(device_path);
    return controller;
}

void controller_destroy(Controller *controller) {
    libevdev_free(controller->dev);
    close(controller->fd);
    free(controller);
}

bool controller_list(void) {
    for (uint32_t i = 0;; ++i) {
        char device_path[DEVICE_PATH_SIZE];
        snprintf(device_path, DEVICE_PATH_SIZE, "/dev/input/event%d", i);
        const int fd = open(device_path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            if (errno == ENOENT) return true;  // no more device
            if (errno == EACCES) continue;  // we can't access this device

            log_errorf("failed to open %s: %s", device_path,
                       strerror(errno));
            return false;
        }
        struct libevdev *dev;
        if (!controller_init_libevdev(fd, &dev)) {
            close(fd);
            return false;
        }
        if (is_controller(dev)) {
            printf("%s: '%s'\n", device_path, libevdev_get_name(dev));
        }
        libevdev_free(dev);
        close(fd);
    }
}

#ifndef PROD
#define log_event(event)                                                  \
    const char *type_name = libevdev_event_type_get_name(event.type);     \
    if (!type_name) {                                                     \
        log_errorf("failed to get event type name: type=%d", event.type); \
        return false;                                                     \
    }                                                                     \
    const char *code_name = libevdev_event_code_get_name(event.type,      \
                                                         event.code);     \
    if (!code_name) {                                                     \
        log_errorf("failed to get event code name: type=%s code=%d",      \
                   type_name, event.code);                                \
        return false;                                                     \
    }                                                                     \
    log_debugf("event: type=%s code=%s value=%d", type_name, code_name,   \
               event.value);
#else
#define log_event(event)
#endif

#define HAT_INDEX(hat_button) ((hat_button) - CONTROLLER_BUTTON_UP)

static void controller_handle_hat_event(
    Controller *controller,
    const struct input_event *event,
    const ControllerButton button_positive,
    const ControllerButton button_negative,
    const ControllerButtonEventCallBack on_button_down,
    const ControllerButtonEventCallBack on_button_up
) {
    if (event->value == 1) {
        if (controller->hat_state[HAT_INDEX(button_negative)]) {
            controller->hat_state[HAT_INDEX(button_negative)] = false;
            on_button_up(button_negative);
        }
        if (!controller->hat_state[HAT_INDEX(button_positive)]) {
            controller->hat_state[HAT_INDEX(button_positive)] = true;
            on_button_down(button_positive);
        }
    } else if (event->value == 0) {
        if (controller->hat_state[HAT_INDEX(button_negative)]) {
            controller->hat_state[HAT_INDEX(button_negative)] = false;
            on_button_up(button_negative);
        }
        if (controller->hat_state[HAT_INDEX(button_positive)]) {
            controller->hat_state[HAT_INDEX(button_positive)] = false;
            on_button_up(button_positive);
        }
    } else if (event->value == -1) {
        if (!controller->hat_state[HAT_INDEX(button_negative)]) {
            controller->hat_state[HAT_INDEX(button_negative)] = true;
            on_button_down(button_negative);
        }
        if (controller->hat_state[HAT_INDEX(button_positive)]) {
            controller->hat_state[HAT_INDEX(button_positive)] = false;
            on_button_up(button_positive);
        }
    }
}

static void controller_handle_event(
    Controller *controller,
    const struct input_event *event,
    const ControllerButtonEventCallBack on_button_down,
    const ControllerButtonEventCallBack on_button_up
) {
    if (event->type == EV_KEY) {
#define BUTTON(controller_button, button_code) \
    if (event->code == button_code) {          \
        if (event->value) {                    \
            on_button_down(controller_button); \
        } else {                               \
            on_button_up(controller_button);   \
        }                                      \
        return;                                \
    }

    CONTROLLER_BUTTONS

#undef BUTTON
    } else if (event->type == EV_ABS) {
        if (event->code == ABS_Z) {
            if (!event->value) {
                on_button_up(CONTROLLER_BUTTON_ZL);
            } else {
                on_button_down(CONTROLLER_BUTTON_ZL);
            }
            return;
        }
        if (event->code == ABS_RZ) {
            if (!event->value) {
                on_button_up(CONTROLLER_BUTTON_ZR);
            } else {
                on_button_down(CONTROLLER_BUTTON_ZR);
            }
            return;
        }

        if (event->code == ABS_HAT0Y) {
            controller_handle_hat_event(
                controller,
                event,
                CONTROLLER_BUTTON_DOWN,
                CONTROLLER_BUTTON_UP,
                on_button_down,
                on_button_up
            );
            return;
        }
        if (event->code == ABS_HAT0X) {
            controller_handle_hat_event(
                controller,
                event,
                CONTROLLER_BUTTON_RIGHT,
                CONTROLLER_BUTTON_LEFT,
                on_button_down,
                on_button_up
            );
            return;
        }
    }
}

static bool controller_handle_syn_dropped(
    Controller *controller,
    const ControllerButtonEventCallBack on_button_down,
    const ControllerButtonEventCallBack on_button_up
) {
    struct input_event event;
    int return_code = LIBEVDEV_READ_STATUS_SYNC;
    while (return_code == LIBEVDEV_READ_STATUS_SYNC) {
        return_code = libevdev_next_event(controller->dev,
                                          LIBEVDEV_READ_FLAG_SYNC,
                                          &event);
        if (return_code < 0) {
            if (return_code != -EAGAIN) {
                log_errorf("failed to get next event: %s",
                           strerror(-return_code));
                return false;
            }
        }

        log_event(event)
        controller_handle_event(controller, &event, on_button_down,
                                on_button_up);
    }

    return true;
}

static bool controller_stop_rumble(Controller *controller) {
    struct input_event stop;
    stop.type = EV_FF;
    stop.code = controller->rumble_effect_id;
    stop.value = 0;

    if (write(controller->fd, &stop, sizeof(stop)) < 0) {
        log_errorf("failed to send rumble stop event: %s", strerror(errno));
        return false;
    }

    controller->is_rumbling = false;

    return true;
}

bool controller_update(Controller *controller,
                       const ControllerButtonEventCallBack on_button_down,
                       const ControllerButtonEventCallBack on_button_up) {
    if (controller->is_rumbling &&
        get_time_ms() - controller->rumble_start > CONTROLLER_RUMBLE_DURATION) {
        if (!controller_stop_rumble(controller)) return false;
    }

    struct input_event event;
    int return_code = LIBEVDEV_READ_STATUS_SUCCESS;

    while (return_code != -EAGAIN) {
        return_code = libevdev_next_event(controller->dev,
                                          LIBEVDEV_READ_FLAG_NORMAL,
                                          &event);
        if (return_code < 0) {
            if (return_code != -EAGAIN) {
                log_errorf("failed to get next event: %s",
                           strerror(-return_code));
                return false;
            }
        } else if (return_code == LIBEVDEV_READ_STATUS_SYNC) {
            log_event(event)
            if (!controller_handle_syn_dropped(
                controller,
                on_button_down,
                on_button_up
            )) return false;
        } else if (return_code == LIBEVDEV_READ_STATUS_SUCCESS) {
            log_event(event)
            controller_handle_event(controller, &event, on_button_down,
                                    on_button_up);
        }
    }

    return true;
}

void controller_get_stick(const Controller *controller,
                          const ControllerStick stick, float *x, float *y) {
    int x_axis, y_axis;
    if (stick == CONTROLLER_STICK_LEFT) {
        x_axis = ABS_X;
        y_axis = ABS_Y;
    } else if (stick == CONTROLLER_STICK_RIGHT) {
        x_axis = ABS_RX;
        y_axis = ABS_RY;
    } else {
        assert(false && "unreachable");
        return;
    }

    const int x_value = libevdev_get_event_value(controller->dev, EV_ABS,
                                                 x_axis);
    const int y_value = libevdev_get_event_value(controller->dev, EV_ABS,
                                                     y_axis);

    *x = 2.0f * (float)(x_value + CONTROLLER_AXIS_MAX) /
        (CONTROLLER_AXIS_MAX - CONTROLLER_AXIS_MIN) - 1.0f;
    *y = 2.0f * (float)(y_value + CONTROLLER_AXIS_MAX) /
        (CONTROLLER_AXIS_MAX - CONTROLLER_AXIS_MIN) - 1.0f;

    if (fabsf(*x) < CONTROLLER_AXIS_ROUND) {
        *x = 0.0f;
    } else if (*x > 1.0f - CONTROLLER_AXIS_ROUND) {
        *x = 1.0f;
    } else if (*x < -1.0f + CONTROLLER_AXIS_ROUND) {
        *x = -1.0f;
    }
    if (fabsf(*y) < CONTROLLER_AXIS_ROUND) {
        *y = 0.0f;
    } else if (*y > 1.0f - CONTROLLER_AXIS_ROUND) {
        *y = 1.0f;
    } else if (*y < -1.0f + CONTROLLER_AXIS_ROUND) {
        *y = -1.0f;
    }
}

bool controller_rumble(Controller *controller) {
    log_debugf("controller rumble");

    if (controller->is_rumbling) {
        if (!controller_stop_rumble(controller)) return false;
    }

    struct input_event play;
    play.type = EV_FF;
    play.code = controller->rumble_effect_id;
    play.value = 1;

    if (write(controller->fd, &play, sizeof(play)) < 0) {
        log_errorf("failed to send rumble play event: %s", strerror(errno));
        return false;
    }

    controller->is_rumbling = true;
    controller->rumble_start = get_time_ms();

    return true;
}

bool controller_get_grabbed(const Controller *controller) {
    return controller->grabbed;
}

bool controller_toggle_grabbed(Controller *controller) {
    const int err = libevdev_grab(
        controller->dev,
        controller->grabbed ? LIBEVDEV_UNGRAB : LIBEVDEV_GRAB
    );
    if (err < 0) {
        log_errorf("failed to grab/ungrab controller: %s", strerror(-err));
        return false;
    }

    controller->grabbed = !controller->grabbed;

#ifndef PROD
    if (controller->grabbed) {
        log_debugf("grabbed controller");
    } else {
        log_debugf("ungrabbed controller");
    }
#endif

    return true;
}
