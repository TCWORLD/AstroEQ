@echo off

set SolutionDir=%1

set /p vernum=< %SolutionDir%VerNum.txt

echo "%vernum%" > %SolutionDir%VerNumStr.txt
echo //AUTO GENERATED - DO NOT MODIFY >> %SolutionDir%VerNumStr.txt

exit
