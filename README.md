Forest
======

An interactive light art installation.

2015, Micah Elizabeth Scott

Portfolio link: [misc.name/forest](http://www.misc.name/forest)


Running
-------

The installation is designed to run from a Mac Mini. Specifically, it's been developed and tested on model `Macmini6,2` with a quad core 2.3 GHz Intel Core i7 and Intel HD Graphics 4000.

Systems with different specs may influence the density of the particle system, as the software tunes itself to maintain an approximately fixed frame rate by adjusting the particle release rate.

* Copy the `bin` directory from here to the installation machine
* Set `fcserver.command` and `CircleEngine.app` to run at logon

Testing Lights
--------------

While running CircleEngine:

* (optional) Press `tab` to bring up the parameter UI
* Press `m` or click "Mapping test mode" in the parameters panel
* (optional) Press `2` or click "Draw LED model" to see where the mapped LEDs are located
* Move the mouse around the installation, ensuring that the lights follow the on-screen display. Watch for lights that are stuck on, stuck off, displaying the wrong color.
* Press `m` again to leave mapping test mode.

Testing Sensors
---------------

While running CircleEngine:

* Inspect the spinners on-screen, be sure the angle is approximately right and that all spinners flash green.
* If a spinner doesn't flash, try moving it. Spinners only send data when the color changes: typically the values change a lot due to measurement noise, but it's possible for the sensors to settle on a single value and stop talking unless you move them.

Troubleshooting
---------------

If a sensor isn't responding, lights stop updating, or lights show the wrong color:

1. Unplug all USB cables from the Mac Mini
2. Unplug the AC power cables running to the USB hubs
3. Wait a few seconds
4. Reconnect the AC power cables
5. Reconnect the USB cables one by one, waiting a second or two between each

Calibrating Sensors
-------------------

While running CircleEngine:

* Press `tab` to bring up the parameter UI
* Use the `[]` keys or type a spinner number into the "Show color cube test" parameter in "Spinner controls"
* You will see a 3D color cube display showing the sensor calibration data. Move the mouse to rotate the camera.
* If the spinner has trouble rotating through a full cycle, you may need to reset the color cube. Press `q` or the "Clear current color cube" button.
* Slowly move the spinner through multiple full rotations until you've collected at least ~300 points in the cube.
* Move the spinner so it's exactly horizontal
* Press "Set current cube origin" to lock in this position
* Repeat for other spinners if necessary
* To save the calibration as default, click "Export color cubes"
* This will generate a file in the same directory as CircleEngine.app called `exported-color-cubes.json`
* You can rename this file to replace the defaults, located at `CircleEngine.app/Contents/Resources/init-color-cubes.json`

Parts
-----

* `bin` – Precompiled files for installation computer
* `growth-sim` – Cinder app that "grows" a forest using a dynamic simulation. This has many parameters that can be tweaked in real-time. After reaching a pleasing arrangement, the results can be exported to JSON.
* `layout` – Data files describing the current layout, generated by the other tools here.
* `fabricator` – Tools for building manufacturing files from the layout data.
* `circle-engine` – Cinder app for interactive animation.
* `firmware` – Teensyduino firmware for the sensor boards

Build Dependencies
------------------

* Cinder – `circle-engine` and `growth-sim` look for this in a `cinder_0.8.6_mac` directory alongside `forest` by default
* Node.js – Required for the `fabricator` tools
