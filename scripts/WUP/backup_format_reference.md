# Backup Format Reference — `backup_saves.py` & `backup_miis.py`

Both scripts are minimal Python reimplementations of SaveMii's console→SD
backup, driven by `wupclient.py` (TCP to the `wupserver` homebrew, FSA-level
filesystem access). They reproduce SaveMii's on-SD layout and metadata **byte
for byte**, so backups made here appear in — and restore correctly through —
SaveMii's own UI.

- `backup_saves.py` → SaveMii `backupAllSave`: Wii U user + system saves, plus
  vWii saves.
- `backup_miis.py` → SaveMii `MiiRepo::backup`: the three internal Mii
  databases (FFL, RFL, ACCOUNT).

At the FSA layer the SD card is `/vol/storage_sdcard`; Cafe OS sees the same
tree as `sd:/wiiu/backups/...`.

## Two backup roots

| Kind  | Root | Constant |
|-------|------|----------|
| Saves | `/vol/storage_sdcard/wiiu/backups/batch/<datetime>/` | `SD_BATCH_ROOT` |
| Miis  | `/vol/storage_sdcard/wiiu/backups/mii_repos_bckp/<repo_folder>/` | `MII_BACKUP_ROOT` |

## Save backup folder structure

One timestamped **batch** folder per run (`now_date_for_folder()` →
`YYYY-MM-DDTHHMMSS`), one **title** folder per title, numbered **slots** inside:

```
batch/2026-07-25T143000/
├── savemiiMeta.json                        ← batch-root metadata (root_metadata)
├── <ShortName> [<hi>-<lo>]/                ← title_dir_name(): escaped name + hex id
│   ├── 0/                                   ← slot (find_empty_slot: first free 0..255)
│   │   ├── <copied save tree>              ← w.cpdir(src, dst): the /user or /data tree
│   │   ├── savemii_cfg_statInfo.out         ← permissions sidecar (per slot)
│   │   └── savemiiMeta.json                 ← per-slot metadata (slot_metadata)
│   └── 1/ ...
└── <AnotherTitle> [<hi>-<lo>]/ ...
```

- **Title folder name** — `title_dir_name()` = `setTitleNameBasedDirName`:
  FAT32-escaped shortname (`fat32_escape` maps non-ASCII and `\/:*?"<>|` → `_`),
  truncated to 235 bytes, plus `" [%08x-%08x]"` (hi-lo).
