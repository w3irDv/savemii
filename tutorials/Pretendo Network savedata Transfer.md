#  How To - The easiest way to transfer save data to your Pretendo Network account (Wii U)

## Objective

Simplify the savedata transfer from an old NNID to a new PNID using a new feature of SaveMii 1.7.0 : `Batch Copy/Move Pofile`.

The difference between Move and Copy is the following:
* Move will wipe the targe user, then will move the savedata from the source user to the target user. You will end with only one copy of the savedata , under the new PNID profile.
* Copy will (optionally) wipe target data, and then will copy source data to the target profile, so you will end with two copies of the savedata (source data is not touched!).

Copy is the recommended option to use.

## Prerequisites
I will assume that you have followed the [Pretendo Setup Guide](https://pretendo.network/docs/install/wiiu) till the section  `Transferring save data to your Pretendo Network account`, so that you have already created a new PNID. And that you have already installed [SaveMii 1.70](https://github.com/w3irDv/savemii/releases/tag/1.7.0).

## How to
To copy all the savedata from one NNID to the new PNID, just:

1) Open SaveMii 1.7.0 /ProcessMod

   <img src="images/1.png" width="50">

2) Select "Batch Copy to Other Profile"
  ![](images/2.png)

3) For "user to copy from" option, select your old NNID (`Gravity` in this example).

   For "user to copy to", select your new PNID (`Xvr`in the picture).

   Leave the other two setting as they are:

   * Wipe target users savedata: Yes
   * Backup all data: Yes

   ![](images/3.png)

4) Press `A`  "Go to title Selection".

   All the titles that have savedata for the old NNID will appear. Just review the list and unmark any title savedata that you don't want to copy.
   ![](images/4.png)

5) Press `A` : ProfileCopy.

   The review screen will appear, last opportunity to verify that the NNID and the PNID are ok.
   ![](images/5.png)

6) If Ok, press `A`.

   Copy process will begin. For all selected titles, savedata from the PNID profile will be wiped, and then the savedata from the NNID profile will be copied to the PNID profile.
   ![](images/6.png)

7) When the copy finishes, a summary screen with the number of OKs/ KOs will appear.

   *Note: If the copy fails for some title, its name will appear here. The reason for failure would have been shown during the previous copy step. Just correct the problems with these titles and try the process again only for these ones. As a last restor, in case that you need it, an All Users backup  for the titles can be found on the sdcard* --> `sd:/wiiu/backups/batch/<timestamp> `).
   ![](images/7.1.png)

   ![](images/7.2.png)

END) That's all. Hope it helps!

