# OpenSimPit

_Open source one-size-fits-all DIY simulation cockpit based on Arduino and cheap modules from aliexpress_

----

OpenSimPit is a set of resources for building simpits (simulation gaming cockpits), for both flight simulators and space games (realistic or fantasy), using Arduino and boards and modules from aliexpress.

The strategy in OpenSimPit is to have a flexible, modular package of firmware/software, where the main board is entirely configurable via PC software to match your cockpit design (the board firmware code is not even aware of what your cockpit has). The focus is on cheap and widely available components to connect or solder yourself, so your shopping list will have things like a $2 I2C expansion board, not a $200 Airbus replica panel or the alike. 

The code can be deployed in your Arduino board without any coding at all (unless you want to add new features, of course - it's open source after all). This way, you can focus on the rest of the work - building and shopping for the nice gear - and set everything up with a clean "cockpit setup" interface.

The cockpit connects with the host computer recognised as a USB Joystick (if using the Bluepill board) or via USB Serial port (all board). The Serial port can be used directly in games compatible with OpenSimPit (''.dll''/''.so'' files will be provided for this, in this repository), or using a driver software on PC reading from the COM port and emulating a joystick (using e.g. vJoy).

----

This repository contains:

  - Main Arduino sketch -> this is the firmware for your cockpit main board. No changes required.

  - Secondary Arduino sketches where needed -> some features (like the Open-Smart battery display) might use a small Arduino Pro Mini as middle-man to bridge into the I2C network. (WIP, coming soon)
  
  - PC Configuration Software -> this is the graphical tool used to configure your board to let it know what your simpit has, what, where, how. It is made in Godot Engine and is also open source. (WIP, coming soon)
  
  - PC Serial Driver Software -> this allows you to connect your simpit via Serial port into any games by translating into a virtual joystick. You don't need this if you use a Bluepill board in USB Joystick mode, as it works directly like a USB joystick. Currently made in python and also open source - an executable binary (faster) C version is planned. (WIP, coming soon)
  
  - SDK for game developers (as ''.dll''/''.so'') for making games _directly_ compatible to OpenSimPit via Serial port (and therefore not requiring a Bluepill board). Game developers don't really _need_ the SDK, it is just a convenience: any programming language capable of reading a serial port can read the OpenSimPit packets directly as they are simple JSON strings (the SDK includes a sample python script to do this). (WIP, coming soon)