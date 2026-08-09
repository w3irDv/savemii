
<p align="right">
<a href="https://github.com/w3irdv/savemii/#savemii-wut-port-processmod--savemymiis" title="SaveMii">Back</a>&nbsp&nbsp
<a href="https://github.com/w3irdv/savemii/#savemii-wut-port-processmod--savemymiis" title="SaveMii"><img src="../savemii.png" width="125" align="center"></a>
<p align="right">

# Recovery Resources
- [Recover your Wii U in case of a brick after a faulty System title restore](#recover-your-wii-u-in-case-of-a-brick-after-a-faulty-system-title-restore)
- [Back up your saves and Miis over the network](#back-up-your-saves-and-miis-over-the-network)
---

## Recover your Wii U in case of a brick after a faulty System title restore

In extreme cases, a faulty restore of a system title can lead to a brick of your console. For example, you decide to wipe de Mii Make savedata expecting that the next time you open it the mii database will be initialized from scratch with the right permissions. But Mii Maker savedata contains also some FaceLib lib files that the Wii U Menu uses to render miis in the initial selection screen. It does not find them, and the startup process hangs on the Wii U logo forever ...

We will cover the case that an error manipulating Mii Maker savedata leads to a brick. For orher cases, a straighforward adaption of functions `wipe_mii_maker/check_mii_maker_is_wiped/restore_mii_maker` in `wupclient.py` will be needed. 

You will need a backup of the Mii Maker savedata. Always perform an `All Users` backup before trying any restore! At worst, you will have the initial checkpoint backup in `sd:/wiiu/backups/mii_db_checkpoint/(WiiU serial number)`.

To recover from such a semi-brick  you can use [UDPIH: USB Host Stack exploit + Recovery Menu](https://gbatemp.net/threads/udpih-usb-host-stack-exploit-recovery-menu.613369/). 

At a glance:
- [Flash your device](https://github.com/GaryOderNichts/udpih#device-setup) with the UDPIH exploit image.
- Insert the SD card in your PC
- Put the [Recovery Menu](https://github.com/GaryOderNichts/recovery_menu/releases) on the root of the SD card 
- Put a `network.cfg` file on the root of the SD card with the following information:

```
type=wifi
ssid=<your ssid here>
key=<your wifikey here>
key_type=WPA2_PSK_AES
```
- Download this [wupclient.py](../scripts/wupclient.py) file to your computer
- Create a directory `mm_bckp` in the folder where you downloaded `wupclient.py`
- Copy the backup of the Mii Maker `common` folder into `mm_bckp`
    - use the last backup you have
    - If you don't have one handy, keep in mind that the first time SaveMii ran a Mii management task, it created a backup in `sd:/wiiu/backups/mii_db_checkpoint/(WiiU serial number)`   

    The layout should be:
    ```
    wupclient.py
    mm_bckp/stadio.sav
    mm_bckp/db/FCL_DB.dat
    mm_bckp/db/FFL_HDB.dat
    mm_bckp/db/FFL_ODB.dat
    mm_bckp/db/FFL_ODB_OLD.dat
    ``` 
- Put the SD card on your Wii U
- Power On your Wii U, connect the device with the UDPIH exploit when the Wii U logo appear. 
- Once you are in the `Recovery Menu`, enter in to `Load Network Configuration`.
- Once the network is configured, go to `Start wupserver`. The IP assigned to the Wii U will appear on the screen.
- Update this line in `wupclient.py`with the IP assigned to the Wii U:
```python
def __init__(self, ip='YOUR.IP.HE.RE', port=1337)
```

- Now execute:

```sh
python3 -i wupclient.py
```

- Run these lines inside the interactive prompt. Substitute `"your_region_here"` by "eur", "usa" or "jpn"


```python
wipe_mii_maker("your_region_here")

check_mii_maker_is_wiped("your_region_here")
```

- Once the Mii Maker folder is empty, restore the backup with:

```python
restore_mii_maker("your_region_here")

exit
```

- Stop `wupserver`, go back to the `Recovery Menu` main screen, and Shutdown the Wii U
- Unplug the UDPIH device
- Power On the Wii U. It should start with no problems.

---

## Back up your saves and Miis over the network

Sometimes you need to rescue a console's data without relying on homebrew running on it: a dying eMMC, a Wii U you can only reach through the `Recovery Menu`, or simply wanting an offline, scriptable backup before you attempt any risky restore. You can pull all of its savedata and Miis straight to the SD card over your network, in SaveMii's own on-disk format, ready to restore on this or another console.

Two small Python tools do this from your PC, talking to `wupserver` (TCP port `1337`) on the console — the same server you start from the `Recovery Menu` above:

- `backup_saves.py` — Wii U user + system saves, vWii saves, and injects (vWii injects are backed up under their Wii U ids; GameCube injects are reported so you can restore them from Nintendont).
- `backup_miis.py` — the three internal Mii databases (FFL, RFL, ACCOUNT), plus importable custom (`_C`) repos.

Both write under `sd:/wiiu/backups/`, so the result shows up directly in SaveMii's `Batch Restore` and Mii repo lists.

At a glance:
- Get the scripts: `git clone https://github.com/w3irDv/savemii.git` and `cd savemii/scripts/WUP` (or just download [`wupclient.py`](../scripts/WUP/wupclient.py), [`backup_saves.py`](../scripts/WUP/backup_saves.py) and [`backup_miis.py`](../scripts/WUP/backup_miis.py) into one folder).
- Start `wupserver` on the console through the [Recovery Menu](https://github.com/GaryOderNichts/recovery_menu) — the same UDPIH setup as above (flash the image, `network.cfg`, `Load Network Configuration`, `Start wupserver`) — and note the IP it prints.
- Insert an SD card with enough free space in the console.
- From your PC, run (using that IP), saves first, then Miis:

```sh
python3 backup_saves.py <console-ip>
python3 backup_miis.py  <console-ip>
```

- Each run flushes the SD at the end; once you see the final `done:` line it is safe to power the console down and eject the card.
- Move the SD card to the target Wii U and restore with `SaveMii / ProcessMod`: saves appear in `Batch Restore`, Miis in the Mii repo import screen.

See the full walkthrough — prerequisites, output layout, injects, permission sidecars and reliability details — in [how-to.backup-saves-and-miis-with-WUP.md](../scripts/WUP/how-to.backup-saves-and-miis-with-WUP.md).





