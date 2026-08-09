#!/usr/bin/env python3
# Self-checking end-to-end test for backup_miis.py via MockWupclient.
#
# Backs up the three internal Mii repos against real local data (FFL + ACCOUNT
# from a Cemu mlc01, RFL from a Dolphin Wii NAND) into a throwaway temp SD, and
# asserts layout, byte-identical copies, and metadata. Exits non-zero on failure.
#
# Usage: python3 test_mock_miis.py [cemu_root] [dolphin_wii]

import glob
import json
import os
import shutil
import sys
import tempfile

import backup_miis as m
from mock_wupclient import MockWupclient

CEMU = sys.argv[1] if len(sys.argv) > 1 else "/mnt/c/Users/Xavier/repo/cemu"
DOLPHIN_WII = sys.argv[2] if len(sys.argv) > 2 else \
    "/mnt/c/Users/Xavier/AppData/Roaming/Dolphin Emulator/Wii"

MLC = os.path.join(CEMU, "mlc01")
_failures = []


def check(cond, label):
    print(("PASS" if cond else "FAIL"), label)
    if not cond:
        _failures.append(label)


def read(path):
    with open(path, "rb") as f:
        return f.read()


def same_file(a, b):
    return os.path.isfile(a) and os.path.isfile(b) and read(a) == read(b)


def make_client(sd):
    return MockWupclient("/unused", mounts={
        "storage_mlc01": MLC,
        "storage_sdcard": sd,
        "storage_slccmpt01": DOLPHIN_WII,
        "storage_usb01": os.path.join(sd, "_n1"),
        "storage_usb02": os.path.join(sd, "_n2"),
    })


