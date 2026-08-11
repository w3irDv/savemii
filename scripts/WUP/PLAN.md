# Minimal `backupAllSave` in Python (console → SD)

## Context
SaveMii's `backupAllSave` (`src/savemng.cpp:686`) batch-copies each title's save
folder from console storage to a timestamped folder on the SD card. This is a
**minimal standalone Python reimplementation** (`backup_saves.py`) driven by
`wupclient.py` (talks to the wupserver homebrew over TCP), that:
- enumerates saves by **directory scan** (no title DB),
- names each backup folder with SaveMii's **escaped title-name** convention,
- writes **SaveMii-compatible metadata** so the backups appear in SaveMii's
  BackupSetList and per-slot listings,
- handles **NAND + USB** with USB-before-NAND ordering (a dupe lands USB→slot 0,
  NAND→slot 1, matching SaveMii).

Out of scope (documented limitations): FAT32 filename-escape translation files,
`saveinfo.xml` copy, and backing up **GameCube injects** (their saves belong to
Nintendont — they are still detected and reported). Wii U user + system saves,
vWii saves, and **vWii injects** (Wii games repackaged as Wii U titles, saves on
the vWii NAND) are all handled.

Permission metadata **is** reproduced: each slot gets SaveMii's StatManager
sidecar (`savemii_cfg_statInfo.out`) so a SaveMii restore resets IOSU
owner/group/mode losslessly (see "StatManager sidecar" below).

## Reference behavior (verified in source)
- **Source** save dir (FSA layer): `/vol/<storage>/usr/save/<hi>/<lo>/user`
  where `<storage>` = `storage_mlc01` (NAND) or `storage_usb01`/`usb02` (USB).
  The whole `user/` is copied (per-account dirs + `common`).
- **Dest** (SaveMii `fs:/vol/external01/...` = FSA `/vol/storage_sdcard/...`):
  `/vol/storage_sdcard/wiiu/backups/batch/<datetime>/<dirName>/<slot>`
  (`batchBackupPath`, `getBatchBackupPath` → `savemng.cpp:32,107`).
- **datetime** folder = `getNowDateForFolder()` = `%Y-%m-%dT%H%M%S`, e.g.
  `2026-07-19T101500`.
- **Escaped title name** (`setTitleNameBasedDirName` `TitleUtils.cpp:896`,
  `convertToFAT32ASCIICompliant` `EscapeFAT32Utils.cpp:248`, `forbidden` = `\/:*?"<>|`):
  1. `shortName` from `meta.xml` field `shortname_en` (fallback `shortname_ja`),
     XML-decoded (`&quot;&apos;&lt;&gt;&amp;`), text between `>`…`<`;
     empty → `"%08x%08x" % (hi, lo)`.
  2. escape: each non-ASCII codepoint → `_`; ASCII in `\/:*?"<>|` → `_`; else keep;
     strip **leading** spaces.
  3. `dirName = escaped[:235] + " [%08x-%08x]" % (hi, lo)`
     (235 = FILENAME_BUFF_SIZE 256 − ID_STR_BUFF_SIZE 21).
- **meta.xml** path (`TitleUtils.cpp:184`): prefer installed-title meta
  `/vol/<storage>/usr/title/<hi>/<lo>/meta/meta.xml`, else save meta
  `/vol/<storage>/usr/save/<hi>/<lo>/meta/meta.xml`.
- **Per-slot metadata** `<dstPath>/savemiiMeta.json` (`Metadata::write` +
  `writeMetadata(..., FILES)` `Metadata.cpp:123`, `savemng.cpp:656`), keys in order:
  `Date, serialId, storage, tag, highID, lowID, vWiiHighID, vWiiLowID, shortName, backupFormat`.
  - `Date` = `getNowDate()` = `%d/%m/%Y %H:%M`; `serialId` = `_WIIU_`
    (SaveMii's `unknownSerialId`; real value needs an MCP `GetSysProdSettings`
    ioctl not in wupclient — optional add-on); `storage` = `"USB"`/`"NAND"`;
    `tag` = `""`; ids `%08x` (vWii ids `00000000`); `shortName` = resolved
    (un-escaped) name; `backupFormat` = `"FILES"`.
