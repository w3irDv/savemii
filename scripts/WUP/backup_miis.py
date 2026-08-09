# Back up SaveMii's internal Mii repos (Mii databases) to the SD card.
#
# Covers the three internal repos, each to its first free slot:
#   - FFL     : Wii U Mii database (Mii Maker)  -> mii_bckp_ffl
#   - RFL     : vWii Mii database (SLCCMPT)      -> mii_bckp_rfl
#   - ACCOUNT : account Miis (usr/save/system/act) -> mii_bckp_account
#
# Layout + metadata mirror SaveMii's MiiRepo::backup so the backups show up in
# SaveMii's Mii repo restore UI. Backups live under a different root than saves:
#   /vol/storage_sdcard/wiiu/backups/mii_repos_bckp/<backup_folder>/<slot>/
# No StatManager stat sidecar is written for Miis (SaveMii disables it).
#
# After each internal repo is backed up, its content is also replicated into the
# matching live SaveMii custom (_C) repo (MiiUtils.cpp:80-94), so the extracted
# Miis appear as an importable SD repo. Custom repos live at a fixed path (no slot,
# no savemiiMeta.json) and are a byte-identical copy of the internal DB:
#   /vol/storage_sdcard/wiiu/backups/mii_repos/mii_repo_<REPO>_C/...
#     FFL_C     -> db/FFL_ODB.dat (+ stadio.sav at the repo root)
#     RFL_C     -> RFL_DB.dat
#     ACCOUNT_C -> the whole act tree
# If a custom repo already exists it is first renamed (atomic FSA rename) to
# mii_repo_<REPO>_C-<YYYY-MM-DDTHHMMSS> (batch-folder timestamp) so the previous
# custom Miis are preserved, never deleted; a noticeable banner lists the renamed
# folders at the end of the run.
#
# Driven by wupclient.py; PC-testable via mock_wupclient.py.
#
# Usage:  python3 backup_miis.py <console-ip>

import sys
import time

from wupclient import wupclient
from backup_saves import (
    dir_exists, makedirs, write_json, read_file_quiet, mount_sd, mount_slc,
    flush_sd, fmt_elapsed, now_date, now_date_for_folder, list_dirs,
    find_empty_slot, tag_with_marker, SLC_STORAGE, SERIAL_ID,
)

# MiiRepo.h:90  BACKUP_ROOT "fs:/vol/external01/wiiu/backups/mii_repos_bckp"
MII_BACKUP_ROOT = "/vol/storage_sdcard/wiiu/backups/mii_repos_bckp"

# MiiUtils.cpp:81-88  live custom (_C) repo root "fs:/vol/external01/wiiu/backups/mii_repos"
MII_REPOS_ROOT = "/vol/storage_sdcard/wiiu/backups/mii_repos"


def file_exists(w, path):
    """True if a file can be opened for reading."""
    fsa = w.get_fsa_handle()
    ret, fh = w.FSA_OpenFile(fsa, path, "r")
    if ret == 0:
        w.FSA_CloseFile(fsa, fh)
        return True
    return False


def mii_metadata(repo_name, tag=""):
    """Per-slot savemiiMeta.json (Metadata::write_mii, Metadata.cpp:165)."""
    return {
        "Date": now_date(),
        "serialId": SERIAL_ID,
        "storage": "internal",
        "tag": tag_with_marker(tag),
        "repoName": repo_name,
    }


def find_mii_maker_id(w):
    """Region-agnostic Mii Maker id (getMiiMakerOwner): the 1004a?00 dir under
    usr/save/00050010 that actually holds an FFL_ODB.dat (1004a000 JPN /
    1004a100 USA / 1004a200 EUR). None if no Wii U Mii database exists."""
    base = "/vol/storage_mlc01/usr/save/00050010"
    for name in list_dirs(w, base):
        try:
            v = int(name, 16)
        except ValueError:
            continue
        if v >> 12 == 0x1004a and file_exists(
                w, "%s/%s/user/common/db/FFL_ODB.dat" % (base, name)):
            return name
    return None


def backup_repo(w, repo_name, backup_folder, files=None, folder=None):
    """Back up one repo to its first free slot. `files` -> copy each file into the
    slot dir (FILE repos); `folder` -> copy the whole tree (FOLDER/ACCOUNT).
    Returns 0 on success, 1 on failure (with rollback)."""
    base = "%s/%s" % (MII_BACKUP_ROOT, backup_folder)
    slot = find_empty_slot(w, base)
    if slot < 0:
        print("[%s] no free slot" % repo_name)
        return 1
    dst = "%s/%d" % (base, slot)
    print("[%s] -> slot %d" % (repo_name, slot))
    makedirs(w, dst)
    try:
        if folder is not None:
            w.cpdir(folder, dst)
        else:
            for src in files:
                w.cp(src, "%s/%s" % (dst, src.rsplit("/", 1)[1]))
        write_json(w, "%s/savemiiMeta.json" % dst, mii_metadata(repo_name))
        print("  ok")
        return 0
    except Exception as exc:
        print("  failed: %r" % exc)
        try:
            w.rmtree(dst)
            print("  rolled back %s" % dst)
        except Exception as rbexc:
            print("  WARNING: rollback of %s failed: %r" % (dst, rbexc))
        return 1


