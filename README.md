*Last Updated: 4th September 2018*

# AstroEQ Telescope Mount Controller

### About The AstroEQ Project


AstroEQ was written and designed by Thomas Carpenter. Assistance with EQMOD and the Skywatcher Protocol was provided by Chris over at the EQMOD Yahoo group.

AstroEQ was primarily designed as an alternative to the SynScan upgrade for Skywatcher telescopes so that I could convert my non-GoTo mount into a full GoTo. Being a EE student at the time I decided to built my own controller rather than buying a commercial one. 

The primary goal of the project was to act as a gateway to connect the excellent EQMOD software (and hence ASCOM planetariums) to a motorised Equatorial mount that previously lacked Goto functionality.

The resulting design allows for almost any equatorial telescope mount which has bipolar stepper motors fitted to the RA and DEC axes to be connected to a PC and controlled via EQMOD.

The AstroEQ code contains a custom developed protocol decoder which uses the same Skywatcher protocol for communication that many Skywatcher go-to mounts use, and the same protocol which EQMOD was developed around. This allows it to be connected to EQMOD without any modification to that software regardless of what type of mount it is.

### Versions

The current software and hardware versions are as follows:

 * Config Utility Version: **3.9.0**
 * Software Verison: **8.14**
 * Hardware Version: **4.6/4.7 - see note below**

The AstroEQ configuration utility and precompiled firmware can be downloaded from the AstroEQ website at: https://astroeq.co.uk/download

The configuration utility is designed to run natively on Windows or Debian (e.g. Raspian/Ubuntu). You only need to run this utility when programming the mount.

I recommend people use the V4.6 design for DIY controllers. The blank PCBs I sell through the AstroEQ website will remain the V4.6 version.

The new V4.7 hardware design is functionality equivalent to the V4.6 design, but uses a mixture of PTH and SMD components to make it compatible with machine assembly. I will be having future batches of AstroEQ controllers professionally assembled.

### Making your own AstroEQ 

There are two hardware options:

 * Use an Arduino Mega (Uno is not supported), which requires only two DRV8825 or A4988 driver boards and a few passive components to build.
 * Use a completely custom hardware desgin which is based around an Atmega162 chip. Although this is not officially supported by the Arduino IDE, I have modified the bootloader to suit.

Further information can be found at https://astroeq.co.uk/buildown

### Help and Support

If you have any questions or need support, I have opened an AstroEQ Forum which can be found at https://astroeq.co.uk/forum/

Additionally a small number of AstroEQ controllers are available to purchase. These can be bought from me on my website (https://astroeq.co.uk/purchase), however please contact me through the website first to make sure there are some before ordering.

The AstroEQ Firmware is currently proving to be very stable. If you do happen across any glitches, let me know as I can usually fix them fairly quickly.

### Required Software

Ascom can be downloaded from their website at: http://ascom-standards.org/ 

The EQMOD Project can be found at: http://eq-mod.sourceforge.net/
