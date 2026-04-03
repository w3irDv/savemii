#pragma once

#include <ApplicationState.h>
#include <memory>
#include <savemng.h>
#include <utils/DataBin.h>
#include <utils/InputUtils.h>
#include <utils/LanguageUtils.h>
#include <vector>

class KeyListState : public ApplicationState {
public:
    explicit KeyListState() {
        if (!key_list_initialized) {
            if (DataBin::populate_key_list()) {
                keyFilesCount = DataBin::key_list.size();
                if (keyFilesCount > 0)
                    key_list_initialized = true;
                
            }
        }
    }

    enum eState {
        STATE_KEY_LIST,
        STATE_DO_SUBSTATE,
    };

    void render() override;
    ApplicationState::eSubState update(Input *input) override;

private:
    std::unique_ptr<ApplicationState> subState{};
    eState state = STATE_KEY_LIST;

    static inline bool key_list_initialized = false;
    static inline int keyFilesCount = 0;

    int scroll = 0;
    int cursorPos = 0;

    void moveDown(unsigned amount = 1, bool wrap = true);
    void moveUp(unsigned amount = 1, bool wrap = true);
};