def rename_warning(renamed):
    """Build the noticeable end-of-run banner listing renamed custom repos."""
    bar = "=" * 70
    lines = [
        "",
        bar,
        "  !!  EXISTING CUSTOM MII REPO(S) RENAMED - NOTHING WAS DELETED  !!",
        bar,
        "  A live custom (_C) repo already existed. Before writing the fresh",
        "  copy, each was renamed with a timestamp, so your previous custom",
        "  Miis are preserved under wiiu/backups/mii_repos/ :",
        "",
    ]
    width = max(len(old.rsplit("/", 1)[1]) for old, _ in renamed)
    for old, new in renamed:
        lines.append("    %-*s  ->  %s" % (width, old.rsplit("/", 1)[1],
                                           new.rsplit("/", 1)[1]))
    lines += [
        "",
        "  To use one again, rename it back (drop the -<timestamp> suffix) so",
        "  SaveMii sees it; otherwise delete them once you no longer need them.",
        bar,
    ]
    return "\n".join(lines)


def replicate_custom_repo(w, repo_name, custom_repo, timestamp, renamed,
                          files=None, file_map=None, folder=None):
    """Replicate a just-backed-up internal repo into its live SaveMii custom (_C)
    repo (MiiUtils.cpp:80-94). Byte-identical copy, no savemiiMeta.json.

    `files` -> copy each file, placing it at `file_map[basename]` within the custom
    repo (defaults to the flat basename); `folder` -> copy the whole tree.

    If a custom repo already exists it is first renamed (atomic FSA rename) to
    `<custom_repo>-<timestamp>` so the previous custom Miis are preserved, never
    deleted; the (old, new) pair is appended to `renamed` for the end-of-run
    warning. Best-effort: a failure is reported but does not change the exit code."""
    print("[%s_C] -> %s" % (repo_name, custom_repo))
    try:
        if dir_exists(w, custom_repo):
            archived = "%s-%s" % (custom_repo, timestamp)
            w.rename(custom_repo, archived)
            renamed.append((custom_repo, archived))
            print("  existing custom repo preserved as %s"
                  % archived.rsplit("/", 1)[1])
        makedirs(w, custom_repo)
        if folder is not None:
            w.cpdir(folder, custom_repo)
        else:
            for src in files:
                base = src.rsplit("/", 1)[1]
                rel = (file_map or {}).get(base, base)
                dst = "%s/%s" % (custom_repo, rel)
                parent = dst.rsplit("/", 1)[0]
                if parent != custom_repo:
                    makedirs(w, parent)
                w.cp(src, dst)
        print("  ok")
        return 0
    except Exception as exc:
        print("  failed: %r" % exc)
        try:
            w.rmtree(custom_repo)
            print("  rolled back %s" % custom_repo)
        except Exception as rbexc:
            print("  WARNING: rollback of %s failed: %r" % (custom_repo, rbexc))
        return 1


def backup_miis(w):
    start = time.monotonic()
    w.cp_block_size = 0x100000      # 1 MB copy buffer (fewer round-trips = faster)
    w.cp_block_size_min = 0x40000   # fall back to 256 KB if the heap can't fit 1 MB
    print("mounting SD card ...")
    mount_sd(w)
    makedirs(w, MII_BACKUP_ROOT)
    failures = 0
    renamed = []                    # (old, new) custom-repo folders renamed this run
    ts = now_date_for_folder()      # shared batch-style timestamp for all renames

    # FFL - Wii U Mii database (+ stadio.sav), region auto-detected
    mm = find_mii_maker_id(w)
    if mm is not None:
        common = "/vol/storage_mlc01/usr/save/00050010/%s/user/common" % mm
        files = [common + "/db/FFL_ODB.dat"]
        if file_exists(w, common + "/stadio.sav"):
            files.append(common + "/stadio.sav")
        rc = backup_repo(w, "FFL", "mii_bckp_ffl", files=files)
        failures += rc
        if rc == 0:
            replicate_custom_repo(
                w, "FFL", "%s/mii_repo_FFL_C" % MII_REPOS_ROOT, ts, renamed,
                files=files, file_map={"FFL_ODB.dat": "db/FFL_ODB.dat"})
    else:
        print("FFL: no Wii U Mii database found, skipping")

    # RFL - vWii Mii database (SLCCMPT)
    mount_slc(w)
    rfl = "/vol/%s/shared2/menu/FaceLib/RFL_DB.dat" % SLC_STORAGE
    if file_exists(w, rfl):
        rc = backup_repo(w, "RFL", "mii_bckp_rfl", files=[rfl])
        failures += rc
        if rc == 0:
            replicate_custom_repo(
                w, "RFL", "%s/mii_repo_RFL_C" % MII_REPOS_ROOT, ts, renamed,
                files=[rfl])
    else:
        print("RFL: no vWii Mii database found, skipping")

    # ACCOUNT - Wii U account Miis (whole act folder)
    act = "/vol/storage_mlc01/usr/save/system/act"
    if dir_exists(w, act):
        rc = backup_repo(w, "ACCOUNT", "mii_bckp_account", folder=act)
        failures += rc
        if rc == 0:
            replicate_custom_repo(
                w, "ACCOUNT", "%s/mii_repo_ACCOUNT_C" % MII_REPOS_ROOT, ts,
                renamed, folder=act)
    else:
        print("ACCOUNT: no account save found, skipping")

    print("\nflushing SD ...")
    flush_sd(w)

    print("done: %d repo(s) failed  ->  %s" % (failures, MII_BACKUP_ROOT))
    print("elapsed: %s" % fmt_elapsed(time.monotonic() - start))
    if renamed:
        print(rename_warning(renamed))
    return failures


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: python3 backup_miis.py <console-ip>")
        sys.exit(1)
    w = wupclient(sys.argv[1])
    backup_miis(w)
