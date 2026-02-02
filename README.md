# SaveMii WUT Port /ProcessMod 

<p align="center"><a href="https://github.com/w3irdv/savemii/" title="SaveMii"><img src="savemii.png" width="300"></a></p>
<p align="center">
<a href="https://github.com/w3irdv/savemii/releases" title="Releases"><img src="https://img.shields.io/github/v/release/w3irdv/savemii?logo=github"></a>
</p>

**WiiU/vWii Save Manager**

`Original by Ryuzaki-MrL - Mod by GabyPCGeek - WUT Port by DaThinkingChair - /ProcessMod by w3irDv.`

This homebrew allows you to back up your Wii U and vWii saved games to the SD card and restore them later. 

This mod expands SaveMii's capabilities to manage titles in bulk: multiple titles can be saved, restored and wiped at once using batch functions.

Savedata can also be easily moved between Wii U profiles, simplifying `Pretendo` setup.

Can also copy saves from NAND-USB if title is installed to both



## Quick Notes

Move across screen by using Up/Down D-Pad buttons. Cycle between options by using Left/Right D-Pad buttons. `A` to select a task / enter a menu. `B` to go back to the previous menu. Special functions can be accessed by using `X`/`Y`/`+`/`-` buttons, as described at the bottom line in each menu.

For sorting Titles press `R` to change sorting method and press `L` to change between descending and ascending.

Use it at your own risk and please report any issues that may occur.



## Wii U Title Management / vWii Title Management

Allows you to backup/restore/wipe individual titles.

1. First select the title you want to manage.
	1. Wii U titles: If the title has not been initialized (you have not created an initial save by playing to it beforehand) it will be marked as "Not init" and will appear in yellow. You can try to manage it (restore from a previous backup), but the outcome is uncertain. It is recommended to first play and create "real" savedata before trying to restore any backup.
		1. An special case of this are vWii injects. They have both a Wii U title with no savedata  that will appear in "Wii U Title Management" marked as "Not init", and a vWii title where the game's savedata is that will appear in vWii Title Management after you play it and create an initial game savedata. Savedata for these titles must be managed from the vWii Management tasks. Just in case, SaveMii allows you to manage the Wii U title portion, but it usually doesn't contain any savedata.   
2. Select the task you want to do:
	1. Backup: Copy savedata from USB/NAND to SD
	2. Restore: Copy savedata from SD to USB/NAND
	3. Wipe: Delete savedata from USB/NAND
	4. Move to Other Profile: Move savedata from one profile to a different one, Target profile will be wiped
	5. Copy to Other Profile: Copy savedata from one profile to a different profile. You will end with two copies of your savedata in different profiles.
	6. Export / Import to Loadiine: Legacy options to manage Loadiine savedata
	7. Copy to other Device: If savedata for a title is present in USB and NAND, copy it from one storage to the other

### Backup
1. Select a slot to store the savedata. You can select any number from 0 to 255, each one representing a different folder in the SD card. Individual backups will always be stored in the `Root backupSet`. Next to the slot number you will see a `[T]`if the title backup is using titleName format, or an `[H]` if it is using hexadecimal format. 
2. For Wii U titles, select which data to save:
	1. `Only common save`. This option will copy only common savedata.
	2. `All users`: Recommended option. Will backup all game data.
	3. `From user: xxxxxxxx`. Will only backup the data for the specified user/profile. In this case, you must also specify if you want to save the "common" data or not. "Common" savedata is data shared by all profiles. Titles can have common save data, profile savedata or both.
3. Press `A` to initiate the backup. After the backup is done, you can tag the slot with a meaningful name pressing `+` button while you are in the backup menu. If the slot is unneeded, you can delete it by pressing `-` button.

*Wii U titles savedata layout:*

