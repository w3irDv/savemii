#!/usr/bin/env python3
# Self-checking end-to-end test for backup_saves.py via MockWupclient.
#
# Runs the real backup logic against the local NAND copy, writing to throwaway
# temp SD/USB dirs (mlc01 stays read-only), and asserts the output. Also builds a
# USB fixture that duplicates a NAND title to prove the USB->slot 0 / NAND->slot 1
# ordering. Exits non-zero if any check fails.
#
# Usage:  python3 test_mock_backup.py [nand_root]
#         nand_root default /mnt/c/Users/Xavier/repo/cemu (must contain mlc01/)

import json
import os
import shutil
import sys
import tempfile

import backup_saves as b
from mock_wupclient import MockWupclient

NAND_ROOT = sys.argv[1] if len(sys.argv) > 1 else "/mnt/c/Users/Xavier/repo/cemu"

# a title known to exist on the NAND copy (Nintendo Land)
TARGET_HI, TARGET_LO = 0x00050000, 0x10102100
TARGET_NAME = "Nintendo Land"
TARGET_DIR = "%s [%08x-%08x]" % (TARGET_NAME, TARGET_HI, TARGET_LO)
TARGET_REL = "/usr/save/%08x/%08x" % (TARGET_HI, TARGET_LO)

_failures = []


def check(cond, label):
    print(("PASS" if cond else "FAIL"), label)
    if not cond:
        _failures.append(label)


def only_target_enum(w, storage):
    """Patched enumerate_saves: yield just the target title (fast + focused)."""
    src = "/vol/%s%s/user" % (storage, TARGET_REL)
    if b.dir_exists(w, src):
        yield TARGET_HI, TARGET_LO


def enum_only(hi, lo):
    """Factory: a patched enumerate_saves that yields just (hi, lo) if present."""
    def gen(w, storage):
        if b.dir_exists(w, "/vol/%s/usr/save/%08x/%08x/user" % (storage, hi, lo)):
            yield hi, lo
    return gen


def read(path):
    with open(path, "rb") as f:
        return f.read()


def scenario_structure():
    """Structure, metadata, overlay sidecar, copy fidelity (NAND only)."""
    print("\n--- scenario A: structure / metadata / sidecar / copy ---")
    sd = tempfile.mkdtemp(prefix="mocksd_")
    orig_enum = b.enumerate_saves
    b.enumerate_saves = only_target_enum
    try:
        overlay = {TARGET_REL + "/user/common":
                   {"mode": 0x660, "owner": TARGET_LO, "group": 0x400, "quota": 0x1790000}}
        w = MockWupclient(NAND_ROOT, stat_overlay=overlay,
                          mounts={"storage_sdcard": sd,
                                  "storage_usb01": os.path.join(sd, "_nousb1"),
                                  "storage_usb02": os.path.join(sd, "_nousb2")})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        slot0 = os.path.join(batch, TARGET_DIR, "0")

        check(os.path.isdir(slot0), "escaped title/slot dir created")
        root_meta_path = os.path.join(batch, "savemiiMeta.json")
        check(os.path.isfile(root_meta_path), "root savemiiMeta.json exists")

        # root tag summary (1 user Wii U title, 0 failed)
        root_meta = json.loads(read(root_meta_path))
        check(root_meta["tag"] == "WUPS: ALL OK - WU:1,vW:0,S:0 titles",
              "root metadata summary tag (user title)")

        # per-slot metadata
        meta = json.loads(read(os.path.join(slot0, "savemiiMeta.json")))
        check(meta["highID"] == "00050000" and meta["lowID"] == "10102100",
              "slot metadata ids")
        check(meta["shortName"] == TARGET_NAME, "slot metadata shortName")
        check(meta["storage"] == "NAND" and meta["backupFormat"] == "FILES",
              "slot metadata storage/format")

        # stat sidecar overlay line
        sidecar = read(os.path.join(slot0, "savemii_cfg_statInfo.out")).decode()
        check(sidecar.splitlines()[0] == "[STAT EXCEPTIONSFILE]", "sidecar header")
        want = "DQ 660 10102100:400 1790000 %s/user/common" % TARGET_REL
        check(want in sidecar, "sidecar records overlaid common exception")

        # copy fidelity: copied common/save.dat == source
        src_dat = os.path.join(NAND_ROOT, "mlc01" + TARGET_REL, "user/common/save.dat")
        dst_dat = os.path.join(slot0, "common/save.dat")
        check(os.path.isfile(dst_dat) and read(dst_dat) == read(src_dat),
              "copied save.dat matches source")
    finally:
        b.enumerate_saves = orig_enum
        shutil.rmtree(sd, ignore_errors=True)


