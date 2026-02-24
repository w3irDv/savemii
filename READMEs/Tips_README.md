<p align="right"><a href="https://github.com/w3irdv/savemii/" title="SaveMii">Back&nbsp&nbsp&nbsp<img src="../savemii.png" width="125"></a>
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

You can follow this [procedure](../tutorials/Pretendo%20Network%20savedata%20Transfer.md)