```
sd:/wiiu/backups/         # Root backupSet
    TitleName [xxxxxxxx-yyyyyyy]/     #  New format! Legacy format was: xxxxxxxxyyyyyyy 
        0/
            SaveMiiMeta.json
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
**Note**: Starting with version 1.7.0, backup folders for new titles will _always_ be named after the title name. For older titles that use hexadecimal format, SaveMii will prompt you to convert them to the new format when it detects one. You can disable this prompt in the Options menu screen. If for the same title there is a backup folder using hexadecimal format and a also a backup folder using titleName format, data will always be stored inside the hexadecimal folder. If you decide to convert the hexadecimal folder to titleName format, its content will be merged with the one in the titleName folder (in a new slot), and from then on only the titleName folder will be used.
### Restore
1. Select a slot to get the data from.  If you haven't selected any backupSet, the data from the `Root backupSet`  (the one where the manual backups are always stored) is used. But you can also use data from any batch backupSet, by pressing the `X` button and selecting the backupSet you want to use. Notice that the last backupSet you previously selected in any task (Batch Restore or BackupSet Management) will be the one used here by default. BackupSets can be tagged by pressing `+` button in the BackupSet List Menu, or from the BackupSet Management in Main menu.
   To identify which data the slot contains: If the slot has been tagged, you will see its tag next to the slot number. On the top screen line, you will see which backupSet is being used. And at the last screen line, you can see when the savedata were taken, and from which console. Finally, next to the slot number you will see a `[T]`if the title backup is using titleName format, or an `[H]` if it is using hexadecimal format. In case that hexadecimal and titleName folder exists for the same title, save data from the hexadecimal folder will always be used. Conversion will merge slots from both savedata, and then only the new titleName folder will be used.
2. For Wii U titles, select witch data to restore:
	1. `Only common save`
       This will restore only common savedata.
	2. `From: All users / To: Same user than Source`
	    This will restore all save data (profiles+common) from the selected slot keeping the same userid that was used to backup the data. This option can only be used to restore previous savedata from the same console, or if the profile ids in the new console are identical to the ones in the source console. If profile ids from source and target differ, you must use the next option.
   3. `From: select source user / To: select target user`. This will copy savedata from the specified source profile id in the slot backup to the specified target profile id in the console. You can specify if copy common savedata or not.
	   If you are just copying the savedata from one profile id to a different one in the same console, choose `copy common savedata: no`. If you  are restoring to a new console with different profile ids, just choose `copy common savedata: yes` once for any of the profile ids, and copy the rest of profiles with `copy common savedata: no`
	 4. Press `A` to initiate the restore. 
	    **Note**: Starting with version 1.7.0, the `From: All Users` restore will detect and not allow restoring savedata to non-existent user profiles. In this case, perform specific `From: Select Source User / To: Select Destination User` restores, selecting the correct users.
### Wipe
Sometimes you will need to wipe savedata in the console before restoring a previous backup: If a restore is unsuccesful, you can try to wipe previous data before attempting a new restore. Options are the same than in the *Backup* task, but now refer to savedata for the specified title in the NAND or in the USB. 

### Move to other profile

You can directly move copy the savedata from one profile to a different profile in the same title+storage type. Target profile will be wiped, and then source profile will be renamed to the new profile. Common savedata is left untouched.  
Select witch data to move:

1. `From: select source user / To: select target user`. This will copy savedata from the specified source profile id in the slot backup to the specified target profile id in the console.
2. Press `A` to initiate the move. 

### Copy to other profile

You can directly copy the savedata from one profile to a different profile in the same title+storage type. You will end with two copies of the savedata in two different profiles.  Common savedata is left untouched.
Select witch data to copy:

1. `From: select source user / To: select target user`. This will copy savedata from the specified source profile id in the slot backup to the specified target profile id in the console.
2. Press `A` to initiate the copy. 

### Copy to other device
If a title has savedata in  NAND and in USB, you can transfer it between both storages. Options are the same than in the *Restore* task.



## Batch Backup

You can backup savedata for all Wii U titles and all Wii titles at once using this tasks. Savedata will be stored in a "Backup set" in the `batch` directori, and can after be used to restore individual titles or to batch restore all of them.
BackupSets can be tagged by pressing "+" button in the BackupSet List menu or by entering in the menu Backupset Management from the Main menu.

```
sd:/wiiu/backups/batch/
    ${timestamp}/                  # batch backupSet
        SaveMiiMeta.json
        TitleName [xxxxxxx-yyyyyyy]/          # one folder for each title
            0/                    # slot containing USB or NAND savedata for the title
                SaveMiiMeta.json
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

### Backup All
This task will backup all initialized wiiU and vWii titles in a backup set. No exclude is applied.

### Backup Wii U / Backup vWii titles
You can select the titles you want to back up. Press `Y` `<-` `->` to select or deselect titles. Press `+` `-` to select or deselect all titles.
Press `X` to access the "Backup Excludes" menu. There you can:

- Save currently unselected titles as "Backup Excludes."
- Apply saved "Backup Excludes", so that matching titles are deselected. You can also configure whether you always want to apply "Backup Excludes" in the "Options" menu.

The "Backup Excludes" option is useful if you have many titles that you don't usually play, so you don't need to back them up every time you perform a batch backup.



## Batch Restore

This task allows you to restore the savedata for all titles already installed in the Wii U or in the vWii from a batch backupSet,
1. Select wether you want to restore Wii U titles or vWii titles
2. Select the backupSet you want to restore from
3. Select wich data to restore. Options are the same than in the *Restore* task. You can also choose if you want to perform a full backup (recommended) or to wipe data before restoring it.
4. The list of all titles that are installed, that have a backup in the backupset, and that match  the savedata criteria chosen in the previous step will appear. You can choose which ones to restore: Press `Y` `<-` `->` to select or deselect individual  titles. Press `+` `-` to select or deselect all titles. Titles with "Not Init" will be skipped by default.
   **Note**: If you select `All users`and the the backup contains savedata from a non-existent profile, a warning will appear: by default, SaveMii will not  allow you to restore this backup (unless that you disable this check in the Options menu, but this is discouraged). You can restore the backup using the option  `From: select source user / To: select target user`. `To users` are always defined in the console.  