def scenario_usb_slot_ordering():
    """A dupe title on USB + NAND must land USB->slot 0, NAND->slot 1."""
    print("\n--- scenario B: USB/NAND dupe slot ordering ---")
    sd = tempfile.mkdtemp(prefix="mocksd_")
    usb = tempfile.mkdtemp(prefix="mockusb_")
    orig_enum = b.enumerate_saves
    b.enumerate_saves = only_target_enum
    try:
        # build a USB fixture that duplicates the target title:
        #   same meta.xml (so the escaped name matches) + a marker save file
        usb_save = os.path.join(usb, "usr/save/%08x/%08x" % (TARGET_HI, TARGET_LO))
        os.makedirs(os.path.join(usb_save, "meta"))
        os.makedirs(os.path.join(usb_save, "user/common"))
        shutil.copyfile(
            os.path.join(NAND_ROOT, "mlc01" + TARGET_REL, "meta/meta.xml"),
            os.path.join(usb_save, "meta/meta.xml"))
        with open(os.path.join(usb_save, "user/common/save.dat"), "wb") as f:
            f.write(b"USB-MARKER")

        w = MockWupclient(NAND_ROOT,
                          mounts={"storage_sdcard": sd, "storage_usb01": usb,
                                  "storage_usb02": os.path.join(sd, "_nousb2")})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        title = os.path.join(sd, "wiiu/backups/batch", dt, TARGET_DIR)
        slots = sorted(os.listdir(title)) if os.path.isdir(title) else []
        check(slots == ["0", "1"], "two slots created (0 and 1)")

        def storage_of(slot):
            return json.loads(read(os.path.join(title, slot, "savemiiMeta.json")))["storage"]

        def is_usb_data(slot):
            p = os.path.join(title, slot, "common/save.dat")
            return os.path.isfile(p) and read(p) == b"USB-MARKER"

        check(storage_of("0") == "USB", "slot 0 metadata storage == USB")
        check(is_usb_data("0"), "slot 0 contains the USB marker data")
        check(storage_of("1") == "NAND", "slot 1 metadata storage == NAND")
        check(not is_usb_data("1"), "slot 1 contains NAND (not USB) data")
    finally:
        b.enumerate_saves = orig_enum
        shutil.rmtree(sd, ignore_errors=True)
        shutil.rmtree(usb, ignore_errors=True)


def scenario_system_title():
    """A system-title save (highID 0x00050010) is backed up and categorized S."""
    print("\n--- scenario C: system title categorization ---")
    sys_hi, sys_lo = 0x00050010, 0x1004a200  # a system save present on the NAND copy
    sd = tempfile.mkdtemp(prefix="mocksd_")
    orig_enum = b.enumerate_saves
    b.enumerate_saves = enum_only(sys_hi, sys_lo)
    try:
        check(b.is_system_title(sys_hi), "highID 00050010 classified as system")
        w = MockWupclient(NAND_ROOT,
                          mounts={"storage_sdcard": sd,
                                  "storage_usb01": os.path.join(sd, "_nousb1"),
                                  "storage_usb02": os.path.join(sd, "_nousb2")})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        # exactly one slot dir was produced for the system title
        title_dirs = [d for d in os.listdir(batch)
                      if os.path.isdir(os.path.join(batch, d))]
        check(len(title_dirs) == 1
              and ("[%08x-%08x]" % (sys_hi, sys_lo)) in title_dirs[0],
              "system title backed up (slot dir present)")
        root_meta = json.loads(read(os.path.join(batch, "savemiiMeta.json")))
        check(root_meta["tag"] == "WUPS: ALL OK - WU:0,vW:0,S:1 titles",
              "root metadata summary tag (system title)")
    finally:
        b.enumerate_saves = orig_enum
        shutil.rmtree(sd, ignore_errors=True)