- **Batch-root metadata** `<batchroot>/savemiiMeta.json` (`writeBackupAllMetadata`,
  Metadata ctor 4 `Metadata.h:45`): same keys; `Date` = the `<datetime>` folder
  string, `storage` = `""`, ids/shortName/backupFormat empty. This makes the set
  appear in SaveMii's BackupSetList. The `tag` mirrors SaveMii's summary shown in
  that list (`BatchBackupState.cpp:154-169`), split by category:
  `"ALL OK - WU:<u>,vW:<v>,S:<s> titles"` or
  `"OK/KOs > WU:<ok>/<ko>,vW:<ok>/<ko>,S:<ok>/<ko>"`.

## Title categories (user vs system)
The dir scan enumerates every highID under `usr/save`, which already covers both
SaveMii Wii U lists (all under the same path): **user** titles
`{0x00050000, 0x00050002}` (`loadWiiUTitles`) and **system** titles
`{0x00050010, 0x00050030}` (`loadWiiUSysTitles`). Each save is classified via
`is_system_title(hi)`; counts are tracked per category and surfaced in the
per-title log (`[NAND/System] …`), the final summary, and the root tag above.
System-title save data was always copied — this just makes the handling explicit
and reported, matching SaveMii's separate wiiutitles/wiiusystitles passes.

## vWii (SLCCMPT) saves
A third pass backs up vWii saves, mirroring SaveMii's `wiititles` backup:
- Mount `/dev/slccmpt01` → `/vol/<SLC_STORAGE>` (`mount_slc`), then enumerate
  `title/<hi>/<lo>` with a `data` subdir (`enumerate_vwii_saves`). Source is the
  `data` folder (not `user`); device-relative stat paths become `/title/…/data/…`.
- **Name** comes from the Wii save **banner** (`data/banner.bin`), UTF-16BE title,
  32 code units at offset 0x20 (`TitleUtils.cpp:440`), not `meta.xml`;
  hex fallback if absent.
- **Metadata**: `highID/lowID` and `vWiiHighID/vWiiLowID` all = the scanned
  `hi/lo`; `storage="NAND"` (SaveMii writes `isUSB=false`).
- **Stat defaults** differ (`set_default_stat_cfg_for_vwii_savedata`):
  `{uid=savemii_title_id, gid=0x400, mode=0x666}`. `savemii_title_id` is the
  running SaveMii app's title-id low word (`OSGetTitleID`), unknown offline → `0`;
  SaveMii itself notes vWii permission restore is unreliable, so this is
  best-effort. Category **vWii** feeds the `vW` counts/tag.

## Injects (Wii/GameCube games repackaged as Wii U titles)
An **inject** is installed as a Wii U title (highID `00050000`/`00050002`) but
its savedata lives on the **vWii NAND**, not `usr/save` — so the normal save scan
misses it. A dedicated pass mirrors SaveMii's inject handling:
- **Detect** (`enumerate_injects`): a `usr/title/<hi>/<lo>` (hi in
  `USER_HIGH_IDS`) is an inject **iff** it has `code/fw.img` (the vWii boot image;
  a native Cafe title does not). Probed with `file_exists` (open/close, no read).
- **Embedded vWii ids** (`resolve_meta`): vWii **low** id = `meta.xml`
  `<reserved_flag2>`; its 4 big-endian bytes → 4-char **product code**
  (`vwii_inject_product_code`, signed byte `< 32` → `.`). vWii **high** id from
  `find_inject_vwii_high` (probe the three `VWII_HIGH_IDS` for
  `title/<H>/<vwii_lo>`, prefer the `H` that has `data`), else
  `guess_vwii_high_id` (faithful port incl. the original `>> 16` quirk).
- **Classify** (`is_gamecube_inject`): `product[0]` `G`/`D` → GameCube; `P` →
  GameCube only at vWiiHighID `00010001`; else Wii.
