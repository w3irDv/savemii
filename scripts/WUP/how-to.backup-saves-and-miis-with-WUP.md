# How to back up saves and Miis using wupserver — `backup_saves.py` & `backup_miis.py`

Two standalone tools that back up a Wii U console's savedata and Mii databases to
its SD card, in SaveMii's on-disk format, over the network. They talk to
**wupserver** running on the console (TCP port `1337`) via `wupclient.py`;
nothing runs on the console beyond wupserver itself.

- `backup_saves.py` — Wii U user + system saves, vWii saves, and injects.
- `backup_miis.py` — the three internal Mii databases (FFL, RFL, ACCOUNT).

For what each script writes and why, see
[`backup_format_reference.md`](backup_format_reference.md); for the design notes,
[`PLAN.md`](PLAN.md).

## Prerequisites

**Get the scripts.** You need `wupclient.py`, `backup_saves.py` and
`backup_miis.py` together in one folder. Either:

- **Option 1 — clone the repo:**
  ```bash
  git clone https://github.com/w3irDv/savemii.git
  cd savemii/scripts/WUP
  ```
- **Option 2 — download just these files** into the same folder:
  [`wupclient.py`](https://raw.githubusercontent.com/w3irDv/savemii/main/scripts/WUP/wupclient.py),
  [`backup_saves.py`](https://raw.githubusercontent.com/w3irDv/savemii/main/scripts/WUP/backup_saves.py),
  [`backup_miis.py`](https://raw.githubusercontent.com/w3irDv/savemii/main/scripts/WUP/backup_miis.py).

Then:

1. **Python 3** on your PC. Only the standard library is used — nothing to
   install. Run from this `scripts/WUP/` folder so the imports resolve
   (`wupclient`, and `backup_miis.py`'s reuse of `backup_saves.py`).
2. **wupserver running on the console.** Launch wupserver through the
   [RecoveryMenu](https://github.com/GaryOderNichts/recovery_menu) and leave it on
   the "waiting for connection" screen. It listens on TCP **1337**.
3. **Same network.** PC and console on the same LAN, no firewall blocking 1337.
4. **The console's IP address.** wupserver prints it on screen; you can also find
   it in *System Settings → Internet*. You pass it to each script.
5. **An SD card in the console** with enough free space. Both tools mount it and
   write under `wiiu/backups/`.

> These are read-only against console storage (they only *copy from* NAND/USB/SLC
> and *write to* the SD card), but always work from a console you can afford to
> experiment with, and don't interrupt a run mid-copy.

## Running

Each tool takes one argument — the console IP:

```bash
# from scripts/WUP/
python3 backup_saves.py <console-ip>     # e.g. 192.168.1.38
python3 backup_miis.py  <console-ip>
```

The two are independent and each mounts the SD card itself, so you can run either
alone or both. To capture everything, run **saves first, then Miis**:

```bash
python3 backup_saves.py 192.168.1.38
python3 backup_miis.py  192.168.1.38
```

Each run flushes the SD volume at the end, so it's safe to eject once the script
prints its final `done:` line.

### `backup_saves.py`

Backs up, into one timestamped batch folder
(`wiiu/backups/batch/<YYYY-MM-DDTHHMMSS>/`):

- **Wii U saves** — every `usr/save/<hi>/<lo>` with a `user/` tree, from **USB
  first, then NAND** (so a title on both lands USB→slot 0, NAND→slot 1). User and
  system titles are handled and counted separately.
- **vWii saves** — from the SLCCMPT (Wii NAND), named from each save's
  `banner.bin`.
- **Injects** (Wii/GameCube games repackaged as Wii U titles) — vWii injects are
  backed up from the NAND under their Wii U inject ids; **GameCube injects are
  reported but not backed up** (their saves belong to Nintendont). Any inject
  whose vWii NAND save is unavailable is reported too. See the *Injects* section
  of `backup_format_reference.md`.

Each save slot also gets a `savemii_cfg_statInfo.out` permissions sidecar so
SaveMii restores owner/group/mode losslessly.

Typical output:

```
mounting SD card ...
permissions: FSA_GetStat verified (real owner/group/mode)

[NAND/WiiU] Super Mario 3D World  (00050000-10145d00) -> slot 0
...
GAMECUBE INJECT (cannot be managed - restore its save from Nintendont): ...
done: WiiU 40/0, vWii 8/0, System 14/0, Inject 2/0 (ok/fail)  ->  .../batch/2026-08-09T...
  GameCube injects NOT backed up (restore from Nintendont): 1
      - Super Smash Bros. Melee [00050002-...]
elapsed: 1m 12s
```

### `backup_miis.py`

Backs up the three internal Mii repos, each to its first free slot under
`wiiu/backups/mii_repos_bckp/`:

| repo | what | source |
|------|------|--------|
| **FFL** | Wii U Mii database (Mii Maker), + `stadio.sav` | NAND `usr/save/00050010/<region>/…` (region auto-detected) |
| **RFL** | vWii Mii database | SLCCMPT `shared2/menu/FaceLib/RFL_DB.dat` |
| **ACCOUNT** | account Miis (whole `act` tree) | NAND `usr/save/system/act` |

Any repo that doesn't exist on the console is skipped with a note (e.g. no vWii →
RFL skipped). No stat sidecar is written for Miis (SaveMii disables it).

After backing up each repo, it also **replicates it into the live custom (`_C`)
repo** under `wiiu/backups/mii_repos/`, so the Miis appear as an importable SD
repo in SaveMii. If a `_C` repo already exists it is **renamed** with a timestamp
suffix (never deleted) and a banner at the end of the run lists the renamed
folders — rename one back (drop the `-<timestamp>`) to use it again.

## Where the backups land

On the SD card (Cafe OS sees this as `sd:/wiiu/backups/…`):

```
wiiu/backups/
├── batch/<YYYY-MM-DDTHHMMSS>/        ← backup_saves.py (one folder per run)
│   ├── savemiiMeta.json
│   └── <ShortName> [<hi>-<lo>]/<slot>/  …save tree + sidecars…
├── mii_repos_bckp/                  ← backup_miis.py (numbered-slot backups)
│   ├── mii_bckp_ffl/<slot>/
│   ├── mii_bckp_rfl/<slot>/
│   └── mii_bckp_account/<slot>/
└── mii_repos/                       ← backup_miis.py (live custom _C repos)
    ├── mii_repo_FFL_C/  mii_repo_RFL_C/  mii_repo_ACCOUNT_C/
```

## Restoring on a console

1. Move the SD card to the target Wii U (or copy the `wiiu/backups/` tree onto the
   card it boots from).
2. Launch **SaveMii / ProcessMod** on that console.
3. Saves appear in **Batch Restore** (each run is one BackupSet); Miis appear in
   the Mii repo restore / import UI. Restore the entries you want.

## Reliability notes

- **Per-item, resumable.** Each title/repo is copied in its own transaction: a
  failure prints the error, rolls back that partial slot, counts it failed, and
  the run continues. The final tag reads `OK/KOs > …` when anything failed.
- **Re-running is safe.** Slots are picked as the first free `0..255`, so a second
  run adds new slots rather than overwriting; batch runs get a fresh timestamp
  folder. (Only the Mii `_C` custom repos are replaced — and the previous one is
  preserved by rename, see above.)
- **Copy buffer** is 1 MB, falling back to 256 KB if the console heap can't spare
  it — no tuning needed.
- **Origin marker.** Every `savemiiMeta.json` `tag` these scripts write is
  prefixed with **`WUPS`** (WUP = this toolset), so backups made here are
  identifiable in SaveMii: `WUPS: <summary>` on a batch root, or just `WUPS` on a
  per-slot/Mii entry. It's cosmetic — SaveMii restores them regardless.

## Testing on a PC without a console (optional)

The backup logic takes the client as a parameter, so a filesystem-backed mock can
drive it against a local dump (e.g. a Cemu/Dolphin NAND) — no console required:

```bash
python3 run_mock_backup.py [root]     # writes a real batch backup under <root>/SD/…
python3 test_mock_backup.py           # self-checking end-to-end test (saves)
python3 test_mock_miis.py             # self-checking end-to-end test (Miis)
```

See `mock_wupclient.py` and `PLAN.md` for the fixture layout and the
`mock_stat.json` permission overlay.