def scenario_vwii():
    """A synthetic vWii SLC save is backed up: banner-based name, `data` tree,
    vWii metadata ids, and the vW slot in the tag."""
    print("\n--- scenario D: vWii (SLCCMPT) backup ---")
    v_hi, v_lo = 0x00010001, 0x524d4345
    v_name = "vWii Game"
    v_dir = "%s [%08x-%08x]" % (v_name, v_hi, v_lo)

    sd = tempfile.mkdtemp(prefix="mocksd_")
    slc = tempfile.mkdtemp(prefix="mockslc_")
    orig_enum = b.enumerate_saves
    b.enumerate_saves = lambda w, storage: iter(())  # no Wii U saves this run
    try:
        # build the SLC fixture: title/<hi>/<lo>/data/{banner.bin, save.bin}
        data = os.path.join(slc, "title/%08x/%08x/data" % (v_hi, v_lo))
        os.makedirs(data)
        banner = b"\x00" * 0x20 + v_name.encode("utf-16-be").ljust(64, b"\x00")
        with open(os.path.join(data, "banner.bin"), "wb") as f:
            f.write(banner)
        with open(os.path.join(data, "save.bin"), "wb") as f:
            f.write(b"VWII-SAVE")

        w = MockWupclient(NAND_ROOT,
                          mounts={"storage_sdcard": sd, "storage_slccmpt01": slc,
                                  "storage_usb01": os.path.join(sd, "_n1"),
                                  "storage_usb02": os.path.join(sd, "_n2")})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        slot0 = os.path.join(batch, v_dir, "0")
        check(os.path.isdir(slot0), "vWii banner-named slot dir created")

        meta = json.loads(read(os.path.join(slot0, "savemiiMeta.json")))
        check(meta["shortName"] == v_name, "vWii shortName from banner")
        check(meta["highID"] == "00010001" and meta["lowID"] == "524d4345",
              "vWii metadata highID/lowID")
        check(meta["vWiiHighID"] == "00010001" and meta["vWiiLowID"] == "524d4345",
              "vWii metadata vWiiHighID/vWiiLowID set")

        check(os.path.isfile(os.path.join(slot0, "save.bin"))
              and read(os.path.join(slot0, "save.bin")) == b"VWII-SAVE",
              "vWii save file copied")
        # default vWii stats match -> header-only sidecar (no exceptions)
        sidecar = read(os.path.join(slot0, "savemii_cfg_statInfo.out")).decode()
        check(sidecar.strip() == "[STAT EXCEPTIONSFILE]",
              "vWii sidecar header-only (defaults match)")

        root_meta = json.loads(read(os.path.join(batch, "savemiiMeta.json")))
        check(root_meta["tag"] == "WUPS: ALL OK - WU:0,vW:1,S:0 titles",
              "root tag counts vWii title")
    finally:
        b.enumerate_saves = orig_enum
        shutil.rmtree(sd, ignore_errors=True)
        shutil.rmtree(slc, ignore_errors=True)


