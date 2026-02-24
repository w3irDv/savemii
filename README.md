# SaveMii WUT Port /ProcessMod /SaveMyMiis

<p align="center"><a href="https://github.com/w3irdv/savemii/" title="SaveMii"><img src="savemii.png" width="350"></a></p>
<p align="center">
<a href="https://github.com/w3irdv/savemii/releases" title="Releases"><img src="https://img.shields.io/github/v/release/w3irdv/savemii?logo=github"></a>
</p>

**WiiU/vWii Save + Mii Manager**

`Original by Ryuzaki-MrL - Mod by GabyPCGeek - WUT Port by DaThinkingChair - /ProcessMod & /SaveMyMiis by w3irDv.`

This homebrew allows you to back up your Wii U and vWii saved games and miis to the SD card and restore them later.

This mod expands SaveMii's capabilities:
- to manage titles in bulk: multiple titles can be saved, restored and wiped at once using batch functions.
- It can also backup all Mii databases and import/export/transform any mii. 

Savedata can also be easily moved between Wii U profiles, simplifying `Pretendo` setup.

Can also copy saves from NAND-USB if title is installed to both


## Quick Notes

Move across screen by using Up/Down D-Pad buttons. Cycle between options by using Left/Right D-Pad buttons. `A` to select a task / enter a menu. `B` to go back to the previous menu. Special functions can be accessed by using `X`/`Y`/`+`/`-` buttons, as described at the bottom line in each menu.

For sorting Titles press `R` to change sorting method and press `L` to change between descending and ascending.

Use it at your own risk and please report any issues that may occur.


## Detailed HowTos
<p align="left">
<a href="READMEs/Saves_README.md" title="Base"><img src="READMEs/savemii_base.png" width="40" align="center"></a>&nbsp
<a href="READMEs/Saves_README.md" title="Base">Base tasks</a>
<p align="left">

<p align="left">
<a href="READMEs/Batch_README.md" title="Batch"><img src="READMEs/batch.png" width="40" align="center"></a>&nbsp
<a href="READMEs/Batch_README.md" title="Batch">Batch Process</a>
<p align="left">

<p align="left">
<a href="READMEs/Batch_README.md" title="Batch"><img src="READMEs/batch_2.png" width="40" align="center"></a>&nbsp
<a href="READMEs/Batch_README.md" title="Batch">Batch Process</a>
<p align="left">


<p align="left">
<a href="READMEs/Mii_README.md" title="Miis"><img src="READMEs/miis.png" width="40" align="center"></a>&nbsp
<a href="READMEs/Mii_README.md" title="Miis">Mii Management</a>
</p>
<p align="left">

<p align="left">
<a href="READMEs/Tips_README.md" title="Tips"><img src="READMEs/tips_5.png" width="40" align="center"></a>&nbsp
<a href="READMEs/Tips_README.md" title="Tips">Tips</a>
</p>
<p align="left">


----

## Reasons to use /ProcessMod over previous mods:

- Fully compatible with Aroma
- You can manage Miis
- Addresses critical issues present in previous versions of the mod
- Includes options to move/copy profiles, very useful for finalizing `Pretendo` setups
- Performs error checks on all backup and restore operations to ensure they complete successfully.
- All operations can be performed in batches.
- You can restore uninitialized titles (WiiU and injects), without needing to run the game before restoring
- It preserves file permissions (WiiU) and can backup saves with FAT32 non-compliant chars in its filename
- Maintained
- WUT port additions to the original:
  - Faster copy speeds
  - VC injects are shown
  - If VC is vWii, the user is warned to go to the vWii saves section instead
  - Demo support
  - Fixes issues present in the mod version
  - Shows useful info about the saves like its date and time of creation


## Credits:

- Bruno Vinicius, for the icon
- Maschell, for libmocha and countless help
- Crementif for helping with freetype
- V10lator for helping with a lot of stuff
- Vague Rant for testing
- xploit-U, for creating the WUT port
