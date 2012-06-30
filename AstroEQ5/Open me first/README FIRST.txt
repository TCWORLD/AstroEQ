There are 5 steps to setup AstroEQ for the first time.

(1) There is a folder named 'hardware'.
---> This needs to be copied into the arduino install dirctory. When asked, merge folders with existing ones, replacing files.
---> This will add support to the arduino IDE for AstroEQ. 
[NOTE: If you plan to use the old Arduino Mega hardware, you can skip step (1)]

(2) In the 'Driver' folder is contained a .inf Windows compatible driver for the AstroEQ USB-Serial port.
---> When you connect AstroEQ, Windows will try to install the board automatically.
---> If Windows Update fails to find a driver, use the Add Hardware... wizard and specify to look in this folder.
---> If you have a Mac or use Linux it should install automatically, but I haven't tested this (I don't have a Mac or Linux)
[NOTE: If you plan to use the old Arduino Mega hardware, you can skip step (2)]

(3) The "Initialisation Variable Calculator" file will allow you to enter specifics about your mount.
---> It will generate a line of text: Synta synta(...stuff here...);
---> This is the initialisation call for your mount. More about this in step 4.

(4) In the AstroEQ5 folder, open AstroEQ5.ino with the Arduino IDE.
---> There is some introductary text which you can read if you want to understand more.
---> If not, move to the end of the text and find the line "Synta synta(1281, 26542080, 190985, 16, 184320, 10);" - Approx. line 101
---> Replace this line with the one generated in step (3).

(5) Finally you need to download the program. In arduino IDE do the following:
---> (a) Select 'Tools->Board->AstroEQ w/ Atmega162' if you have the new hardware, or 'Tools->Board->Arduino Mega' for the old hardware. 
[NOTE: If you use the old hardware there are two versions of an Arduino Mega: 1280 or 2560. Select the one you have]
---> (b) Select 'Tools->Serial Port->//your port//'
---> (c) Select Upload (File->Upload, or the second round button)

You should now be up and running. Setup EQMOD and connect. If all goes well AstroEQ is now setup and it will be just plug and play from now on.

If you have any problems, open an issue on the github repository and I will do my best to help.



As a sidenote, If you purchased either a kit, or the IC pack from me, the Atmega162 will come with its bootloader and USB-Serial IC will come ready programmed.
If you buy these IC's yourself, you will need a PIC programmer to burn the .hex file in the Driver folder to the USB-Serial IC, and Atmel programmer to burn the bootloader to the Atmega162 (Tools->Burn Bootloader in Arduino IDE).