def _write_inject_fixture(mlc, hi, lo, short_name, vwii_lo, extra_meta=""):
    """Build usr/title/<hi>/<lo>/{code/fw.img, meta/meta.xml} for an inject."""
    base = os.path.join(mlc, "usr/title/%08x/%08x" % (hi, lo))
    os.makedirs(os.path.join(base, "code"))
    os.makedirs(os.path.join(base, "meta"))
    with open(os.path.join(base, "code/fw.img"), "wb") as f:
        f.write(b"FWIMG-STUB")  # presence is what marks an inject
    xml = ('<menu><shortname_en>%s</shortname_en>'
           '<reserved_flag2 type="hexBinary" length="8">%08x</reserved_flag2>%s'
           '</menu>' % (short_name, vwii_lo, extra_meta))
    with open(os.path.join(base, "meta/meta.xml"), "w") as f:
        f.write(xml)


def scenario_inject():
    """A vWii inject: detected in the WFS (code/fw.img), its save read from the SLC
    under the embedded vWii ids, but backed up under the Wii U inject ids."""
    print("\n--- scenario F: vWii inject backup ---")
    hi, lo = 0x00050000, 0x10012300         # Wii U inject ids
    v_hi, v_lo = 0x00010000, 0x524d4350     # embedded vWii ids ("RMCP" -> disc, not GC)
    name = "Injected Wii Game"
    tdir = "%s [%08x-%08x]" % (name, hi, lo)

    sd = tempfile.mkdtemp(prefix="mocksd_")
    mlc = tempfile.mkdtemp(prefix="mockmlc_")
    slc = tempfile.mkdtemp(prefix="mockslc_")
    orig_enum, orig_venum = b.enumerate_saves, b.enumerate_vwii_saves
    b.enumerate_saves = lambda w, storage: iter(())        # no Wii U saves
    b.enumerate_vwii_saves = lambda w: iter(())            # isolate the inject pass
    try:
        _write_inject_fixture(mlc, hi, lo, name, v_lo)
        data = os.path.join(slc, "title/%08x/%08x/data" % (v_hi, v_lo))
        os.makedirs(data)
        with open(os.path.join(data, "save.bin"), "wb") as f:
            f.write(b"INJECT-SAVE")

        w = MockWupclient(NAND_ROOT,
                          mounts={"storage_sdcard": sd, "storage_mlc01": mlc,
                                  "storage_slccmpt01": slc,
                                  "storage_usb01": os.path.join(sd, "_n1"),
                                  "storage_usb02": os.path.join(sd, "_n2")})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        slot0 = os.path.join(batch, tdir, "0")
        check(os.path.isdir(slot0), "inject slot dir named by Wii U inject ids")
        check(os.path.isfile(os.path.join(slot0, "save.bin"))
              and read(os.path.join(slot0, "save.bin")) == b"INJECT-SAVE",
              "inject save copied from SLC")

        meta = json.loads(read(os.path.join(slot0, "savemiiMeta.json")))
        check(meta["highID"] == "00050000" and meta["lowID"] == "10012300",
              "inject metadata: Wii U inject highID/lowID")
        check(meta["vWiiHighID"] == "00010000" and meta["vWiiLowID"] == "524d4350",
              "inject metadata: embedded vWii ids")
        check(meta["storage"] == "NAND" and meta["backupFormat"] == "FILES",
              "inject metadata: storage NAND / FILES")

        root_meta = json.loads(read(os.path.join(batch, "savemiiMeta.json")))
        check("inj:1" in root_meta["tag"], "root tag counts the inject")
    finally:
        b.enumerate_saves, b.enumerate_vwii_saves = orig_enum, orig_venum
        for d in (sd, mlc, slc):
            shutil.rmtree(d, ignore_errors=True)