5. Once you have reviewed the list of titles to be restored, press `A`. A summary screen will appear, and if it is OK, you can initiate the restore.
6. Once the restore is completed, a summary screen will show the number of sucess/failed/skipped titles.
7. The list of all titles will appear again, now showing the restored status. You can try to select failed titles and restore them again to see what the error is. Successfully restored titles will be skipped.



## Batch Wipe

This task allows you to wipe  savedata belonging to the selected list of titles.

1. Select which data to wipe: `Only common save`, `All users`, `From: select source user / To: select target user`. `common save: Yes/No`. You can also choose if you want to perform a full backup (recommended).
2. The list of all installed titles that match the savedata criteria chosen in the previous step will appear. You can select / deselect which titles to copy with `Y` `<-` and `->`, or select/deselect all at once with `+` and `-`. 
3. Once you have reviewed the list of titles to be wiped, press `A`. A summary screen will appear, and if it is OK, you can initiate the wipe.
4. Once the wipe is completed, a summary screen will show the number of sucess/failed/skipped titles.
5. The list of all titles will appear again, now showing the wipestatus. You can try to select failed titles and wipe them again to see what the error is. Successfully wiped titles will be skipped.
   **Note**: Step 1 will detect savedata that is already stored in the console but that belongs to profiles that no longer exists. In these cases, you can choose to delete it (or  ignore it). 



## Batch Move / Copy to Other Profile

This task allows you to move / copy  savedata from one profile to a different profile for all titles already installed in the Wii U (or for some of them).

1. Select wich data to restore:`From: select source user / To: select target user`. You can also choose if you want to perform a full backup (recommended) or to wipe data before restoring it.
2. The list of all installed titles that match the savedata criteria chosen in the previous step will appear. You can select / deselect which titles to move / copy with `Y` `<-` and `->`, or select/deselect all at once with `+` and `-`. 
3. Once you have reviewed the list of titles to be moved/copied, press `A`. A summary screen will appear, and if it is OK, you can initiate the operation.
4. Once the move/copy is completed, a summary screen will show the number of sucess/failed/skipped titles.
5. The list of all titles will appear again, now showing the copy status. You can try to select failed titles and move/copy them again to see what the error is. Succesfully moved/copied titles will be skipped.
**Note**: Step 1 will detect savedata that is already stored in the console but that belongs to profiles that no longer exist. In these cases, you can  move it to an existent profile, or use Batch Wipe to delete it.



## Batch Copy to Other Device

This task allows you to move  savedata between NAND and USB  for titles that have already savedata in both media. 

1. Select if you want to move "From NAND to USB" or "From USB to NAND".
2. Select wich data to restore:``Only common save`, `All users`, `From: select source user / To: select target user`. `common save: Yes/No` .  You can also choose if you want to perform a full backup (recommended) or to wipe data before restoring it.
3. The list of all titles that are installed on both NAND and USB, and  that match  the savedata criteria you chose in the previous step will appear. You can select / deselect which titles to copy with `Y` `<-` and `->`, or select/deselect all at once with `+` and `-`. 
4. Once you have reviewed the list of titles to be restored, press `A`. A summary screen will appear, and if it is OK, you can initiate the copy.
5. Once the copy is completed, a summary screen will show the number of sucess/failed/skipped titles.
6. The list of all titles will appear again, now showing the copy status. You can try to select failed titles and restore them again to see what the error is. Successfully copied titles will be skipped.
   **Note**: Step 1 will detect savedata that is already stored in the console but that belongs to  user profiles that are no longer defined. If you select `All users` option, a warning will appear: by default, SaveMii will not  allow you to restore this backup (unless that you disable this check in the Options menu, but this is discouraged). You can copy the savedata using the option  `From: select source user / To: select target user`. `To users` are always defined in the console, or delete it with Batch Wipe.   



## Backupset management



In this menu you can tag backupsets (`+`) or delete the ones you don't need (`-`). You can also set the one you want to use to restore savedata (`A`).



## Configuration Options

Press `X` in the Main Menu to enter the Options screen. You can:

- Select Language
- Decide if Backup Excludes are always applied when entering the Title Select screen in Batch Backup
- Enable/disable the prompt for folder format conversion between the legacy hex format (`xxxxxxxxyyyyyyy`) and the title name format (`Title Name [xxxxxxxx-yyyyyyy]`)
- Enable/disable the prompt to delete savedata from non-existent profiles and to allow/not allow restores to non-existent profiles.

Configuration can be saved by pressing `+`



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

----

## Reasons to use /ProcessMod over previous mods:

- Fully compatible with Aroma
- Addresses critical issues present in previous versions of the mod
- Includes options to move/copy profiles, very useful for finalizing `Pretendo` setups
- Performs error checks on all backup and restore operations to ensure they complete successfully.
- All operations can be performed in batches.
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
