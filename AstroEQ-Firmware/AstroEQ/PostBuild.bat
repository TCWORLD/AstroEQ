<!-- : Begin batch script
@echo off

set SolutionDir=%~1
set OutputDirectory=%~2
set OutputFileName=%~3
set BoardName=%~4
set BoardTitle=%~5
set BoardMCU=%~6
set BoardBootloader=%~7
set BoardBaudRate=%~8

set /p vernum=< "%SolutionDir%VerNum.txt"

xcopy "%OutputDirectory%\%OutputFileName%.hex" "%SolutionDir%..\AstroEQ-ConfigUtility\hex\%BoardName%.hex" /Y
cscript //nologo "%~f0?.wsf" "%SolutionDir%..\AstroEQ-ConfigUtility\hex\versions.txt" "%BoardName%" "%vernum%" "%BoardTitle%" "%BoardMCU%" "%BoardBootloader%" "%BoardBaudRate%"

exit

----- Begin wsf script --->
<job><script language="VBScript">
Const ForReading = 1
Const ForWriting = 2

strFileName = Wscript.Arguments(0)
FirmwareName = Wscript.Arguments(1)
NewVersion = Wscript.Arguments(2)
FirmwareCommonName = Wscript.Arguments(3)
CPUName = Wscript.Arguments(4)
BootloaderName = Wscript.Arguments(5)
BaudRate = Wscript.Arguments(6)

Set objFSO = CreateObject("Scripting.FileSystemObject")
Set objFile = objFSO.OpenTextFile(strFileName, ForReading)
strText = objFile.ReadAll
objFile.Close

Set re = new regexp
re.Pattern = Replace(Replace(FirmwareName & ".*", "(", "\("), ")", "\)")
ReplaceText = FirmwareName & VBTab & NewVersion & VBTab & FirmwareCommonName & VBTab & CPUName & VBTab & BootloaderName & VBTab & BaudRate

If re.Test(strText) Then
    strNewText = re.Replace(strText, ReplaceText)
Else
    strNewText = Trim(strText) & ReplaceText & VBCrLf 
End If

Set objFile = objFSO.OpenTextFile(strFileName, ForWriting)
objFile.Write strNewText
objFile.Close
</script></job>