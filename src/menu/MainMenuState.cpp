#include <BackupSetList.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchBackupState.h>
#include <menu/BatchJobOptions.h>
#include <menu/BatchJobState.h>
#include <menu/ConfigMenuState.h>
#include <menu/MainMenuState.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/statDebug.h>

#define ENTRYCOUNT 9

extern FSAClientHandle handle;

void MainMenuState::render() {
    if (this->state == STATE_DO_SUBSTATE) {
        if (this->subState == nullptr) {
            OSFatal("SubState was null");
        }
        this->subState->render();
        return;
    }
    if (this->state == STATE_MAIN_MENU) {
        DrawUtils::setFontColor(COLOR_INFO);
        consolePrintPosAligned(0, 4, 1, LanguageUtils::gettext("Main menu"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 2, LanguageUtils::gettext("   Wii U Save Management (%u Title%s)"), this->wiiuTitlesCount,
                                 (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 3, LanguageUtils::gettext("   vWii Save Management (%u Title%s)"), this->vWiiTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 5, LanguageUtils::gettext("   Batch Backup"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 6, LanguageUtils::gettext("   Batch Restore"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 7, LanguageUtils::gettext("   Batch Wipe"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
        Console::consolePrintPos(M_OFF, 8, LanguageUtils::gettext("   Batch Move to Other Profile"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 6);
        Console::consolePrintPos(M_OFF, 9, LanguageUtils::gettext("   Batch Copy to Other Profile"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 7);
        Console::consolePrintPos(M_OFF, 10, LanguageUtils::gettext("   Batch Copy to Other Device"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 8);
        Console::consolePrintPos(M_OFF, 12, LanguageUtils::gettext("   BackupSet Management"));
        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2 + cursorPos + ((cursorPos > 1) ? 1 : 0) + ((cursorPos > 7) ? 1 : 0), "\u2192");
        consolePrintPosAligned(17, 4, 2, LanguageUtils::gettext("\uE002: Options \ue000: Select Mode"));
    }
}

ApplicationState::eSubState MainMenuState::update(Input *input) {
    if (this->state == STATE_MAIN_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiiutitles, this->wiiuTitlesCount, true);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiititles, this->vWiiTitlesCount, false);
                    break;
                case 2:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchBackupState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, RESTORE);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, WIPE_PROFILE);
                    break;
                case 5:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, MOVE_PROFILE);
                    break;
                case 6:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobOptions>(this->wiiutitles, this->wiiuTitlesCount, true, PROFILE_TO_PROFILE);
                    break;
                case 7:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchJobState>(this->wiiutitles, this->wiititles, this->wiiuTitlesCount, this->vWiiTitlesCount, COPY_TO_OTHER_DEVICE);
                    break;
                case 8:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>();
                    break;
                default:
                    break;
            }
        }
        if (input->get(ButtonState::TRIGGER, Button::X)) {
            this->state = STATE_DO_SUBSTATE;
            this->subState = std::make_unique<ConfigMenuState>();
        }
        if (input->get(ButtonState::TRIGGER, Button::UP) || input->get(ButtonState::REPEAT, Button::UP))
            if (--cursorPos == -1)
                ++cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::DOWN) || input->get(ButtonState::REPEAT, Button::DOWN))
            if (++cursorPos == ENTRYCOUNT)
                --cursorPos;
        if (input->get(ButtonState::TRIGGER, Button::Y) || input->get(ButtonState::REPEAT, Button::Y)) {
            //FSError fserror;
            //fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/1","/vol/storage_usb01/usr/save/00050000/10200800/user/1:1");
            //fserror =  FSAMakeDir(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/2:2",(FSMode) 0x666);
            //Console::promptMessage(COLOR_BG_KO,"%s",FSAGetStatusStr(fserror));
            //mkdir("storage_usb01:/usr/save/00050000/10200800/user/3:3", 0666);
            //"\\:*?\"<>|";

            /*
            //rename ok , too long
            Console::promptMessage(COLOR_BG_OK,":");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/1","/vol/storage_usb01/usr/save/00050000/10200800/user/1:1");
            Console::promptMessage(COLOR_BG_KO,"%s",FSAGetStatusStr(fserror)); // ok
            fserror = FSAMakeDir(handle, "/vol/storage_usb01/usr/save/00050000/10200800/user/dir1:1", (FSMode) 0x666); // ok
            Console::promptMessage(COLOR_BG_KO,"mkdir %s",FSAGetStatusStr(fserror));
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/5","/vol/storage_usb01/usr/save/00050000/10200800/user/dir1:1/5<5");
            Console::promptMessage(COLOR_BG_KO,"special : in folder %s",FSAGetStatusStr(fserror));// ok
            mkdir("fs:/vol/external01/prova/1:1", 0666);
            Console::promptMessage(COLOR_BG_KO,"%s",strerror(errno));   // too long

            //rename ok , no such file or dir 
            Console::promptMessage(COLOR_BG_OK,"\\");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/2","/vol/storage_usb01/usr/save/00050000/10200800/user/2\\2"); // ok
            Console::promptMessage(COLOR_BG_KO,"%s",FSAGetStatusStr(fserror));
            fserror = FSAMakeDir(handle, "/vol/storage_usb01/usr/save/00050000/10200800/user/dir2\\2", (FSMode) 0x666);  // ok
            Console::promptMessage(COLOR_BG_KO,"mkdir %s",FSAGetStatusStr(fserror));
            */
            //fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/6","/vol/storage_usb01/usr/save/00050000/10200800/user/dir2\\2/5:5");
            //Console::promptMessage(COLOR_BG_KO,"special : in folder %s",FSAGetStatusStr(fserror)); // ok
            /*
            mkdir("fs:/vol/external01/prova/2\\2", 0666);
            Console::promptMessage(COLOR_BG_KO,"%s",strerror(errno));  // no such file or directory
            */

            //rename ok , too long
            /*
            Console::promptMessage(COLOR_BG_OK, ": in slc");


            FSAFileHandle fd;
            fserror = FSAOpenFileEx(handle, "/vol/storage_slcc01/title/00010000/534d4e50/data/prova", "w", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fd);
            Console::promptMessage(COLOR_BG_KO, "prova - %s", FSAGetStatusStr(fserror)); // OK
            fserror = FSACloseFile(handle, fd);
            Console::promptMessage(COLOR_BG_KO, "close prova - %s", FSAGetStatusStr(fserror)); // OK

            fserror = FSAOpenFileEx(handle, "/vol/storage_slcc01/title/00010000/534d4e50/data/prov:a", "w", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fd);
            Console::promptMessage(COLOR_BG_KO, "prov:a -  %s", FSAGetStatusStr(fserror)); // OK
            fserror = FSACloseFile(handle, fd);
            Console::promptMessage(COLOR_BG_KO, "close prov:a %s", FSAGetStatusStr(fserror)); // OK
            
            FILE *source = fopen("storage_slcc01:/title/00010000/534d4e50/data/b:dd", "wb");  // illegal byte sequence, but has created the file
            Console::promptMessage(COLOR_BG_KO,"open for wrtit %s",strerror(errno));
            fclose(source);
            Console::promptMessage(COLOR_BG_KO,"close %s",strerror(errno));   // it is not clear if it has close the file ....  illegal byte sequence error again    
            

            //rename("storage_slcc01:/title/00010000/534d4e50/data/b","storage_slcc01:/title/00010000/534d4e50/data/1:1");
            //Console::promptMessage(COLOR_BG_KO,"%s",strerror(errno));  // IO_ERROR


            //fserror = FSARename(handle,"/vol/storage_slcc01/title/00010000/534d4e50/data/b","/vol/storage_slcc01/title/00010000/534d4e50/data/1:1");
            //Console::promptMessage(COLOR_BG_KO,"%s",FSAGetStatusStr(fserror)); // // invalid_param
            */
            /*
            fserror = FSAMakeDir(handle, "/vol/storage_slcc01/title/00010000/534d4e50/data/dir1:1", (FSMode) 0x666);  // Ok
            */
            //Console::promptMessage(COLOR_BG_KO,"mkdir %s",FSAGetStatusStr(fserror));
            //fserror = FSARename(handle,"/vol/storage_slcc01/title/00010000/534d4e50/data/b","/vol/storage_slcc01/title/00010000/534d4e50/data/d<5"); // invalid param
            //Console::promptMessage(COLOR_BG_KO,"rename to file special  %s",FSAGetStatusStr(fserror));

            //mkdir("/vol/storage_slcc01/title/00010000/534d4e50/data/directorio", 0666);     // not owner
            //Console::promptMessage(COLOR_BG_KO,"mkdir normal %s",strerror(errno)); //

            /*
            fserror = FSAMakeDir (handle, "/vol/storage_slcc01/title/00010000/534d4e50/data/directorio", (FSMode) 0x666);   // OK
            Console::promptMessage(COLOR_BG_KO,"mkdir normal %s",FSAGetStatusStr(fserror)); // 

            fserror = FSARename(handle,"/vol/storage_slcc01/title/00010000/534d4e50/data/directorio","/vol/storage_slcc01/title/00010000/534d4e50/data/dir:ectorio"); // OK
            Console::promptMessage(COLOR_BG_KO,"rename dir to special %s",FSAGetStatusStr(fserror)); 

            */
            //mkdir("/vol/storage_slcc01/title/00010000/534d4e50/data/dirn:n", 0666);    // softlock
            //Console::promptMessage(COLOR_BG_KO,"mkdir special %s",strerror(errno)); //

            /*
            fserror = FSAMakeDir (handle, "/vol/storage_slcc01/title/00010000/534d4e50/data/dir:n", (FSMode) 0x666);  // OK
            Console::promptMessage(COLOR_BG_KO,"mkdir spectal %s",FSAGetStatusStr(fserror)); // invalid path

            fserror = FSARename(handle,"/vol/storage_slcc01/title/00010000/534d4e50/data/c","/vol/storage_slcc01/title/00010000/534d4e50/data/dirn:n/55"); //   Not FOUND   
            Console::promptMessage(COLOR_BG_KO,"move normal file to special dir  %s",FSAGetStatusStr(fserror));

            flushVol("storage_usb01:/usr/save/00050000/10200800/user/");
            flushVol("storage_slcc01:/title/00010000/534d4e50/data");

            */
            /*
            // invald_path,  too long
            Console::promptMessage(COLOR_BG_OK,"*");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/3","/vol/storage_usb01/usr/save/00050000/10200800/user/3*3");
            Console::promptMessage(COLOR_BG_KO,"%s",FSAGetStatusStr(fserror)); // invalid path
            fserror = FSAMakeDir (handle, "/vol/storage_usb01/usr/save/00050000/10200800/user/dir3*3", (FSMode) 0x666);
            Console::promptMessage(COLOR_BG_KO,"mkdir %s",FSAGetStatusStr(fserror));   // invald path
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/7","/vol/storage_usb01/usr/save/00050000/10200800/user/dir3*3/7:7");
            Console::promptMessage(COLOR_BG_KO,"special : in folder %s",FSAGetStatusStr(fserror));  // invalifpath
            mkdir("fs:/vol/external01/prova/3*3", 0666);
            Console::promptMessage(COLOR_BG_KO,"%s",strerror(errno)); // too long

            //invalid path, too long
            Console::promptMessage(COLOR_BG_OK,"?");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/4","/vol/storage_usb01/usr/save/00050000/10200800/user/4?4"); 
            Console::promptMessage(COLOR_BG_KO,"%s",FSAGetStatusStr(fserror));  // invalid path
            fserror = FSAMakeDir (handle, "/vol/storage_usb01/usr/save/00050000/10200800/user/dir4?4", (FSMode) 0x666);
            Console::promptMessage(COLOR_BG_KO,"mkdir %s",FSAGetStatusStr(fserror)); // invalid path
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/8","/vol/storage_usb01/usr/save/00050000/10200800/user/dir4?4/8:8");
            Console::promptMessage(COLOR_BG_KO,"special : in folder %s",FSAGetStatusStr(fserror)); // invalid path
            mkdir("fs:/vol/external01/prova/4?4", 0666);  // too long
            Console::promptMessage(COLOR_BG_KO,"%s",strerror(errno));  
            
            // ok, too long
            Console::promptMessage(COLOR_BG_OK,"\"");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/5","/vol/storage_usb01/usr/save/00050000/10200800/user/5\"5");
            Console::promptMessage(COLOR_BG_KO,"rename %s",FSAGetStatusStr(fserror)); // ok
            mkdir("storage_usb01:/usr/save/00050000/10200800/user/dir5\"5", 0666);
            Console::promptMessage(COLOR_BG_KO,"mkdir in usb %s",strerror(errno)); // illegal byte seq  >> but created!!
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/8","/vol/storage_usb01/usr/save/00050000/10200800/user/dir5\"5/8:8");  //
            Console::promptMessage(COLOR_BG_KO,"special : in folder %s",FSAGetStatusStr(fserror));   // ok?
            mkdir("fs:/vol/external01/prova/5\"5", 0666);
            Console::promptMessage(COLOR_BG_KO,"mkdir in SD %s",strerror(errno)); // too long

            // ok, too long
            Console::promptMessage(COLOR_BG_OK,"<");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/6","/vol/storage_usb01/usr/save/00050000/10200800/user/6<6");
            Console::promptMessage(COLOR_BG_KO,"rename %s",FSAGetStatusStr(fserror));  //
            mkdir("storage_usb01:/usr/save/00050000/10200800/user/dir6<6", 0666);
            Console::promptMessage(COLOR_BG_KO,"mkdir in usb %s",strerror(errno));  // too long  >> but created
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/9","/vol/storage_usb01/usr/save/00050000/10200800/user/dir6<6/9|9");
            Console::promptMessage(COLOR_BG_KO,"special | in foider %s",FSAGetStatusStr(fserror)); // OK
            mkdir("fs:/vol/external01/prova/6<6", 0666); 
            Console::promptMessage(COLOR_BG_KO,"mkdir in %s",strerror(errno)); // too long

            // ok, too long
            Console::promptMessage(COLOR_BG_OK,"|");
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/7","/vol/storage_usb01/usr/save/00050000/10200800/user/7|7");
            Console::promptMessage(COLOR_BG_KO,"rename %s",FSAGetStatusStr(fserror)); // ok
            mkdir("storage_usb01:/usr/save/00050000/10200800/user/dir7|7", 0666);  
            Console::promptMessage(COLOR_BG_KO,"mkdir in usb %s",strerror(errno)); // too long  >> but created
            fserror = FSARename(handle,"/vol/storage_usb01/usr/save/00050000/10200800/user/a","/vol/storage_usb01/usr/save/00050000/10200800/user/dir7|7/a<a");
            Console::promptMessage(COLOR_BG_KO,"special < in foider %s",FSAGetStatusStr(fserror));  // ok
            mkdir("fs:/vol/external01/prova/7|7", 0666);
            Console::promptMessage(COLOR_BG_KO,"%s",strerror(errno));  // too long
            */
        }


    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            //     if ( this->substateCalled == STATE_BACKUPSET_MENU) {
            //         slot = 0;
            //         getAccountsFromVol(&this->title, slot);
            //     }
            this->subState.reset();
            this->state = STATE_MAIN_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