- **Handle**:
  - **GameCube inject** → **never backed up** (GC saves are Nintendont
    memory-card files, not on the NAND); reported + named (`gc_injects`).
  - **vWii inject, NAND save unavailable** (SLC unmounted or no `data`) → not
    backed up; reported + named (`vwii_injects_nonand`).
  - **vWii inject with a save** → source read from NAND
    `title/<vWiiHi>/<vWiiLo>/data`, but folder + `savemiiMeta.json` use the **Wii U
    inject** ids (with `vWiiHighID/vWiiLowID` = embedded pair, `storage=NAND`,
    format FILES) and the **vWii stat defaults** (`uid=0, gid=0x400, mode=0x666`).
    SaveMii then restores it onto the installed inject via Wii U Save Management.
- The SLCCMPT is mounted once (`mount_slc`) and feeds both the inject pass and the
  native vWii pass. Injects feed the `Inject` counts and the batch-root tag's
  `inj:<n>,gc:<n>,inj-nonand:<n>` suffix; the two not-backed-up lists are itemized
  at the end of the run.

## Implementation — `backup_saves.py`
Imports `wupclient`. Structure:
- **Pure helpers** (offline-testable): `fat32_escape`, `title_dir_name`,
  `now_date`/`now_date_for_folder`, `decode_xml`, `slot_metadata`/`root_metadata`.
- **Console helpers** on a `wupclient` instance: `dir_exists` (FSA_OpenDir),
  `makedirs` (mkdir -p; wupclient `mkdir` is single-level and `cpdir` doesn't
  create its own root), `read_file_quiet`, `write_bytes`/`write_json`,
  `mount_sd`, `flush_mlc`, `resolve_short_name`, `list_dirs`, `enumerate_saves`,
  `find_empty_slot`.
- **`backup_all_saves(w, datetime_folder, tag="")`**: mount SD → make batch root
  → build storage list (USB volumes probed first, then NAND) → for each save
  (pair `<hi>/<lo>` with a `user/` subdir): resolve name, build escaped
  `dirName`, pick first free slot, `makedirs(dst)`, `w.cpdir(src, dst)`, write
  per-slot `savemiiMeta.json`; then write root `savemiiMeta.json`, flush NAND,
  print `ok/failed` summary.
- **`__main__`**: `ip = sys.argv[1]`; `w = wupclient(ip)`;
  `backup_all_saves(w, now_date_for_folder())`.

Reuses existing wupclient methods: `cpdir`, `silent_ls`, `FSA_OpenDir/CloseDir`,
`FSA_OpenFile/ReadFile/WriteFile/CloseFile`, `FSA_MakeDir`, `FSA_Mount`,
`FSA_FlushVolume`, `open`/`close`, `get_fsa_handle`.

## StatManager sidecar (`savemii_cfg_statInfo.out`)
Each slot gets SaveMii's permission-exceptions file so restores are lossless.

- **Permission source — path-based `FSA_GetStat` (authoritative).** wut docs
  (`filesystem_fsa.h`) confirm `FSAGetStat(client, path, FSAStat*)` is the
  canonical per-path stat for files AND dirs — exactly what SaveMii's `get_stat`
  uses. `FSA_ReadDir`'s entry stat (`unk`) is *not* guaranteed to populate
  owner/group/mode, so we no longer rely on it. Added `wupclient.FSA_GetStat`
  (GetInfoByQuery ioctl `0x18`, query `FSA_QUERY_INFO_STAT=0x5`; request path@0x4,
  type@0x284; response `FSAStat` at output[4:]). `build_stat_lines` reads each
  entry (incl. the `user`/`data` root) via `FSA_GetStat`; `silent_ls` is used only
  to enumerate names + file/dir.
- **Command-id safety + fallback.** The raw GetInfoByQuery id can't be confirmed
  from the repo, so `verify_path_stat` cross-checks `FSA_GetStat` against the
  known-good `FSA_GetStatFile` (0x14) on one real file at startup. On mismatch it
  warns and falls back to the readdir-`unk` stat (`build_stat_lines(...,
  use_path_stat=False)`) — best-effort rather than failing.
