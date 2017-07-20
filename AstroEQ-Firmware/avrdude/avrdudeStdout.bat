@echo off

set solndir=%~1
set device=%~2
set hexfile=%~3
set port=%~4

if /I "%device%" == "ATMega2560" (
    set prog=wiring
    set baud=115200
) else (
    set prog=arduino
    set baud=57600
)

echo Running AVRDude With Command:
echo "%solndir%avrdude\avrdude.exe" -C"%solndir%avrdude\avrdude.conf" -p%device% -c%prog% -b%baud% -D -v -Uflash:w:"%hexfile%":i -P%port% 2>&1

"%solndir%avrdude\avrdude.exe" -C"%solndir%avrdude\avrdude.conf" -p%device% -c%prog% -b%baud% -D -v -Uflash:w:"%hexfile%":i -P%port% 2>&1
