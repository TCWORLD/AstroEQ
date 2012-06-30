There are two versions of the board:
"Through Hole" uses through-hole components (except for the DRV8824PWP IC's which are only available as an SMD Package)
"SMD" uses all surface mount components and is thus alot smaller.

The Through Hole board has been fully tested and I know works.
The SMD board I have double checked and looks correct, however I have not had it made, so functionality is not confirmed.

BOMs can be gotten by using the "Design Link" feature on the schematic. (In Eagle 6, click the "Design Link" on the right from the schematic editor, and then click Schematic.
This will then load the BOM and also Farnell part numbers. All items can be purchased through Farnell, though if you prefer, you can use the BOM to find similar parts from places such as Digikey or RS.

Please note that the board designs have two types of connector, the RJ11 type for Skywatcher mounts which use that connector, or a set of 8 screw terminals. You can only use one or the other, not both, though both part numbers are in the BOM.

For the Through Hole board, the DRV8824PWP ICs are to be soldered to the breakout boards on the right. If there is enough interest, I can sell the ICs ready soldered, but only if there are at leasst 5 people who need them.
Also, the IC sockets need to be of this sort: http://www.rshelectronics.co.uk/img/p/607-630-large.jpg
NOT this sort:  http://www.westfloridacomponents.com/mm5/graphics/H01/28-pin-machined-ic-socket.jpg
Else the breakout boards cannot be plugged in. Also note that in the BOM, you need twice as many of the 14x1 pin headers as these are used on the breakout boards. (If I do sell the IC's broken out, they will come with the headers attached).



If you have problems with the "Design Link" and retrieving the BOM, please open an issue on the GitHub page. Though I have tested and it should work.