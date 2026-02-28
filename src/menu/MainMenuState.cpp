#include <BackupSetList.h>
#include <coreinit/debug.h>
#include <menu/BackupSetListState.h>
#include <menu/BatchTasksState.h>
#include <menu/ConfigMenuState.h>
#include <menu/MainMenuState.h>
#include <menu/MiiProcessSharedState.h>
#include <menu/MiiRepoSelectState.h>
#include <menu/MiiTypeDeclarations.h>
#include <menu/TitleListState.h>
#include <savemng.h>
#include <utils/Colors.h>
#include <utils/ConsoleUtils.h>
#include <utils/FSUtils.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <utils/MiiUtils.h>
#include <utils/StringUtils.h>
#include <utils/statDebug.h>

#include <segher-s_wii/segher.h>

#include <unistd.h>
#include <utils/AmbientConfig.h>

#define ENTRYCOUNT 6

#include <coreinit/filesystem_fsa.h>
#include <malloc.h>
bool setOwner(uint32_t owner, uint32_t group, FSMode mode, std::string path, FSError &fserror) {

    fserror = FSAChangeMode(FSUtils::handle, FSUtils::newlibtoFSA(path).c_str(), mode);
    if (fserror != FS_ERROR_OK) {
        Console::showMessage(ERROR_CONFIRM, _("Error\n%s\nsetting permissions for\n%s"), FSAGetStatusStr(fserror), path.c_str());
        return false;
    }

    FSAFlushVolume(FSUtils::handle, "/vol/storage_slcc01");

    fserror = FS_ERROR_OK;
    FSAShimBuffer *shim = (FSAShimBuffer *) memalign(0x40, sizeof(FSAShimBuffer));
    if (!shim) {
        Console::showMessage(ERROR_SHOW, _("Error creating shim for change perms\n\n%s"), path.c_str());
        return false;
    }

    shim->clientHandle = FSUtils::handle;
    shim->ipcReqType = FSA_IPC_REQUEST_IOCTL;
    strcpy(shim->request.changeOwner.path, FSUtils::newlibtoFSA(path).c_str());
    shim->request.changeOwner.owner = owner;
    shim->request.changeOwner.group = group;
    shim->command = FSA_COMMAND_CHANGE_OWNER;
    fserror = __FSAShimSend(shim, 0);
    free(shim);

    if (fserror != FS_ERROR_OK) {
        Console::showMessage(ERROR_CONFIRM, _("Error\n%s\nsetting owner/group for\n%s"), FSAGetStatusStr(fserror), path.c_str());
        return false;
    }

    return true;
}


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
        Console::consolePrintPosAligned(0, 4, 1, _("Main menu"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 0);
        Console::consolePrintPos(M_OFF, 2, _("   Wii U Save Management (%u Title%s)"), this->wiiuTitlesCount,
                                 (this->wiiuTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 1);
        Console::consolePrintPos(M_OFF, 3, _("   vWii Save Management (%u Title%s)"), this->vWiiTitlesCount,
                                 (this->vWiiTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 2);
        Console::consolePrintPos(M_OFF, 4, _("   Wii U System Titles Save Management (%u Title%s)"), this->wiiuSysTitlesCount,
                                 (this->wiiuSysTitlesCount > 1) ? "s" : "");
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 3);
        Console::consolePrintPos(M_OFF, 6, _("   Batch Tasks Management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 4);
        Console::consolePrintPos(M_OFF, 8, _("   BackupSet Management"));
        DrawUtils::setFontColorByCursor(COLOR_TEXT, COLOR_TEXT_AT_CURSOR, cursorPos, 5);
        Console::consolePrintPos(M_OFF, 10, _("   Mii Management"));

        DrawUtils::setFontColor(COLOR_TEXT);
        Console::consolePrintPos(M_OFF, 2 + cursorPos + (cursorPos > 2 ? 1 : 0) + (cursorPos > 3 ? 1 : 0) + (cursorPos > 4 ? 1 : 0), "\u2192");
        Console::consolePrintPosAligned(17, 4, 2, _("\\ue002: Options \\ue000: Select Mode"));
    }
}


ApplicationState::eSubState MainMenuState::update(Input *input) {
    if (this->state == STATE_MAIN_MENU) {
        if (input->get(ButtonState::TRIGGER, Button::A)) {
            std::vector<bool> mii_repos_candidates;
            switch (cursorPos) {
                case 0:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiiutitles, this->wiiuTitlesCount, IS_WIIU);
                    break;
                case 1:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiititles, this->vWiiTitlesCount, IS_VWII);
                    break;
                case 2:
                    if (showSystemTitlesManagementDisclaimer) {
                        if (!Console::promptConfirm(ST_WARNING, _("This task allows you to backup/restore savedata for MiiMaker, Internet Browser and other system titles.\n\nProceed with caution and be aware of what you are modifying! Some saves can affect system behavior, and misuse on them can have unexpected consequences!\n\nDo you want to continue?\n\n")))
                            return SUBSTATE_RUNNING;
                        else
                            showSystemTitlesManagementDisclaimer = false;
                    }
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<TitleListState>(this->wiiusystitles, this->wiiuSysTitlesCount, IS_WIIU);
                    break;
                case 3:
                    this->state = STATE_DO_SUBSTATE;
                    this->subState = std::make_unique<BatchTasksState>(this->wiiutitles, this->wiititles, this->wiiusystitles, this->wiiuTitlesCount, this->vWiiTitlesCount, this->wiiuSysTitlesCount);
                    break;
                case 4:
                    this->state = STATE_DO_SUBSTATE;
                    this->substateCalled = STATE_BACKUPSET_MENU;
                    this->subState = std::make_unique<BackupSetListState>();
                    break;
                case 5:
                    this->state = STATE_DO_SUBSTATE;
                    if (FSUtils::checkEntry(MiiUtils::MiiRepos["RFL"]->path_to_repo.c_str()) != 1)
                        MiiUtils::ask_if_to_initialize_db(MiiUtils::MiiRepos["RFL"], DB_NOT_FOUND);
                    if (FSUtils::checkEntry(MiiUtils::MiiRepos["FFL"]->path_to_repo.c_str()) != 1)
                        MiiUtils::ask_if_to_initialize_db(MiiUtils::MiiRepos["FFL"], DB_NOT_FOUND);
                    MiiUtils::initial_checkpoint();
                    for (size_t i = 0; i < MiiUtils::mii_repos.size(); i++)
                        mii_repos_candidates.push_back(true);
                    this->subState = std::make_unique<MiiRepoSelectState>(mii_repos_candidates, MiiProcess::SELECT_SOURCE_REPO, &MiiUtils::mii_process_shared_state);
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
        if (input->get(ButtonState::HOLD, Button::PLUS) && input->get(ButtonState::HOLD, Button::L)) {
            //char *arg[] = {(char *) "just to compile"};
            //pack(1, arg);
            //unpack(1, arg);
            //statDebugUtils::statVol();
            //const std::string pathrfl("storage_slcc01:/shared2/menu/FaceLib/nRFL_DB.dat");
            //FSError fserror;
            //setOwner(0x1000400A,0x400, (FSMode) 0x666, pathrfl,fserror);
            //statDebugUtils::statMiiEdit();
            //statDebugUtils::statMiiMaker();
            //statDebugUtils::statAct();
            /*
            Console::showMessage(ERROR_CONFIRM, "device_hash %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                                 AmbientConfig::device_hash[0], AmbientConfig::device_hash[1], AmbientConfig::device_hash[2], AmbientConfig::device_hash[3], AmbientConfig::device_hash[4],
                                 AmbientConfig::device_hash[5], AmbientConfig::device_hash[6], AmbientConfig::device_hash[7]);

            Console::showMessage(ERROR_CONFIRM, "mac:%02x:%02x:%02x:%02x:%02x:%02x",
                                 AmbientConfig::mac_address.MACAddr[0], AmbientConfig::mac_address.MACAddr[1], AmbientConfig::mac_address.MACAddr[2],
                                 AmbientConfig::mac_address.MACAddr[3], AmbientConfig::mac_address.MACAddr[4], AmbientConfig::mac_address.MACAddr[5]);
            */
            FSError fserror;
            Console::showMessage(WARNING_SHOW, "creating non-fat32 files");
            std::string path = "storage_slcc01:/title/00010000/534d4e50/data/file_ko";
            FILE *fp = fopen(path.c_str(), "w");
            if (fp != nullptr) {
                fprintf(fp, "Prova\n");
                fclose(fp);
                FSUtils::copyFile("storage_slcc01:title/00010000/534d4e50/data/file_ko", "storage_slcc01:title/00010000/534d4e50/data/file:ko");
                unlink("storage_slcc01:/title/00010000/534d4e50/data/file_ko");
            } else {
                Console::showMessage(ERROR_CONFIRM, "%s %s", path.c_str(), strerror(errno));
            }
            FSUtils::flushVol(path);
            path = "storage_usb01:/usr/save/00050000/10101e00/user/8000000b/file_complain";
            fp = fopen(path.c_str(), "w");
            if (fp != nullptr) {
                fprintf(fp, "Prova\n");
                fclose(fp);
                fserror = FSARename(FSUtils::handle, "/vol/storage_usb01/usr/save/00050000/10101e00/user/8000000b/file_complain", "/vol/storage_usb01/usr/save/00050000/10101e00/user/8000000b/file:complain");
                Console::showMessage(ERROR_CONFIRM, "%s %s", FSAGetStatusStr(fserror), path.c_str());
            } else {
                Console::showMessage(ERROR_CONFIRM, "%s %s", path.c_str(), strerror(errno));
            }
            path = "storage_usb01:/usr/save/00050000/1010ed00/user/common/file_complain";
            fp = fopen(path.c_str(), "w");
            if (fp != nullptr) {
                fprintf(fp, "Prova\n");
                fclose(fp);
                fserror = FSARename(FSUtils::handle, "/vol/storage_usb01/usr/save/00050000/1010ed00/user/common/file_complain", "/vol/storage_usb01/usr/save/00050000/1010ed00/user/common/file<complain");
                Console::showMessage(ERROR_CONFIRM, "%s %s", FSAGetStatusStr(fserror), path.c_str());
            } else {
                Console::showMessage(ERROR_CONFIRM, "%s %s", path.c_str(), strerror(errno));
            }
            path = "storage_usb01:/usr/save/00050000/10184e00/user/common/file_complain";
            fp = fopen(path.c_str(), "w");
            if (fp != nullptr) {
                fprintf(fp, "Prova\n");
                fclose(fp);
                fserror = FSARename(FSUtils::handle, "/vol/storage_usb01/usr/save/00050000/10184e00/user/common/file_complain", "/vol/storage_usb01/usr/save/00050000/10184e00/user/common/file<complain");
                Console::showMessage(ERROR_CONFIRM, "%s %s", FSAGetStatusStr(fserror), path.c_str());
            } else {
                Console::showMessage(ERROR_CONFIRM, "%s %s", path.c_str(), strerror(errno));
            }

            FSUtils::flushVol(path);

            return SUBSTATE_RUNNING;
        }
        if (input->get(ButtonState::HOLD, Button::MINUS) && input->get(ButtonState::HOLD, Button::L)) {
            unlink("storage_slcc01:/title/00010000/534d4e50/data/file:ko");
            unlink("storage_usb01:/usr/save/00050000/10101e00/user/8000000b/file:complain");
            unlink("storage_usb01:/usr/save/00050000/1010ed00/user/common/file<complain");
            unlink("storage_usb01:/usr/save/00050000/10184e00/user/common/file<complain");
            return SUBSTATE_RUNNING;
        } 
    } else if (this->state == STATE_DO_SUBSTATE) {
        auto retSubState = this->subState->update(input);
        if (retSubState == SUBSTATE_RUNNING) {
            // keep running.
            return SUBSTATE_RUNNING;
        } else if (retSubState == SUBSTATE_RETURN) {
            //     if ( this->substateCalled == STATE_BACKUPSET_MENU) {
            //         slot = 0;
            //         AccountUtils::getAccountsFromVol(&this->title, slot);
            //     }
            this->subState.reset();
            this->state = STATE_MAIN_MENU;
        }
    }
    return SUBSTATE_RUNNING;
}