def scenario_inject_gamecube():
    """A GameCube inject (product code starts 'G') is reported but never backed up."""
    print("\n--- scenario G: GameCube inject reported, not backed up ---")
    hi, lo = 0x00050000, 0x10099900
    v_lo = 0x47585850  # "GXXP" -> GameCube
    name = "Injected GC Game"

    sd = tempfile.mkdtemp(prefix="mocksd_")
    mlc = tempfile.mkdtemp(prefix="mockmlc_")
    slc = tempfile.mkdtemp(prefix="mockslc_")
    os.makedirs(os.path.join(slc, "title/00010000"))  # so mount_slc succeeds
    orig_enum, orig_venum = b.enumerate_saves, b.enumerate_vwii_saves
    b.enumerate_saves = lambda w, storage: iter(())
    b.enumerate_vwii_saves = lambda w: iter(())
    try:
        _write_inject_fixture(mlc, hi, lo, name, v_lo)
        w = MockWupclient(NAND_ROOT,
                          mounts={"storage_sdcard": sd, "storage_mlc01": mlc,
                                  "storage_slccmpt01": slc,
                                  "storage_usb01": os.path.join(sd, "_n1"),
                                  "storage_usb02": os.path.join(sd, "_n2")})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        subdirs = [d for d in os.listdir(batch)
                   if os.path.isdir(os.path.join(batch, d))]
        check(subdirs == [], "GameCube inject writes no backup folder")
        root_meta = json.loads(read(os.path.join(batch, "savemiiMeta.json")))
        check("gc:1" in root_meta["tag"], "root tag reports the GameCube inject")
    finally:
        b.enumerate_saves, b.enumerate_vwii_saves = orig_enum, orig_venum
        for d in (sd, mlc, slc):
            shutil.rmtree(d, ignore_errors=True)


# real Wii NAND (Dolphin) for an optional live vWii test; override via env
DOLPHIN_WII = os.environ.get(
    "DOLPHIN_WII",
    "/mnt/c/Users/Xavier/AppData/Roaming/Dolphin Emulator/Wii")


def scenario_vwii_real():
    """Optional: back up real Wii disc-game (00010000) saves from a Dolphin NAND.
    Skipped when the Dolphin Wii folder is absent."""
    print("\n--- scenario E: real vWii data (Dolphin) ---")
    if not os.path.isdir(os.path.join(DOLPHIN_WII, "title", "00010000")):
        print("SKIP (no Dolphin Wii NAND at %s)" % DOLPHIN_WII)
        return

    sd = tempfile.mkdtemp(prefix="mocksd_")
    orig_enum = b.enumerate_saves
    orig_venum = b.enumerate_vwii_saves
    b.enumerate_saves = lambda w, storage: iter(())
    # only disc games (00010000), which carry a banner
    b.enumerate_vwii_saves = lambda w: (
        (hi, lo) for hi, lo in orig_venum(w) if hi == 0x00010000)
    try:
        w = MockWupclient("/unused",
                          mounts={"storage_sdcard": sd,
                                  "storage_slccmpt01": DOLPHIN_WII})
        dt = "2000-01-01T000000"
        b.backup_all_saves(w, dt)

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        title_dirs = [d for d in os.listdir(batch)
                      if os.path.isdir(os.path.join(batch, d))]
        check(len(title_dirs) >= 1, "at least one real vWii game backed up")

        all_named = True
        all_copied = True
        for d in title_dirs:
            # dir name is "<banner name> [00010000-<lo>]"; name must not be the
            # hex fallback ("0001000<lo>") -> proves the banner parsed
            lo_hex = d[d.rindex("-") + 1:d.rindex("]")]
            name_part = d[:d.rindex(" [")]
            if name_part == ("00010000" + lo_hex):
                all_named = False
            slot0 = os.path.join(batch, d, "0")
            src = os.path.join(DOLPHIN_WII, "title", "00010000", lo_hex, "data")
            for fn in os.listdir(src):
                sp, dp = os.path.join(src, fn), os.path.join(slot0, fn)
                if not (os.path.isfile(dp) and read(dp) == read(sp)):
                    all_copied = False
        check(all_named, "real banner names parsed (not hex fallback)")
        check(all_copied, "real vWii save files copied byte-identical")
    finally:
        b.enumerate_saves = orig_enum
        b.enumerate_vwii_saves = orig_venum
        shutil.rmtree(sd, ignore_errors=True)


