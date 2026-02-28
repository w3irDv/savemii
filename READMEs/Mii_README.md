<p align="right">
<a href="https://github.com/w3irdv/savemii/#savemii-wut-port-processmod" title="SaveMii">Back</a>&nbsp&nbsp
<a href="https://github.com/w3irdv/savemii/#savemii-wut-port-processmod" title="SaveMii"><img src="../savemii.png" width="125" align="center"></a>
<p align="right">


# Mii Management

This menu allows you to:
- Backup / Restore internal Wii U, Account and Wii Mii databases (repos)
- Wipe / Initialize internal Wii U and Wii Mii databases
- Export / Import / Wipe individual Miis between internal Wii U/Account/Wii repos and stage repos on the SD (including SavegameManager GX repos)
- Transform individual Miis on any repo. You can:
    - Make a Mii local to the console, transfer ownership from one Mii to another, transfer physical attributes from one Mii to another, convert betwen Normal/Special/Temporal Miis, togle copy and share attributes, update Mii Id, make a Mii a favorite one, update its CRC checksum

### Relation between repos and tasks.
Savemii has the following type of repos configured:
- Internal Wii U (FFL), Account and Wii (RFL) databases.
- Stage folders: folder on the SD where you can put individual Mii files (FFL_Stage, Account_Stage and RFL_Stage).
- Custom DB Files: FFL/RFL or Account databases on the SD , for testing puposes. They live on the SD. When ready (after importing and transforming Miis as you wish), can be copied to CEMU or Dolphin installations.

**Notes**
    - You can create an account custom db, by making an Account Mii Database backup and manually copying `8000000x/` folders from `SD:/wiiu/backups/MiiRepoBckp/mii_bckp_account/(slot_number)/`  to `SD:/wiiu/backups/MiiRepoBckp/mii_bckp_account_c`.
    - If a FFL database is founf on the USB (mounting a NAND as a USB using ISFSHax), it will appear as `FFL_USB` repo.

You can backup/restore any repo to/from a slot (whole databases). Each repo has its own set of slots, named from 0 to 255 (you can optionally tag them)

You can export/import Miis between any repos

```mermaid
flowchart TD

iddb[("Wii U, vWii, Account Internal DB Files")] <-- Export/Import --> idstage("Stage Folders")
iddb <-- Export/Import --> iddbsd[("Custom DB Files")]
iddbsd <-- Exporet/Import --> idstage

iddb & idstage & iddbsd <== Backup/Restore ==> idslots([Backup Slots])

subgraph "**SD**"
iddbsd
idslots
idstage
end

subgraph "**NAND**"
iddb
end
style iddb fill:#f9f,stroke:#333,stroke-width:1px
style iddbsd fill:#f9f,stroke:#333,stroke-width:1px
style idstage fill:#f9f,stroke:#333,stroke-width:1px
```

## Managing Mii Repos

### Select the repo to be managed

1. Enter into  _Mii Management_ task
1. First, select the repo you want to manage
2. Select the task you want to perform on it.

### Tasks operating on full repos

#### Backup / Restore  Tasks
1. Select the slot you want to use
2. Press `A` to initiate the backup or the restore. After the backup is done, you can tag the slot with a meaningful name pressing `+` button while you are in the backup menu. If the slot is unneeded, you can delete it by pressing `-` button.

Backup will include:
* Internal Wii U: FFL_ODB.dat and stadio.sav files. Can be moved between consoles.
* Internal Wii: RFL_DB.dat file. Can be moved between consoles
* Wii U Account database: all files under the act  profile folder for each profile in the console ( `account.dat`, `miiimgXX.dat`).
    `account.dat` has an attribute with the Mii image data (MiiData) but also contain the pasword of the profile and a lot of other stuff. For this reason, **Savemii only allows you to restore account data if it belongs to the same console** (i.e, if the serial id of the backup is the same than the serial id of the console when you are trying to restore). This check can be spoofed, **so be sure of what you are doing** in the case that you are restoring between different consoles, or if you try to restore a backup of a reinstalled console (which will probably share the serial id). In the later case, **the safe approach is just to Import individually each account Mii or use the Restore DB Mii Section task**.
    `miimgXX.dat` are static pictures of the Mii that the Wii U apps can use. Is updated when you register a Mii against a profile. 
* Stage folders: copy of the individual Mii files
* Custom DB folders: same files that in the case of internal ones.

#### Wipe DB task
1. Press `A` to initiate the wipe of the database. This will delete any file on a stage folder, or the FFL/RFL dat file in the case of internal databases. Account databases cannot be wiped.


#### Initialize DB task
1. Press `A` to initialize a database. **Beware: This will delete of Miis in the selected database**. In the case of a stage folder, it is equivalent to a wipe. For internal Wii U or Wii databases, it will create an empty  FFL/RFL dat file ready to accept Miis from the import/export tasks. For the internal Wii U database it will also create a `stadio.sav` file containing information from the account Miis. Account databases cannot be initialized. 