def main():
    if not os.path.isdir(MLC):
        print("error: %s not found" % MLC)
        sys.exit(2)

    sd = tempfile.mkdtemp(prefix="miisd_")
    try:
        rc = m.backup_miis(make_client(sd))
        root = os.path.join(sd, "wiiu/backups/mii_repos_bckp")
        repos = os.path.join(sd, "wiiu/backups/mii_repos")
        check(rc == 0, "backup_miis reported no failures")

        # --- FFL: FFL_ODB.dat + stadio.sav + meta(repoName=FFL) ---
        mm = None
        for name in os.listdir(os.path.join(MLC, "usr/save/00050010")):
            if os.path.isfile(os.path.join(
                    MLC, "usr/save/00050010", name, "user/common/db/FFL_ODB.dat")):
                mm = name
        common = os.path.join(MLC, "usr/save/00050010", str(mm), "user/common")
        ffl = os.path.join(root, "mii_bckp_ffl/0")
        check(same_file(os.path.join(ffl, "FFL_ODB.dat"),
                        os.path.join(common, "db/FFL_ODB.dat")),
              "FFL_ODB.dat copied byte-identical")
        if os.path.isfile(os.path.join(common, "stadio.sav")):
            check(same_file(os.path.join(ffl, "stadio.sav"),
                            os.path.join(common, "stadio.sav")),
                  "stadio.sav copied byte-identical")
        fm = json.loads(read(os.path.join(ffl, "savemiiMeta.json")))
        check(fm["repoName"] == "FFL" and fm["storage"] == "internal",
              "FFL metadata repoName/storage")

        # --- FFL_C custom repo: db/FFL_ODB.dat (+ stadio.sav), no metadata ---
        ffl_c = os.path.join(repos, "mii_repo_FFL_C")
        check(same_file(os.path.join(ffl_c, "db/FFL_ODB.dat"),
                        os.path.join(common, "db/FFL_ODB.dat")),
              "FFL_C db/FFL_ODB.dat byte-identical")
        if os.path.isfile(os.path.join(common, "stadio.sav")):
            check(same_file(os.path.join(ffl_c, "stadio.sav"),
                            os.path.join(common, "stadio.sav")),
                  "FFL_C stadio.sav byte-identical (repo root)")
        check(not os.path.exists(os.path.join(ffl_c, "savemiiMeta.json")),
              "FFL_C has no savemiiMeta.json")

        # --- RFL: RFL_DB.dat + meta(repoName=RFL) ---
        rfl_src = os.path.join(DOLPHIN_WII, "shared2/menu/FaceLib/RFL_DB.dat")
        if os.path.isfile(rfl_src):
            rfl = os.path.join(root, "mii_bckp_rfl/0")
            check(same_file(os.path.join(rfl, "RFL_DB.dat"), rfl_src),
                  "RFL_DB.dat copied byte-identical")
            rm = json.loads(read(os.path.join(rfl, "savemiiMeta.json")))
            check(rm["repoName"] == "RFL", "RFL metadata repoName")

            # --- RFL_C custom repo: RFL_DB.dat (flat), no metadata ---
            rfl_c = os.path.join(repos, "mii_repo_RFL_C")
            check(same_file(os.path.join(rfl_c, "RFL_DB.dat"), rfl_src),
                  "RFL_C RFL_DB.dat byte-identical")
            check(not os.path.exists(os.path.join(rfl_c, "savemiiMeta.json")),
                  "RFL_C has no savemiiMeta.json")
        else:
            print("SKIP RFL (no Dolphin RFL_DB.dat)")

        # --- ACCOUNT: whole act tree + meta(repoName=ACCOUNT) ---
        act_src = os.path.join(MLC, "usr/save/system/act")
        acc = os.path.join(root, "mii_bckp_account/0")
        all_copied = True
        for dirpath, _dirs, files in os.walk(act_src):
            for fn in files:
                sp = os.path.join(dirpath, fn)
                dp = os.path.join(acc, os.path.relpath(sp, act_src))
                if not same_file(sp, dp):
                    all_copied = False
        check(all_copied, "ACCOUNT act/ tree copied byte-identical")
        am = json.loads(read(os.path.join(acc, "savemiiMeta.json")))
        check(am["repoName"] == "ACCOUNT", "ACCOUNT metadata repoName")

        # --- ACCOUNT_C custom repo: whole act tree, no metadata ---
        acc_c = os.path.join(repos, "mii_repo_ACCOUNT_C")
        all_c = True
        for dirpath, _dirs, files in os.walk(act_src):
            for fn in files:
                sp = os.path.join(dirpath, fn)
                dp = os.path.join(acc_c, os.path.relpath(sp, act_src))
                if not same_file(sp, dp):
                    all_c = False
        check(all_c, "ACCOUNT_C act/ tree byte-identical")
        check(not os.path.exists(os.path.join(acc_c, "savemiiMeta.json")),
              "ACCOUNT_C has no savemiiMeta.json")

        # --- slot increments on a second run ---
        m.backup_miis(make_client(sd))
        check(os.path.isdir(os.path.join(root, "mii_bckp_ffl/1")),
              "second run lands FFL in slot 1")

        # --- second run renames the existing custom repos (preserved, not deleted) ---
        archived = glob.glob(os.path.join(repos, "mii_repo_FFL_C-*"))
        check(len(archived) == 1 and os.path.isdir(archived[0]),
              "second run renamed existing FFL_C with a timestamp")
        check(os.path.isfile(os.path.join(archived[0], "db/FFL_ODB.dat")),
              "renamed FFL_C still holds its content")
        check(os.path.isfile(os.path.join(repos, "mii_repo_FFL_C/db/FFL_ODB.dat")),
              "fresh FFL_C re-created after rename")

        # --- rollback on a forced copy failure ---
        sd2 = tempfile.mkdtemp(prefix="miisd2_")
        try:
            w = make_client(sd2)
            w.mounts["storage_sdcard"] = sd2

            def boom(a, b):
                local = w._resolve(b)
                os.makedirs(os.path.dirname(local), exist_ok=True)
                with open(local, "wb") as f:
                    f.write(b"partial")
                raise IOError("simulated copy error")
            w.cp = boom
            m.backup_miis(w)
            check(not os.path.exists(
                os.path.join(sd2, "wiiu/backups/mii_repos_bckp/mii_bckp_ffl/0")),
                "failed FFL slot rolled back")
        finally:
            shutil.rmtree(sd2, ignore_errors=True)
    finally:
        shutil.rmtree(sd, ignore_errors=True)

    if _failures:
        print("\nFAILED: %d check(s): %s" % (len(_failures), ", ".join(_failures)))
        sys.exit(1)
    print("\nALL CHECKS PASSED")


if __name__ == "__main__":
    main()
