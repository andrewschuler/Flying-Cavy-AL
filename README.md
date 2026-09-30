# Flying-Cavy-AL
Flying Cavy Altitude Limiter for F5L

This project is build using an Adafruit KB2040 microcontroller and a
BMP580 barometric pressure sensor.

# Hardware

To start with you need the micro controller and pressure sensor. Here
is what I used:

<https://www.adafruit.com/product/5302> (rp 2040 based microcontroller)
<https://www.adafruit.com/product/6411> (bmp580 pressure sensor)
<https://www.adafruit.com/product/4399> (cable to connect the two)

You will also need a servo extension cable some non conductive tape to
protect the boards (such as kapton tape).  Tools needed are some way
to cut and strip wires and a soldering iron and solder.  The board
needs a usb c cable for programming.  Please look for online for the
instructions to set up the arduino environment for building the code.

First connect the pressure sensor to the microcontroller with the cable.

Cut the extension cable in half and pull apart the 3 wires on each
end. Strip off about 3-4mm of the insulation from each wire. The servo
power needs to be connected to the RAW pin, the ground to any of the
ground pins, The rx side signal is connected to pin 2, the esc side to
pin 4.

Connect the board to your pc with the usb cable and upload the program
to it.

You want to put tape on the backs of each board before placing them
together.  When taping them together make sure the actual pressure
sensor at the center of the breakout board is exposed


# Usage

Plug you Flying Cavy Altitude Limiter to the throttle connections on
your receiver and to the ESC.  When you power up the plane the LED
near the buttons will blink.  The LED blinks blue until the throttle
has been held low for about 1 second; the motor cannot be started
until the LED changes from blue to green.  Pressing the button furthest from the
blinking LED will change the mode. The mode will be indicated by 1, 2,
or 3 blinks.  The default altitudes and times are 80m/15s, 100m/30s,
150m/30s (these will probably change). The current relight logic is
that you can restart below 10m altitude and after and 30 or 60 seconds
(depending on the mode).  After a restart the led will be an
unblinking red.  After a flight without a restart the led will have
one long flash if the motor cut off because of altitude and two long
flashed if it cut off because of the time limit.  There will be 1-3
short flashes that indicate the mode. After flight use the reset
button to reset the board.  You will also need to make sure you are in
the right mode as it doesn't save that setting.

