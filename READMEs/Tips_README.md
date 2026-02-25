<p align="right">
<a href="https://github.com/w3irdv/savemii/#savemii-wut-port-processmod" title="SaveMii">Back</a>&nbsp&nbsp
<a href="https://github.com/w3irdv/savemii/#savemii-wut-port-processmod" title="SaveMii"><img src="../savemii.png" width="125" align="center"></a>
<p align="right">


## Restoring savedata not belonging to your WiiU

If you want to restore saved data obtained from internet or from any other "foreign" source, it's recommended to back up your game's save data first from your console. Then, find the backup folder on your SD card under `sd:/wiiu/backups/` (for new titles, this is very easy, as the folder is named after the title), create a new "slot" folder inside (the next available number will do), and copy the "foreign" savedata there. The content in the "slot" folder should look similar to:
```
8xxxxxx1/
8xxxxxx2/
...
common/
```

You can then return to SaveMii and restore this slot (deleting the destination data is recommended). Probably you will need to use the `From: select source user / To: select target user` to restore data in the right user.

If you follow an online guide, it will likely tell you to create/find a hexadecimal folder (`xxxxxxxxxyyyyyyyy`) for storing the title savedata. SaveMii will still detect savedata stored using this format, but will prompt you to convert it to the new format. If you do so, the new savedata will be moved to the first free slot available in the  backups for the given title (usually, savedata will be stored in the last slot). 

<p>
<p>
## Transfer Savedata to/from your Pretendo account

You can follow this [tutorial](../tutorials/Pretendo%20Network%20savedata%20Transfer.md)

<p>
<p>

## Restore miis from your previous console
You can restore your miis from a Wii U or Wii in a new Wii U. This can be done in several ways:

### (A) Restoring Wii U Miis through Mii Maker savedata
This will delete all miis in your new Wii U.

1. If you have access to your previous Wii U and you can run Savemii, perform a backup of Mii Maker from `Manage Wii U System Titles >> (Select Mii Maker title) >> Backup (with All Users optipon)`
2. Then in the new Wii U, wipe the Mii Maker savedata: `Manage Wii U System Titles >> (Select Mii Maker title) >> Wipe (with All Users option)`. **Important**: Say yes when Savemii ask to perform a backup, and put in in a new free slot
3. Restore the save with `Manage Wii U System Titles >> (Select Mii Maker title) >> Restore (with All Users option)`
4. If all goes OK, continue (optional step) with the secion `Modify the mii so they will seem to be created in the new console`
5. If the restore fails, restore the backup hat you have made in step 2.
 **Important**: Don't leave Savemii or restart your console unless the restore has succeed. If not, you will need to recover from a (semi)brick (See [recovery process](#recover-your-wii-u-in-case-of-a-brick-after-a-faulty-system-title-restore))

### (B) Restoring Wii U Miis by restoring Mii FFL repo
This will delete all miis in your new Wii U.
1. If you have access to your previous Wii U and you can run Savemii, go to `Mii Management >> (select FFL repo) >> Backup DB`
2. Then in the new Wii U, select `Mii Management >> (select FFL repo)` and backup or export the new console miis (if any).
3. Restore the FFL repo  with `Restore DB`
4. If all goes OK, continue (optional step) with the secion `Modify the mii so they will seem to be created in the new console`
5. If the restore fails, restore the backup that you have made in step 2.
6. As last resor,t you can `Wipe DB`, exit SaveMii, and start Mii Maker. It will create a new empty DB.

### (C) Restoring Wii U Miis by Mii Export/Import
Miis in you new Wii U will be kept.
1. If you have access to your previous Wii U and you can run Savemii, go to `Mii Management >> (select FFL repo) >> Export Miis`. All miis are selected. Just press `A` to export. This will copy all miis to the FFLStage repo.
2. Then in the new Wii U, select `Mii Management >> (select FFL repo)` and backup new console miis.
3. Go Back to the Select Repo menu. Select now `FFLStage repo`
3. Go to `Import Miis`. Select the miis you want to import and pre `A`. This will add the new miis to the FFL repo.

### (D) Restoring vWii miis
You can use procedure (B) or (C), but selecting in this case RFL repo or RFLStage repo.

### (E) I have access to the Wii U console through ftp or using the recovery menu, but I cannot execute Savemii
Miis in the new mii will be wiped. You need to copy this files from the Wii U console to the SD:

```
#### Wii U 

#### this applies to a EUR console, for JPN use 1004a000, and for USA use 1004a100
/vol/storage_mlc01/usr/save/00050010/1004a200/user/common/stadio.sav
/vol/storage_mlc01/usr/save/00050010/1004a200/user/common/db/FFL_ODB.dat

to

SD:/wiiu/backups/..../FFL_repo/(new number)/...

#### vWii
/vol/storage/slccmpt01/shared2/menu/FaceLib/RFL_DB.dat

to

SD:/wiiu/backups/..../RFL_repo/(new number)/...
```

and then use procedure `(B)`

### (E) I have access to the Wii U console through ftp or using the recovery menu, but I cannot execute Savemii, and I want to keep miis created in the new Wii U
Old miis will be added to the new ones. You need to copy this files from the Wii U console to the SD:

```
#### Wii U 

#### this applies to a EUR console, for JPN use 1004a000, and for USA use 1004a100
/vol/storage_mlc01/usr/save/00050010/1004a200/user/common/stadio.sav
/vol/storage_mlc01/usr/save/00050010/1004a200/user/common/db/FFL_ODB.dat


to

SD:/wiiu/backups/..../FFLC_repo/(new number)/...

#### vWii
/vol/storage/slccmpt01/shared2/menu/FaceLib/RFL_DB.dat

to

SD:/wiiu/backups/..../RFLC_repo/(new number)/...
```

and then use procedure `(C)`, but executing step 1 from the new Wii U console and selecting as source repo `FFLC` or `RFLC`

### (E) I don't have access to the Wii U console, but I have a NAND backup
IN procedures (C) or (D), extract stadio.sav and FFL_ODB.dat with wfs-extract

### (F) Mame imported miis as belonging to the new console
Imported miis from a different console will appear as foreign miis and cannot be edited. If you want to convert them in first-class citizens of the new console so they can be modified:

`Mii Management >> (select repo RFL o FFL) >> Trasnform tasks >> (select miis to be made local) >> Make them belong to this console : Yes >> (Press A)`

**Beware**: mii swill have a new Mii Id and association with games will be lost. At all effects, they are neww miis created on the new console.
 
<p>
<p>

## Recover your Wii U in case of a brick after a faulty System title restore

In extreme cases, a faulty restore of a system title can lead to a brick of your console. For example, you decide to wipe de Mii Make savedata expecting that the next time you open it the mii database will be initialized from scratch with the right permissions. But Mii Maker savedata contains also some FaceLib lib files that the Wii U Menu uses to render miis in the initial selection screen. It does not find them, and the startup process hangs on the Wii U logo forever ...

You will need a backup of the Mii Maker savedata. Always perform an `All Users` backup before trying any restore!

To recover from such a semi-brick  you can use [UDPIH: USB Host Stack exploit + Recovery Menu](https://gbatemp.net/threads/udpih-usb-host-stack-exploit-recovery-menu.613369/). 

At a glance:
- [Flash your device](https://github.com/GaryOderNichts/udpih#device-setup) with the UDPIH exploit image.
- Put the [Recovery Menu](https://github.com/GaryOderNichts/recovery_menu/releases) on the root of the SD card 
- Put a `network.cfg` file on the root of the SD card with the right information:

```
type=wifi
ssid=<your ssid here>
key=<your wifikey here>
key_type=WPA2_PSK_AES
```
- Power On your Wii U, connect the device with the UDPIH explot when the Wii U logo appear. 
- Once you are in the `Recovery Menu`, enter in to `Load Network Configuration`.
- Once the network is configured, go to `Start wupserver`. The IP assigned to the Wii U will appear on the screen.
- Download this [wupclient.py](../scripts/wupclient.py) file to your computer, and update this line with the IP assigned to the Wii U console.:
```python
def __init__(self, ip='192.168.1.33', port=1337)
```

- Create a directory `mm_bckp` in the folder where you downloaded `wupclient.py`
- Copy the backup of the Mii Maker `common` folder into `mm_bckp`
- The layout should be:
```
wupclient.py
mm_bckp/stadio.sav
mm_bckp/db/FCL_DB.dat
mm_bckp/db/FFL_HDB.dat
mm_bckp/db/FFL_ODB.dat
mm_bckp/db/FFL_ODB_OLD.dat
``` 
- Now execute:

```sh
python3 -i wupclient.py
```

- Finally execute this lines inside the interactive prompt (replacing the vale of `mii_maker` with that of for your region) :

```python

mii_maker_jpn="1004a000"
mii_maker_usa="1004a100"
mii_maker_eur="1004a200"

########################
# replace here mii_maker_eur with the one for your region
########################
mii_maker=mii_maker_eur
########################

owner=int("0x"+mii_maker)

w.cd("/vol/storage_mlc01/usr/save/00050010/"+mii_maker)

w.chmod("user",0x600)
w.chown("user",0x100000f6,0x400)

w.cd("/vol/storage_mlc01/usr/save/00050010/"+mii_maker+"/user")

w.mkquota("common",0x660,0x1790000)
w.chown("common",owner,0x400)

w.cd("common")

w.up("mm_bckp/stadio.sav","stadio.sav")
w.chmod("stadio.sav",0x660)
w.chown("stadio.sav",owner,0x400)

w.mkdir("db",0x644)
w.chown("db",owner,0x400)

w.cd("db")

w.up("mm_bckp/db/FCL_DB.dat","FCL_DB.dat")
w.up("mm_bckp/db/FFL_HDB.dat","FFL_HDB.dat")
w.up("mm_bckp/db/FFL_ODB.dat","FFL_ODB.dat")
w.up("mm_bckp/db/FFL_ODB_OLD.dat","FFL_ODB_OLD.dat")

w.chmod("FCL_DB.dat",0x666)
w.chmod("FFL_HDB.dat",0x666)
w.chmod("FFL_ODB.dat",0x666)
w.chmod("FFL_ODB_OLD.dat",0x666)

w.chown("FCL_DB.dat",owner,0x400)
w.chown("FFL_HDB.dat",owner,0x400)
w.chown("FFL_ODB.dat",owner,0x400)
w.chown("FFL_ODB_OLD.dat",owner,0x400)

flush_mlc()

exit
```

- Stop `wupserver`, go back to the `Recovery Menu` main screen, and Stop the Wii U
- Power On the Wii U. It should start as usual.