- **Format** (`StatManager.cpp:236-279`): line 1 `[STAT EXCEPTIONS FILE]\n`, then
  per exception entry `"%s %x %x:%x %llx %s\n"` =
  `<type> <mode> <owner>:<group> <quota> <path_rel>`, where `type` composes
  `D`(0x80000000)/`Q`(0x60000000)/`F`(0x01000000)/`E`(0x00800000)/`L`(0x00010000)
  and `path_rel` = FSA path minus `/vol/<storage>` → `/usr/save/<hi>/<lo>/user…`.
- **Filter**: record an entry only if `(mode,owner,group)` ≠ default
  `{uid=lowID, gid=groupID, mode=0x660}` (`set_default_stat_cfg_for_wiiu_savedata`).
  `groupID` parsed from `meta.xml` `group_id` (0 if absent).
- Helpers: `resolve_meta`→`(short_name, group_id)`, `parse_stat(statbytes)`,
  `entry_type(flags)`, `stat_line(...)`, `build_stat_lines(..., use_path_stat)`,
  `verify_path_stat`/`find_sample_file`; sidecar written right after
  `w.cpdir(src, dst)`. `MockWupclient` gained `FSA_GetStat` + `FSA_GetStatFile`
  (synthesized from the same `_make_unk`, so the self-check passes offline).

## Error handling & rollback
A read/copy failure for one title must not corrupt the set or abort the run:
- `wupclient.cp` now **raises** `IOError` on any FSA read/write error instead of
  treating the error code as a byte count (the old "error-code-as-count" bug that
  could silently truncate or loop). It also fixes the EOF test (break on the read
  count) and always frees the buffer / closes both handles via `finally`.
- `wupclient.cpdir` raises if a directory can't be opened (so a failed listing
  can't masquerade as an empty dir → silent empty copy). `MockWupclient.cpdir`
  already raises via `shutil.copytree`.
- `backup_all_saves` wraps each title in a try/except: on failure it prints the
  error, **rolls back** the partial slot via `w.rmtree(dst)` (new recursive delete
  on both clients — FSA `FSA_Remove` walk / `shutil.rmtree`), counts the title
  KO, and continues. The run still finishes and writes the `OK/KOs > …` root tag.

## Verification
- `python3 -m py_compile backup_saves.py` — clean.
- Offline unit checks (no console): `fat32_escape` (accents/forbidden/leading
  spaces/astral), `title_dir_name` (235-char truncation), `decode_xml` (`&amp;`
  resolved last), `slot_metadata`/`root_metadata` key order + values, plus
  `parse_stat`/`entry_type`/`stat_line`/default-filter/path-strip — all passing.
- End-to-end (needs a Wii U running wupserver + SD): `python3 backup_saves.py
  <ip>`; confirm `wiiu/backups/batch/<datetime>/<escaped name [ids]>/0/` holds the
  save files + `savemiiMeta.json` + `savemii_cfg_statInfo.out` (with the `common`
  dir as a `DQ` line, `gid=400`, non-zero quota), root `savemiiMeta.json` exists,
  and SaveMii's Batch Restore lists and restores the set with no "not reliable"
  permission warning. Dupe USB+NAND title → USB slot 0, NAND slot 1.

## Local testing without a console (`mock_wupclient.py` + `run_mock_backup.py`)
`backup_all_saves(w, …)` takes the client as a parameter, so a filesystem-backed
mock can drive the real backup logic on a PC — **no changes to `backup_saves.py`**.
- `MockWupclient` implements the wupclient method surface against a Linux tree,
  mapping FSA volumes to dirs under a root (default `/mnt/c/Users/Xavier/repo/cemu`):
  `storage_mlc01`→`<root>/mlc01`, `storage_sdcard`→`<root>/SD`,
  `storage_usb01/02`→`<root>/usb01/02`, `storage_slccmpt01`→`<root>/slccmpt01`
  (absent → skipped).
