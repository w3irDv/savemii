# SaveMii WUT Port

Original by Ryuzaki-MrL WUT Port by me

WiiU/vWii Save Manager

This homebrew allows you to backup your Wii U and vWii savegames to the SD card and also restore them. Up to 256 backups
can be made per title. Backups are stored in sd:/wiiu/backups.

For sorting Titles press R to change sorting method and press L to change between descending and ascending.

Use it at your own risk and please report any issues that may occur.

## Quick Notes

Move across screen by using Up/Down D-Pad buttons. Cycle between options by using Left/Right D-Pad buttons. "A" to select a task / enter a menu. "B" to go back to the previous menu. Special functions can be accessed by using "X/Y/+/-" buttons, as described at the bottom line in each menu.

### Wii U Title Management / vWii Title Management

Allows you to backup/restore/wipe individual titles.

1. First select the title you want to manage.
	1. Wii U titles: If the title has not been initialized (you have not created an initial save by playing to it beforehand) it will be marked as "Not init" and will appear in yellow. You can try to manage it (restore from a previous backup), but the outcome is uncertain. It is recommended to first play and create "real" savedata before trying to restore any backup.
		1. An special case of this are vWii injects. They have both a Wii U title with no savedata  that will appear in "Wii U Title Management" marked as "Not init", and a vWii title where the game's savedata is that will appear in vWii Title Management after you play it and create an initial game savedata. Savedata for these titles must be managed from the vWii Management tasks. Just in case, Savemii allows you to manage the Wii U title portion, but it usually doesn't contain any savedata.   
2. Select the task you want to do:
	1. Backup: Copy savedata from USB/NAND to SD
	2. Restore: Copy savedata from SD to USB/NAND
	3. Wipe: Delete savedata from USB/NAND
	4. Export / Import to Loadiine: Legacy options to manage Loadiine savedata
	5. Copy to other Device: If savedata for a title is present in USB and NAND, copy it from one storage to the other

#### Backup
1. Select a slot to store the savedata. You can select any number from 0 to 255, each one representing a different folder in the SD card.
2. For Wii U titles, select which data to save:
	1. All users: Recommended option. Will backup all game data.
	2. From user: xxxxxxxx. Will only backup the data for the specified user/profile. In this case, you must also specify if you want to save the "common" data or not. "Common" savedata is data shared by all profiles. Titles can have common save data, profile savedata or both.
3. Press "A" to initiate the backup. After the backup is done, you can tag the slot with a meaningful name pressing "+" button while you are in the backup menu. If the slot is unneeded, you can delete it by pressing "-" button.

*Wii U titles savedata layout:*
```
sd:/wiiu/backups/
    xxxxxxxxyyyyyyyy/     # Title Id 
        0/
            saveMiiMeta.json
            80000001/     # one folder for each profile
                ...       # savedata
            80000002/
                ...
            ...
            common/
                ...
        1/                 # one folder for each slot
            ...
```
For vWii titles, savedata is directly under the slot folder.

#### Restore
1. Select a slot to get the data from.  By default, data from the "root" backupSet (the one where the manual backups are stored) is used. But you can also use slots from batch backupSets, by pressing "X" button in this menu and selecting the backupSet you want to use. BackupSets can be tagged by pressing "+" button in the BackupSet List Menu, or from the BackupSet Management in Main menu.
   To identify which data the slot contains: If the slot has been tagged, you will see its tag next to the slot number. On the top screen line, you will see which backupSet is being used. And at the last screen line, you can see when the savedata were taken, and from which console.
2. For Wii U titles, select witch data to restore:
	1. `From: All users / To: Same user than Source`
    This will restore all save data (profiles+common) from the selected slot keeping the same userid that was used to backup the data. This option can only be used to restore previous savedata from the same console, or if the profile ids in the new console are identical to the ones in the source console. If profile ids from source and target differ, you must use the next option.
	2. `From: select source user / To: select target user`. This will copy savedata from the specified source profile id in the slot backup to the specified target profile id in the console. You can specify if copy common savedata or not.
	   If you are just copying the savedata from one profile id to a different one in the same console, choose `copy common savedata: no`. If you  are restoring to a new console with different profile ids, just choose `copy common savedata: yes` once for any of the profile ids, and copy the rest of profiles with `copy common savedata: no`
    3. Press "A" to initiate the restore. 

#### Wipe
Sometimes you will need to wipe savedata in the console before restoring a previous backup: If a restore is unsuccesful, you can try to wipe previous data before attempting a new restore. Options are the same than in the *Backup* task, but now refer to savedata for the specified title in the NAND or in the USB. 

#### Copy to other device
If a title has savedata in the NAND and in the USB, you can copy it between both storages. Options are the same than in the *Restore* task.

### Batch Backup

You can backup savedata for all Wii U titles and all Wii titles at once using this tasks. Savedata will be stored in a "Backup set" in the `batch` directori, and can after be used to restore individual titles or to batch restore all of them.
BackupSets can be tagged by pressing "+" button in the BackupSet List menu or by entering in the menu Backupset Management from the Main menu.

```
sd:/wiiu/backups/batch/
    ${timestamp}/
        saveMiiMeta.json
        xxxxxxxxyyyyyyyy/          # one folder for each title
            0/                    # slot containing USB or NAND savedata for the title
                saveMiiMeta.json
                80000001/
                    ...
                80000002/
                    ...
                ... one folder for each profile
                common/
                    ...
            1/                      # slot containing NAND savedata for titles simultaneously installed in USB and NAND
                 ...
        xxxxxxxxxyyyyyyyy/
            0/
            ...
```

### Batch Restore

This task allows you to restore the savedata for all titles already installed in the Wii U or in the vWii from a batch backupSet,
1. Select wether you want to restore Wii U titles or vWii titles
2. Select the backupSet you want to restore from
3. Select wich data to restore. Options are the same than in the *Restore* task. You can also choose if you want to perform a full backup (recommended) or to wipe data before restoring it.
4. The list of all titles that are installed, that have a backup in the backupset, and that match  the savedata criteria choosen in the previous step will appear. You can select / deselect which titles to restore. Titles with "Not Init" will be skipped by default.
5. Once you have reviewed the list of titles to be restored, press "A". A summary screen will appear, and if it is OK, you can initiate the restore.
6. Once the restore is completed, a summary screen will show the number of sucess/failed/skipped titles.
7. The list of all titles will appear again, now showing the restored status. You can try to select failed titles and restore them again to see what the error is. Succesfully restored titles will be skipped.

### Backupset management
In this menu you can tag backupsets ("+") or delete the ones you don't need ("-"). You can also set the one you want to use to restore savedata ("A").


----


Reasons to use this over original mod:

- Faster copy speeds
- VC injects are shown
- If VC is vWii, the user is warned to go to the vWii saves section instead
- Demo support
- Fixes issues present in the mod version
- Shows useful info about the saves like its date and time of creation

Credits:

- Bruno Vinicius, for the icon
- Maschell, for libmocha and countless help
- Crementif for helping with freetype
- V10lator for helping with a lot of stuff
- Vague Rant for testing