- **Slot selection** — `find_empty_slot()` picks the first non-existent
  `0..255` dir (`getEmptySlot`). USB storages are enumerated before NAND, so a
  title on both lands USB→slot 0, NAND→slot 1 (SaveMii's two-pass ordering).
- **Source trees** — Wii U: `/vol/<storage>/usr/save/<hi>/<lo>/user`;
  vWii: `/vol/storage_slccmpt01/title/<hi>/<lo>/data`.
- **Injects** — see the dedicated section below; the folder/metadata use the
  Wii U inject ids while the save is read from the vWii NAND.

## Injects

An **inject** is a Wii or GameCube game repackaged as a Wii U title (highID
`00050000`/`00050002`). It is installed on the Wii U side, but its **savedata
lives on the vWii NAND**, not under `usr/save` — so `enumerate_saves` never sees
it, and it needs its own pass (`backup_saves.py:717-739`). Detection and
classification follow SaveMii exactly.

**Detection** (`enumerate_injects`, `:578`) — a title under `usr/title/<hi>/<lo>`
(hi in `USER_HIGH_IDS`) is an inject **iff** it carries the vWii boot image
`code/fw.img`; a native Cafe title does not. Probed with `file_exists` (open +
close, no read — `fw.img` is multiple MB).

**Embedded vWii ids** (`resolve_meta`, `:388`) — the vWii **low** id is
`meta.xml`'s `<reserved_flag2>` (low 32 bits). Its four big-endian bytes give the
4-char **product code** (`vwii_inject_product_code`, `:172`; each signed byte
`< 32` → `.`, matching SaveMii's `char[]`). The vWii **high** id comes from
`find_inject_vwii_high` (`:594`): probe the three `VWII_HIGH_IDS` for
`title/<H>/<vwii_lo>` on the mounted SLCCMPT, preferring the `H` that also has a
`data` subdir (returns `has_data`). If not present, fall back to
`guess_vwii_high_id` (`:195`) — a faithful port **including** the original
`>> 16` quirk (which makes the 24-bit game-channel table effectively never match,
so titles fall through to the disc rule).

**Classification** (`is_gamecube_inject`, `:184`) — `product[0]` `G`/`D` →
GameCube; `P` → GameCube **only** at vWiiHighID `00010001` (a promotional GC
disc); otherwise a Wii inject.

**Handling** (per inject):

| Case | Action |
|------|--------|
| **GameCube inject** | **Never backed up** — GameCube saves are memory-card files owned by **Nintendont**, not on the NAND. Reported + named, appended to `gc_injects`. |
| **vWii NAND save unavailable** (SLC not mounted, or `has_data` false → no save initialised) | **Not backed up** — reported + named, appended to `vwii_injects_nonand`. |
| **vWii inject with a save** | Backed up. **Source** read from the NAND `title/<vWiiHi>/<vWiiLo>/data`; **folder + `savemiiMeta.json`** use the **Wii U inject** ids, with `vWiiHighID`/`vWiiLowID` set to the embedded pair, `storage=NAND`, format `FILES`. Uses the **vWii stat defaults** (`uid=0`, `gid=0x400`, `mode=0x666`), not the Wii U ones. |

Because the save is *read* under its vWii ids but *filed* under the Wii U inject
ids, SaveMii restores it straight back onto the installed inject through **Wii U
Save Management**, like any other Wii U title. The SLCCMPT is mounted once
(`mount_slc`, `:710`) and feeds both the inject pass and the native vWii pass.

**Reporting** — injects are counted in `ok["Inject"]`/`ko["Inject"]`; the two
not-backed-up lists are itemized by name at the end of the run (`:779-788`) and
folded into the batch-root tag as `inj:<n>,gc:<n>,inj-nonand:<n>` (`:756-758`).

## Mii backup folder structure

Flat — one fixed folder per repo, slots inside, **no stat sidecar** (SaveMii
disables it for Miis):

```
mii_repos_bckp/
├── mii_bckp_ffl/<slot>/       FFL_ODB.dat (+ stadio.sav), savemiiMeta.json   ← Wii U Mii DB
├── mii_bckp_rfl/<slot>/       RFL_DB.dat, savemiiMeta.json                    ← vWii Mii DB
└── mii_bckp_account/<slot>/   <act tree copied>, savemiiMeta.json            ← account Miis
```

FFL/RFL are **FILE** repos (individual files via `w.cp`); ACCOUNT is a
**FOLDER** repo (`w.cpdir` of `usr/save/system/act`). `find_mii_maker_id`
region-detects the FFL dir (`1004a000`/`100`/`200`) by probing for a real
`FFL_ODB.dat`.

After each internal repo is backed up, `replicate_custom_repo` also copies its
content into the matching **live custom (`_C`) repo** under a *different* root —
`mii_repos/` (no `_bckp`) — so the Miis show up as an importable SD repo. These
are a byte-identical copy of the internal DB with **no `savemiiMeta.json`**, at a
fixed path (no slot). If a custom repo already exists it is first **renamed** (atomic
`FSA_Rename`) to `mii_repo_<REPO>_C-<YYYY-MM-DDTHHMMSS>` — the batch-folder timestamp,
shared across all repos in the run — so previous custom Miis are preserved (never
deleted), and a loud banner lists the renamed folders at the end of the run
(`MiiUtils.cpp:80-94`):

```
mii_repos/
├── mii_repo_FFL_C/db/FFL_ODB.dat  (+ mii_repo_FFL_C/stadio.sav at repo root)   ← Wii U Mii DB
├── mii_repo_RFL_C/RFL_DB.dat      (flat — no db/ subdir)                        ← vWii Mii DB
└── mii_repo_ACCOUNT_C/            <act tree copied>                             ← account Miis
```

## Sidecar / metadata files SaveMii consumes

### `savemiiMeta.json` — visibility + identity

- **Batch root** (`root_metadata`, `backup_saves.py:175`): mostly-empty record;
  `Date` = folder name, `tag` = human summary. Makes the whole set appear in
  SaveMii's `BackupSetList`. Tag is generated at the end
  (`ALL OK - WU:x,vW:y,S:z titles`, or an OK/KO breakdown on failures).
- **Per-slot save** (`slot_metadata`, `:158`): `Date`, `serialId` (`_WIIU_`
  placeholder), `storage` (`USB`/`NAND`), `highID`/`lowID` (hex),
  `vWiiHighID`/`vWiiLowID` (= hi/lo for vWii, `0` for Wii U), `shortName`,
  `backupFormat: "FILES"`.
- **Per-slot Mii** (`mii_metadata`, `backup_miis.py:41`): `storage: "internal"`,
  `repoName` (`FFL`/`RFL`/`ACCOUNT`), plus `Date`/`serialId`/`tag`.

**Tag marker.** Every `tag` written by these scripts is prefixed through
`tag_with_marker()` (`backup_saves.py:212-217`) with `TAG_MARKER = "WUPS"` (WUP =
this `scripts/WUP/` toolset), so a backup's origin is identifiable: a tag with a
detail reads `"WUPS: <detail>"` (e.g. the batch-root
`WUPS: ALL OK - WU:x,vW:y,S:z titles…`), and an otherwise-empty tag (per-slot save
and Mii metadata) reads just `"WUPS"`. `backup_miis.py` imports the same helper,
so its Mii slots carry it too. SaveMii stores `tag` as an unbounded string, so
this never truncates or conflicts with SaveMii's own content.

`shortName` / title-folder name come from `resolve_meta()` (parses `meta.xml`
`shortname_en`, fallback `shortname_ja`, XML-decoded) or `resolve_vwii_name()`
(UTF-16BE title from `banner.bin` at offset 0x20). Both fall back to
`%08x%08x` hex.

### `savemii_cfg_statInfo.out` — permissions sidecar (saves only)

Lets restore reset IOSU **owner/group/mode** losslessly — a plain copy loses
those. Reproduces SaveMii's `StatManager` "exceptions file":

- Header `[STAT EXCEPTIONSFILE]\n` (`STAT_HEADER`).
- One line per entry whose perms differ from the category default
  (`is_stat_exception`), formatted
  `<type> <mode> <owner>:<group> <quota> <device-relative-path>`
  (`stat_line`, matching C `printf "%s %x %x:%x %llx %s"`).
- `build_stat_lines()` walks the copied tree (root dir + every descendant),
  reading perms via path-based `FSA_GetStat`. Because that command id isn't
  reliable on every wupserver, `verify_path_stat()` cross-checks it against
  `FSA_GetStatFile` on a sample file first; on mismatch it falls back to
  `FSA_ReadDir` entry stats (`use_path_stat=False`) and warns.
- Category defaults: Wii U `uid=lo`, `gid=group_id` (from meta), `mode=0x660`;
  vWii `uid=0` (SaveMii's runtime title-id, unknown offline), `gid=0x400`,
  `mode=0x666`. Only deviations get a line, keeping the file small.

## Restore flow (how SaveMii uses these)

1. Scans `batch/` and `mii_repos_bckp/`, reading each `savemiiMeta.json` to
   populate restore lists (batch-root for the set, per-slot for each entry).
2. Copies the slot's save tree back to `usr/save/.../user` (or vWii `.../data`).
3. Replays `savemii_cfg_statInfo.out`, re-applying owner/group/mode per path so
   IOSU permissions match the original — without which the console may reject or
   mishandle the restored save.

## Reliability details

- Slot-level transactions: on any exception the partial slot is `rmtree`'d
  (rollback) and the run continues; a failed batch-metadata write is tolerated
  so completed per-title backups aren't discarded.
- `flush_sd()` (`FSAFlushVolume`) commits to the card at the end.
- Copy buffer: both set `cp_block_size = 0x100000` (1 MB) with
  `cp_block_size_min = 0x40000` (256 KB fallback if the console heap can't fit
  1 MB); `wupclient.cp()` halves down to that floor on alloc failure.

## Known gaps vs the C original (`backup_saves.py:15`)

- No FAT32 filename-escape translation files.
- GameCube injects are reported but not backed up (Nintendont manages GC saves).
- `serialId` is the `_WIIU_` placeholder (real serial needs an MCP ioctl).
- vWii permission restore is inherently unreliable (owner defaults to 0).
