MCserver-RaspberryGPIO
======================

Plugin for the MCserver to control Raspberry Pi's GPIO from Minecraft.
If you want to add some code or make it better in any way, I would be most grateful!
You can see it working at http://www.youtube.com/watch?v=Y1ErAeNBC_0.

Installation:
1)
Install MCserver (http://mc-server.org) on your Raspberry Pi.
I had to build it manually, otherwise it gave me a SegFault after start - it takes about an hour to build (or you can use distributed build like distcc).
Note: This problem went away with newer MCserver releases.

2)
a) The Lua version.
Download the GPIO.so module from the "Lua only" subfolder and put it into /usr/local/lib/lua/5.1/ (if you have other version of Lua, change the path accordingly).
-if this doesn't work for some reason, try building it yourself (takes about five minutes): http://www.andre-simon.de/doku/rpi_gpio_lua/en/rpi_gpio_lua.php
 just watch out, as in the Makefile for it there is an old version of the Python lib linked, so you need to change that for it to work

b) The Lua and C-module version.	--Note: This is probably outdated with the new versions of MCserver and will not be maintained.
I rewrote some of slower functions into C to make it a bit faster, but I think it is even slower (probably because of the C to Lua interface dragging it down).
To use it, download the MCmodule.so from the "Lua with static C module" subfolder and put it into the folder described above.
To build it, download the main.c from "/Lua with static C module/C module source/" subfolder and build it with the instructions included.
You also need the WiringPi library installed (http://wiringpi.com/download-and-install/)!


3)
Edit the settings.ini in your MCserver directory.
Add "Plugin=RasPiGPIO" line somewhere in the "[Plugins]" section to load the lua file.

4)
Copy the files from the desired "Plugins" folder into your MCserver Plugins folder.
The path should be like this:
MCserver/Plugins/RasPiGPIO/*.lua
where * is main, arbitrary, blockHandler (only required if you use the Lua only) and CoreMessaging (not sure if required).

5)
Launch with "sudo ./MCServer" and enjoy!

---------------------------------------------------------------------------------------------------------------------------

In-game usage:
The script remembers last placed block (only for certain types) or you can "mark" any block by using a golden showel on the block.
Than write "/assignlast <Pin number on board> [IN]", where IN is optional flag for inputs.
Now only works with:
 -lever
 -stone/wooden button
 -redstone torch
 -redstone lamp
(as both input and output).
But new blocks can be easily added by some basic editing of the Lua scripts or the C source.
Note: levers as inputs are bugged for some reason - after being in the ON state, the redstone near the lever stays activated no matter what.

For the infosigns place a sign and write "/TEMP", "/CPU", or "/RAM" on the first line and see what happens! :)
