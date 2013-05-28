
On Error Resume Next

set scope = CreateObject("EQMOD.Telescope")

If Err.Number <> 0 Then
  WScript.Quit(1)
End If

scope.Connected = true

If Err.Number <> 0 Then
  WScript.Quit(2)
End If

scope.IncClientCount

WScript.Quit(0)