def scenario_rollback():
    """A copy failure rolls back the partial slot and counts the title failed."""
    print("\n--- scenario G: rollback on copy failure ---")
    sd = tempfile.mkdtemp(prefix="mocksd_")
    orig_enum = b.enumerate_saves
    b.enumerate_saves = only_target_enum
    try:
        w = MockWupclient(NAND_ROOT,
                          mounts={"storage_sdcard": sd,
                                  "storage_usb01": os.path.join(sd, "_n1"),
                                  "storage_usb02": os.path.join(sd, "_n2")})

        # simulate a mid-copy read error that leaves partial content behind
        def boom(src, dst):
            local = w._resolve(dst)
            os.makedirs(local, exist_ok=True)
            with open(os.path.join(local, "partial.tmp"), "wb") as f:
                f.write(b"partial")
            raise IOError("simulated read error")
        w.cpdir = boom

        dt = "2000-01-01T000000"
        rc = b.backup_all_saves(w, dt)  # must not raise
        check(rc == 1, "backup returns 1 failed title (did not crash)")

        batch = os.path.join(sd, "wiiu/backups/batch", dt)
        slot0 = os.path.join(batch, TARGET_DIR, "0")
        check(not os.path.exists(slot0), "partial slot rolled back (removed)")
        root_meta = json.loads(read(os.path.join(batch, "savemiiMeta.json")))
        check(root_meta["tag"] == "WUPS: OK/KOs > WU:0/1,vW:0/0,S:0/0",
              "root tag reflects the failure")
    finally:
        b.enumerate_saves = orig_enum
        shutil.rmtree(sd, ignore_errors=True)


def scenario_path_stat():
    """FSA_GetStat self-check passes, and the overlaid `common` exception is
    recorded identically via path-stat and the readdir fallback."""
    print("\n--- scenario H: path-based FSA_GetStat ---")
    overlay = {TARGET_REL + "/user/common":
               {"mode": 0x660, "owner": TARGET_LO, "group": 0x400, "quota": 0x1790000}}
    w = MockWupclient(NAND_ROOT, stat_overlay=overlay)

    sample = b.find_sample_file(w, [("storage_mlc01", "NAND")])
    check(sample is not None and b.verify_path_stat(w, sample),
          "verify_path_stat passes (FSA_GetStat == FSA_GetStatFile)")

    src = "/vol/storage_mlc01%s/user" % TARGET_REL
    _name, gid, _vwii = b.resolve_meta(w, "storage_mlc01", TARGET_HI, TARGET_LO)
    default = {"uid": TARGET_LO, "gid": gid, "mode": 0x660}
    want = "DQ 660 %x:400 1790000 %s/user/common" % (TARGET_LO, TARGET_REL)
    for flag, label in ((True, "path-stat"), (False, "readdir fallback")):
        lines = b.build_stat_lines(w, "storage_mlc01", src, default, flag)
        check(any(ln.strip() == want for ln in lines),
              "common DQ line recorded via %s" % label)


def main():
    if not os.path.isdir(os.path.join(NAND_ROOT, "mlc01")):
        print("error: %s/mlc01 not found" % NAND_ROOT)
        sys.exit(2)
    scenario_structure()
    scenario_usb_slot_ordering()
    scenario_system_title()
    scenario_vwii()
    scenario_inject()
    scenario_inject_gamecube()
    scenario_vwii_real()
    scenario_rollback()
    scenario_path_stat()
    if _failures:
        print("\nFAILED: %d check(s): %s" % (len(_failures), ", ".join(_failures)))
        sys.exit(1)
    print("\nALL CHECKS PASSED")


if __name__ == "__main__":
    main()
