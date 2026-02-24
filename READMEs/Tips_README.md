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


## Transfer Savedata to/from your Pretendo account

You can follow this [tutorial](../tutorials/Pretendo%20Network%20savedata%20Transfer.md)


## Recover your Wii U in case of a brick after a faulty restore

In extreme cases, a faulty restore of a system title can provoke a brick of your console. For example, you decide to wipe de Mii Make savedata expecting that the next time you open it all  mii database will be initialized from scratch. But MiiMaker savedata contains also some FaceLib that the Wii U Menu uses to render miis. It does not find them, and the startup process hangs on the Wii U splash forever ...

You will need a backup of the Mii Maker savedata

To recover from such a semi-brick  you can use [UDPIH: USB Host Stack exploit + Recovery Menu](https://gbatemp.net/threads/udpih-usb-host-stack-exploit-recovery-menu.613369/). 

At a glance:
- [Flash your device](https://github.com/GaryOderNichts/udpih#device-setup) with the UDPIH exploit image.
- Put the [Recovery Menu](https://github.com/GaryOderNichts/recovery_menu/releases) on the root of the SD card 
- Put a `network.cfg` file on the root of the SD card with the right information:

```
type=wifi
ssid=<your ssidhere>
key=<your wifikeyhere>
key_type=WPA2_PSK_AES
```
- Power On your Wii U, connect the device with the UDPIH explot when the Wii U logo appear. 
- Once you are in the `Recovery Menu`, enter in to `Load Network Configuration`.
- Once the network is configured, go to `Start wupserver`. The IP assigned to the Wii U will appear on the screen.
- Download this [wupclient.py](../scripts/wupclient.py) file to your computer, and update this line:
```python
def __init__(self, ip='192.168.1.33', port=1337)
```
with the IP assigned to the Wii U console.

- Create a directory `mm_bckp` in the folder where you downloaded `wupclient.py`
- Copy the backup of the Mii Maker `common` folder `mm_bckp`
- The layout should be:
```
wupclient.py
mm_bckp/stadio.sav
mm_bckp/db/FCL_DB.dat"
mm_bckp/db/FFL_HDB.dat"
mm_bckp/db/FFL_ODB.dat"
mm_bckp/db/FFL_ODB_OLD.dat"
``` 
- Now execute:

```sh
python3 -i wupclient.py
```

- And now execute this lines inside the interactive prompt:

```python
w.cd("/vol/storage_mlc01/usr/save/00050010/1004a200")

w.chmod("user",0x600)
w.chown("user",0x100000f6,0x400)

w.cd("/vol/storage_mlc01/usr/save/00050010/1004a200/user")

w.mkquota("common",0x660,0x1790000)
w.chown("common",0x1004a200,0x400)

w.cd("common")

w.up("mm_bckp/stadio.sav","stadio.sav")
w.chmod("stadio.sav",0x660)
w.chown("stadio.sav",0x1004a200,0x400)

w.mkdir("db",0x644)
w.chown("db",0x1004a200,0x400)

w.cd("db")

w.up("mm_bckp/db/FCL_DB.dat","FCL_DB.dat")
w.up("mm_bckp/db/FFL_HDB.dat","FFL_HDB.dat")
w.up("mm_bckp/db/FFL_ODB.dat","FFL_ODB.dat")
w.up("mm_bckp/db/FFL_ODB_OLD.dat","FFL_ODB_OLD.dat")

w.chmod("FCL_DB.dat",0x666)
w.chmod("FFL_HDB.dat",0x666)
w.chmod("FFL_ODB.dat",0x666)
w.chmod("FFL_ODB_OLD.dat",0x666)

w.chown("FCL_DB.dat",0x1004a200,0x400)
w.chown("FFL_HDB.dat",0x1004a200,0x400)
w.chown("FFL_ODB.dat",0x1004a200,0x400)
w.chown("FFL_ODB_OLD.dat",0x1004a200,0x400)
```




