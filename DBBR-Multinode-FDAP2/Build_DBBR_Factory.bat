echo off

echo cd z:\wsg\ISA100\Src\BBR\DBBR\out\DBBR_ResetToFactory\Exe\
z:
cd \wsg\ISA100\Src\BBR\DBBR\out\DBBR_ResetToFactory\Exe\

echo del WCI.*
del WCI.*

echo copy "..\..\..\build\bootloader.hex WCI.HEX"
copy "..\..\..\build\bootloader.hex" WCI.HEX

echo "..\..\..\out\DBBR_Platform\Exe\DBBR_Honeywell.hex >>WCI.HEX"
type "..\..\..\out\DBBR_Platform\Exe\DBBR_Honeywell.hex" >>WCI.HEX

echo "..\..\..\build\HEX2BIN.EXE BBR_FW.HEX >WCI.ERR"
..\..\..\build\HEX2BIN.EXE WCI.HEX >WCI.ERR

echo del WCI.ERR
del WCI.ERR

echo dir WCI.*
dir WCI.*