You must initialize custom databases if you want to play with them.

### Tasks operating on individual Miis

#### List Miis
All Miis in te selected repo will be shown. With `X` you can cycle between different screens showing different Miis attributes:
- copyable, shareable, normal/special/temp, favourite, duplicated.
    - Duplicate Miis are marked with a D or DUP. Mii Maker will delete any all but one duplicates. Mii Channel don't seem to pay attention to this fact.
- partial device_hash and author_id where the Mii was created
- mii_id (some flags + timestamp) , partial device_hash
- location (slot number for FFL or RFL databases, filename for Stage or Account databases)
- creator name and birth platform
- timestamp

#### Export Miis
Export individual Miis from one repo to a different one. Press `A` to initiate the task. For internal Databases, the Miis will be copied to the associated Stage Repo. For other databases, a new menu will appear where you can select the repo to export to. Press `A` again after moving the cursor to the desired repo. The Mii selection menu will appear. Select/Deselect individual Miis with `Y`, `DPAD-L/R`, or use `+/-` to select/deselect all at once. Then Press `A` to initiate the export. 

#### Import Miis
You can Import individual Miis from a repo to the currently selected repo. For internal Databases, the Miis will be searched for and copied from the associated Stage Repo. For other databases, you can select the repo to import from. Press `A`to initiate the task. Press `A` again after moving the cursor to the desired repo. Then select the Miis you want to import with `Y`, `DPAD-L/R`, or `+/-` and press `A` to proceed with the import.

**Note**: When importing a Mii into an account repository, you must select which account Mii to overwrite. All the usual Mii data (appearance, name, flags, etc.) will be copied from the Mii to the account Mii.

#### Wipe Miis
Select/Deselect the individual Miis to wipe with `Y`, `DPAD-L/R`, or `+/-` for select/deselect all. Then Press `A`

#### Transform Miis
Here you can apply a series of transformations to a Mii. First, as usual, select the Miis you want to modify with `Y`, `DPAD-L/R`, or `+/-`. Press `A`. The *Transform Tasks* menu will appear. You can scroll up or down through the different tasks and select or deselect the ones you want to apply to the selected Miis iusing `Y`, `DPAD-L/R`. Press `A` when you have selected all the tasks that you need to apply. *Trasfer Appearance* and *Transfer ownership* tasks require a Mii as a template. First select the Repo where the template Mii is located, then select the template Mii. Finally press `A` to transform the selected Miis.

Available tasks:
- _Transfer physical appearance from another Mii_: Selected Miis will get the physical appearance of the Mii you will select in the next menu
- _Transfer Ownership from another Mii_: Selected Miis will get the ownership attributes of the template Mii, so after they will belong to template console. Current games association is lost
- _Make them belong to this console_: Updates MAC Address and AuthID (WiiU) of the Mii, so that it will apeear as created on this console. Updates Mii Id, so games association is lost
- Update Mii Id (Timestamp): So the Mii has a new unique Mii Id (Beware! It will no longer be tied to any game that expects the older Mii Id)
- Toggle favorite flag: Mark Miis as a favorite one so they appear in games that support them
- Toggle Share/Mingle flag: So the Mii can travel to other consoles
- Toggle Normal/Special flag: You can transform a normal Mii into an special one, and viceversa. This updates the Mii Id, so games association is lost
- Toggle Foreign flag: Wii Miis can be forced as foreign irrespective of where they were created. This updates the Mii Id (Mii games association is lost)
- Update CRC: CRC will be recalculated for the selected Mii (if in ffsd,bin,cfsd or rsd files) or for the entire DB (for Miis in a FFL or RFL file repo)
- Togle Copy Flag On/Off: So people that does not own the Mii can modifiy it by creating a copy of the original
- Togle Temporary Flag On/Off: Temporary Miis cannot be seen in FFL DB. This updates the Mii Id (Mii games association is lost)

**Note**: Not all tasks can be selected on all repos. It the tasks is not available for the type of repo that you are managing, you won't be able to select it.

### Account Miis are special ...
After restoring/importing or transforming an account Mii, you will need to restart the console for the changes to have effect.

If you have restored the DB Mii Section, imported or transformed an account Mii, you will also need to re-register the Mii in order to the `miiimgXX.dat` files to be regenerated. If not, a mixture of the old Mii and of the new Mii will be shown.

### Mii Initial Checkpoint

The first time SaveMii runs Mii Management tasks in a console, it creates a copy of the current Mii DBs just in case you need them later. These are located at `SD:/wiiu/backups/mii_db_checkpoint/`. To use them, you must  manually copy them to a backup slot (`SD:/wiiu/backups/MiiRepoBckp/mii_bckp_ffl/(new number)` or `../mii_bckp_rfl` or `.//mii_bckp_account`) or to a custom Mii database (`SD:/wiiu/backups/mii_repos/mii_repo_FFL_C`, `../mii_repo_RFL_C`, or `../mii_repo_ACCOUNT_C`).