- A Cemu NAND copy has no Wii U owner/group/mode/quota, so `silent_ls` synthesizes
  each entry's `FSAStat` (`unk`): flags from dir/file, and by default the SaveMii
  default for that save type — Wii U `mode=0x660, owner=lowID, group=group_id`;
  vWii `mode=0x666, owner=savemii_title_id(0), group=0x400` — so nothing is a stat
  exception (near-empty sidecar). An optional `mock_stat.json` overlay
  (`{device_rel_path: {mode,owner,group,quota}}`) injects exceptions (e.g. a
  `common` dir as `DQ`/`gid=0x400`/quota) to exercise sidecar recording.
- Run: `python3 run_mock_backup.py [root]` → writes a real batch backup into
  `<root>/SD/wiiu/backups/batch/<datetime>/` for inspection.
- `test_mock_backup.py` — self-checking end-to-end test (exit non-zero on
  failure). Writes to throwaway temp SD/USB dirs (mlc01 stays read-only) and
  asserts: escaped dir/slot layout, per-slot + root `savemiiMeta.json` fields,
  the overlaid `common` `DQ` line in the sidecar, byte-identical `save.dat` copy,
  and — via a USB fixture that duplicates a NAND title — that USB lands in slot 0
  (USB-marker data) and NAND in slot 1 (real data). Further scenarios cover system
  titles (highID `00050010` → `S` tag) and a synthetic vWii SLC fixture (banner.bin
  name, `data` tree, vWii metadata ids, `vW` tag). An optional scenario E backs up
  **real** Wii disc-game saves from a Dolphin NAND (`$DOLPHIN_WII`, skipped if
  absent) — validated live: banner names "Tales of Symphonia" / "New SUPER MARIO
  BROS. Wii" parsed, save files copied byte-identical. A rollback scenario forces
  a mid-copy failure and asserts the partial slot is removed, the title is counted
  failed (`OK/KOs` tag), and the run completes without crashing. 30 checks total.

---

# Mii repo backup — `backup_miis.py`

A separate tool backs up SaveMii's three **internal Mii repos** (Mii databases),
each to its first free slot, mirroring `MiiRepo::backup` (`src/mii/MiiRepo.cpp`).
Backups live under a different root than saves:
`/vol/storage_sdcard/wiiu/backups/mii_repos_bckp/<backup_folder>/<slot>/`
(`MiiRepo.h:90`). No stat sidecar (SaveMii calls `disable_all_flags` for Miis).

| repo | kind | source (FSA) | backup_folder |
|------|------|--------------|---------------|
| FFL | file(s) | `storage_mlc01/usr/save/00050010/<miimaker>/user/common/db/FFL_ODB.dat` (+ `…/common/stadio.sav`) | `mii_bckp_ffl` |
| RFL | file | `storage_slccmpt01/shared2/menu/FaceLib/RFL_DB.dat` | `mii_bckp_rfl` |
| ACCOUNT | folder | `storage_mlc01/usr/save/system/act` | `mii_bckp_account` |

- **Region:** the Mii Maker id is region-specific (`1004a000` JPN / `1004a100`
  USA / `1004a200` EUR). `find_mii_maker_id` auto-detects it — the `1004a?00`
  dir under `usr/save/00050010` that actually holds an `FFL_ODB.dat` — so no MCP
  region query is needed and stale multi-region dirs are disambiguated.
- **Slot:** first free `<backup_folder>/<slot>` (`getEmptySlot`), per repo.
- **Metadata:** `<slot>/savemiiMeta.json` via `Metadata::write_mii` —
  `{Date, serialId="_WIIU_", storage="internal", tag="", repoName}`.
- **Copy:** FILE repos use `w.cp` (added `MockWupclient.cp` = `shutil.copyfile`);
  ACCOUNT uses `w.cpdir`. Reuses `backup_saves.py` helpers by import. Same
  per-repo try/except + `w.rmtree(dst)` rollback as the save tool.

**Verification** (`test_mock_miis.py`, real data: Cemu mlc01 + Dolphin Wii NAND,
temp SD): FFL (`FFL_ODB.dat`+`stadio.sav`), RFL (`RFL_DB.dat`, 779968 B), ACCOUNT
(whole `act/` tree) all copied byte-identical with correct `repoName`/`storage`
metadata; slot increments on a second run; a forced `cp` failure rolls back the
slot. 11 checks, all passing.
