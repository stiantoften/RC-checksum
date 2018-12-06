# Ratchet & Clank save.bin checksum repair
When extracting savegames from Ratchet & Clank (ps2) you get save.bin files. If you edit these files they will not work in the game because the game calculates a checksum upon saving. This application recalculates these checksums in the same way the game does it.

## More info
Figured I'd do a section with more info on how this was made in case somone wants to do this for another game :)
 
 ### Methodology 
Figured out how this all worked by opening the game in an emulator, and looking for the save data in cheat engine. Upon finding it I set a breakpoint at the data, and pressed save in-game. Then I stepped through the assembly code until I found the pattern. After some converting, and messing around, I ended up with this.

### Breakdown of the save.bin file
The file starts off with 8 bytes of file header

--Start of file--
* 0x00: Length of chunk0 + 8
* 0x04: Unknown


Then the rest of the file is divided into chunks with an 8-byte header followed by a data section. The amount of chunks depend on the game.

--Start of chunk--
* 0x00: Chunk length
* 0x04: Chunk checksum
* 0x08 to 0x08 + chunk length: Chunk data


The checksums are calculated only on the data portion of the chunks, and appears to just be a CRC-16 with 0x8320 as the initial value, and 0x1F45 as the polynomial.
