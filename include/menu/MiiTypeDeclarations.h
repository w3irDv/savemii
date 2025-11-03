#pragma once

namespace MiiStatus {

    enum eMiiState {
        NOT_TRIED,
        SKIPPED,
        OK,
        KO
    };

#define SELECTED      true
#define UNSELECTED    false
#define CANDIDATE     true
#define NOT_CANDIDATE false

    struct MiiStatus {
        MiiStatus(bool candidate, bool selected, eMiiState state) : candidate(candidate), selected(selected), state(state) {}
        bool candidate;
        bool selected;
        eMiiState state;
    };

} // namespace MiiStatus

namespace MiiProcess {

    enum eMiiProcessActions {
        SELECT_SOURCE_REPO,
        SELECT_TASK,
        BACKUP_DB,
        RESTORE_DB,
        WIPE_DB,
        LIST_MIIS,
        SELECT_IMPORT_REPO,
        SELECT_TRANSFORM_TASK,
        SELECT_MIIS_FOR_EXPORT,
        SELECT_MIIS_FOR_IMPORT,
        SELECT_MIIS_TO_BE_TRANSFORMED,
        SELECT_REPO_FOR_XFER_ATTRIBUTE,
        SELECT_TEMPLATE_MII_FOR_XFER_ATTRIBUTE,
        MIIS_TRANSFORMED
    };
}
