# Desktop-Controller

Control your desktop with a controller.

## The way it work

This program allows you to control the mouse using the left pad of your
controller and scroll with the right pad. Once connected, it exclusively
receives events from the controller, preventing other applications from doing
so. You can easily release the controller to stop controlling the mouse and
switch to gaming with a simple button press.

## Default button mappings

- `HOME`: grab/ungrab the controller
- `L`: slow down mouse speed
- `A`: left click
- `RPAD`: middle click
- `Y`: right click
- `B`: `Escape`
- `X`: `XF86AudioPlay`
- `R`: `Super+q`
- `LEFT`: `Super+Control+h`
- `RIGHT`: `Super+Control+l`
- `UP`: `Super+f`
- `PLUS`: `XF86AudioRaiseVolume`
- `MINUS`: `XF86AudioLowerVolume`
- `ZL`: `Shift`
- `ZR`: `Control`
- `LPAD`: `Super+d`

You can change these mappings in [config.h](src/config.h).
You may also need to change the buttons code for your controller in
[controller.c](src/controller.c).

## Usage

```
usage: desktop-controller [-h] [-v] [-l] [CONTROLLER]

Control your desktop with a controller.

positional arguments:
    CONTROLLER            the path to the controller to use (example: /dev/input/event20)

options:
    -h, --help            show this help message and exit
    -v, --version         show program's version number and exit
    -l, --list            list all available controllers and exit
```

## Build

```sh
make
```

Create an optimized production build by running the following command:
```sh
make PROD=1
```

**⚠️ Warning: if you build and then you build with `PROD=1`, you must force make
to rebuild everything by adding `-B`.**
