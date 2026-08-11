# Minimal Python reimplementation of SaveMii's backupAllSave (console -> SD).
#
# Batch-copies save data to a timestamped batch folder on the SD card,
# reproducing SaveMii's escaped title-name folder convention and writing
# SaveMii-compatible savemiiMeta.json files so the backups show up in SaveMii's
# Batch Restore list. Covers:
#   - Wii U user + system saves  (storage_mlc01 / USB, usr/save/<hi>/<lo>/user)
#   - vWii saves                 (SLCCMPT, title/<hi>/<lo>/data, banner-named)
#   - injects                    (Wii/GameCube games repackaged as Wii U titles):
#                                 vWii injects are backed up from the SLCCMPT under
#                                 their Wii U inject ids; GameCube injects are only
#                                 reported (their saves belong to Nintendont)
#
# Driven by wupclient.py (talks to the wupserver homebrew over TCP).
#
# Includes SaveMii's StatManager permission sidecar (savemii_cfg_statInfo.out)
# per slot, so a SaveMii restore resets IOSU owner/group/mode losslessly.
#
# Limitations vs the C original:
#   - no FAT32 filename-escape translation files
#   - GameCube injects are reported but not backed up (Nintendont manages GC saves)
#   - serialId is the "_WIIU_" placeholder (real serial needs an MCP ioctl)
#   - vWii stat owner defaults to 0 (savemii_title_id is runtime-only); vWii
#     permission restore is unreliable per SaveMii's own comment
#
# Usage:  python3 backup_saves.py <console-ip>

import json
import re
import sys
import time

from wupclient import wupclient

# --- constants mirrored from the SaveMii source -----------------------------

# src/savemng.cpp:32  "fs:/vol/external01/wiiu/backups/batch"  (Cafe OS path)
# at the IOSU/FSA layer the SD card lives at /vol/storage_sdcard
SD_BATCH_ROOT = "/vol/storage_sdcard/wiiu/backups/batch"

# EscapeFAT32Utils.cpp:246  char forbidden[] = "\\/:*?\"<>|";
FORBIDDEN = set('\\/:*?"<>|')

# TitleUtils.cpp:893-894
FILENAME_BUFF_SIZE = 256
ID_STR_BUFF_SIZE = 21
NAME_TRUNC = FILENAME_BUFF_SIZE - ID_STR_BUFF_SIZE  # 235

# AmbientConfig.h:20  inline std::string unknownSerialId{"_WIIU_"};
SERIAL_ID = "_WIIU_"

# NAND is always present; USB / SLC volumes are probed at runtime.
NAND_STORAGE = "storage_mlc01"
USB_STORAGES = ("storage_usb01", "storage_usb02")
# vWii SLCCMPT: we mount /dev/slccmpt01 here (any /vol path works; the stat file
# stores device-relative "/title/..." either way). SaveMii uses /vol/storage_slcc01.
SLC_STORAGE = "storage_slccmpt01"

# highID categories, matching SaveMii's title lists (all under usr/save):
#   loadWiiUTitles    -> user titles   {0x00050000 games, 0x00050002 demos}
#   loadWiiUSysTitles -> system titles {0x00050010, 0x00050030}
USER_HIGH_IDS = (0x00050000, 0x00050002)
SYSTEM_HIGH_IDS = (0x00050010, 0x00050030)

# vWii stat defaults (set_default_stat_cfg_for_vwii_savedata, StatManager.cpp:321).
# owner is the running SaveMii app's title-id low word (OSGetTitleID); unknown
# offline, so 0. vWii permission restore is unreliable per SaveMii's own comment.
SAVEMII_TITLE_ID = 0
VWII_DEFAULT_MODE = 0x666
VWII_DEFAULT_GID = 0x400

# vWii title enumeration, matching loadWiiTitles (TitleUtils.cpp): only these
# highIDs (disc games / channels / hidden channels), minus a blacklist of
# non-save titles (disc update partitions UPD/UPE/UPJ, Homebrew Channel, UNEO).
VWII_HIGH_IDS = (0x00010000, 0x00010001, 0x00010004)
VWII_BLACKLIST = frozenset({
    (0x00010000, 0x00555044), (0x00010000, 0x00555045), (0x00010000, 0x0055504a),
    (0x00010001, 0x48424344), (0x00010001, 0x554e454f),
})

# StatManager.h  savemii_stat_info_file
STAT_INFO_FILE = "savemii_cfg_statInfo.out"
# StatManager.cpp:166 (batch overload used by backupAllSave)
STAT_HEADER = "[STAT EXCEPTIONS FILE]\n"

# FSStatFlags (filesystem.h:156) -> get_stat entryType chars (StatManager.cpp:250)
STAT_FLAGS = (
    (0x80000000, "D"),
    (0x60000000, "Q"),
    (0x01000000, "F"),
    (0x00800000, "E"),
    (0x00010000, "L"),
)

# default file mode for Wii U savedata (set_default_stat_cfg_for_wiiu_savedata)
DEFAULT_WIIU_MODE = 0x660


# --- pure helpers (no console needed) ---------------------------------------

def fat32_escape(name):
    """Reproduce Escape::convertToFAT32ASCIICompliant (EscapeFAT32Utils.cpp:248).

    Each non-ASCII codepoint -> '_'; ASCII chars in the forbidden set -> '_';
    everything else kept; then leading spaces stripped.
    """
    out = []
    for ch in name:
        if ord(ch) > 0x7F:
            out.append("_")
        elif ch in FORBIDDEN:
            out.append("_")
        else:
            out.append(ch)
    return "".join(out).lstrip(" ")


def title_dir_name(short_name, hi, lo):
    """Reproduce setTitleNameBasedDirName (TitleUtils.cpp:896).

    escaped_shortname[:235] + " [%08x-%08x]".  The escaped name is pure ASCII,
    so a str slice matches the C byte-length truncation.
    """
    escaped = fat32_escape(short_name)[:NAME_TRUNC]
    return "%s [%08x-%08x]" % (escaped, hi, lo)


def now_date():
    """getNowDate() -> "%02d/%02d/%d %02d:%02d" (mday/mon/year hour:min)."""
    return time.strftime("%d/%m/%Y %H:%M")


def now_date_for_folder():
    """getNowDateForFolder() -> "%04d-%02d-%02dT%02d%02d%02d"."""
    return time.strftime("%Y-%m-%dT%H%M%S")


def fmt_elapsed(seconds):
    """Human-readable elapsed time."""
    seconds = int(seconds)
    h, rem = divmod(seconds, 3600)
    m, s = divmod(rem, 60)
    return "%dh %02dm %02ds" % (h, m, s) if h else "%dm %02ds" % (m, s)


_XML_ENTITIES = (
    ("&quot;", '"'),
    ("&apos;", "'"),
    ("&lt;", "<"),
    ("&gt;", ">"),
    ("&amp;", "&"),
)


def decode_xml(s):
    """Reproduce StringUtils::decodeXMLEscapeLine (order matters: &amp; last)."""
    for entity, ch in _XML_ENTITIES:
        s = s.replace(entity, ch)
    return s


# --- inject classifiers (pure) ----------------------------------------------
# An inject is a Wii/GameCube game repackaged as a Wii U title (highID
# 00050000/00050002). SaveMii detects one by a code/fw.img (the vWii boot image);
# its savedata lives on the vWii NAND under the embedded vWii ids below.

# guess_vwii_high_id: Wii Fit / Wii Fit+ / Mario Kart / Rabbids ship a game channel
# (TitleUtils.cpp:960).
_GAME_CHANNEL_LOW_IDS = (0x52464E, 0x524650, 0x524D43, 0x524757)


def vwii_inject_product_code(vwii_low_id):
    """The 4-char product code of an inject: the bytes of vwii_low_id big-endian, each
    byte < 32 (SaveMii stores it as signed char[], so high bytes too) -> '.'
    (TitleUtils.cpp:274-278)."""
    out = []
    for shift in (24, 16, 8, 0):
        b = (vwii_low_id >> shift) & 0xFF
        signed = b - 256 if b >= 0x80 else b  # signed-char semantics
        out.append("." if signed < 32 else chr(b))
    return "".join(out)


def is_gamecube_inject(product_code, vwii_high_id):
    """GameCube test (TitleUtils.cpp:297-311): product[0] 'G'/'D' -> GameCube; 'P' ->
    GameCube only at vWiiHighID 00010001 (a promotional GC disc); else not."""
    c0 = product_code[0] if product_code else ""
    if c0 in ("G", "D"):
        return True
    if c0 == "P":
        return vwii_high_id == 0x00010001
    return False


def guess_vwii_high_id(vwii_low_id, product_code):
    """Fallback vWiiHighID for an inject not found on the NAND (TitleUtils.cpp:958).
    Faithful port INCLUDING the original `>> 16` shift, which makes the 24-bit game-
    channel table never match (so a "RFNP"-style title falls through to the disc rule,
    not 00010004). Returns 0x00010004 / 0x00010000 / 0."""
    low_noregion = vwii_low_id >> 16  # faithful to the source (yes, 16 not 8)
    if low_noregion in _GAME_CHANNEL_LOW_IDS:
        return 0x00010004
    c0 = product_code[0] if product_code else ""
    if c0 in ("R", "S", "G", "D", "P"):
        return 0x00010000
    return 0


# Marks every metadata tag these scripts write as produced by this backup tool.
# Detail tags read "WUPS: <detail>"; empty tags (per-title slots, Mii repos) read
# "WUPS". SaveMii stores tag as an unbounded std::string, so this cannot overflow.
TAG_MARKER = "WUPS"


def tag_with_marker(detail=""):
    """Prefix a metadata tag with TAG_MARKER to record the backup's origin."""
    return "%s: %s" % (TAG_MARKER, detail) if detail else TAG_MARKER


def slot_metadata(hi, lo, short_name, storage, vwii_hi=0, vwii_lo=0):
    """Per-slot savemiiMeta.json (Metadata::write, FILES format). For vWii,
    SaveMii sets vWiiHighID/vWiiLowID = highID/lowID; Wii U leaves them 0."""
    return {
        "Date": now_date(),
        "serialId": SERIAL_ID,
        "storage": storage,  # "USB" or "NAND"
        "tag": tag_with_marker(),
        "highID": "%08x" % hi,
        "lowID": "%08x" % lo,
        "vWiiHighID": "%08x" % vwii_hi,
        "vWiiLowID": "%08x" % vwii_lo,
        "shortName": short_name,
        "backupFormat": "FILES",
    }


def root_metadata(datetime_folder, tag=""):
    """Batch-root savemiiMeta.json (writeBackupAllMetadata / Metadata ctor 4)."""
    return {
        "Date": datetime_folder,
        "serialId": SERIAL_ID,
        "storage": "",
        "tag": tag_with_marker(tag),
        "highID": "00000000",
        "lowID": "00000000",
        "vWiiHighID": "00000000",
        "vWiiLowID": "00000000",
        "shortName": "",
        "backupFormat": "",
    }


# --- stat sidecar helpers (pure) --------------------------------------------

def parse_stat(unk):
    """Parse a big-endian FSAStat (the 0x64-byte `unk` from FSA_ReadDir).

    FSStat layout (filesystem.h:329): flags@0, mode@4, owner@8, group@0xC,
    size@0x10, allocSize@0x14, quotaSize@0x18 (u64).
    """
    return {
        "flags": int.from_bytes(unk[0:4], "big"),
        "mode": int.from_bytes(unk[4:8], "big"),
        "owner": int.from_bytes(unk[8:12], "big"),
        "group": int.from_bytes(unk[12:16], "big"),
        "quota": int.from_bytes(unk[24:32], "big"),
    }


def entry_type(flags):
    """Compose the D/Q/F/E/L type string (StatManager.cpp:250-259)."""
    t = "".join(ch for bit, ch in STAT_FLAGS if (flags & bit) != 0)
    return t if t != "" else "00"


def stat_line(st, path_rel):
    """One exceptions-file line: '<type> <mode> <owner>:<group> <quota> <path>'.
    Matches the C printf "%s %x %x:%x %llx %s" (StatManager.cpp:273)."""
    return "%s %x %x:%x %x %s\n" % (
        entry_type(st["flags"]), st["mode"], st["owner"], st["group"],
        st["quota"], path_rel,
    )


def is_stat_exception(st, default):
    """get_stat only records entries whose mode/owner/group differ from the
    default (StatManager.cpp:272)."""
    return (st["mode"] != default["mode"]
            or st["owner"] != default["uid"]
            or st["group"] != default["gid"])


# --- console helpers (operate on a wupclient instance) ----------------------

def dir_exists(w, path):
    fsa = w.get_fsa_handle()
    ret, dir_handle = w.FSA_OpenDir(fsa, path)
    if ret == 0:
        w.FSA_CloseDir(fsa, dir_handle)
        return True
    return False


def file_exists(w, path):
    """True if `path` opens as a file. Cheaper than read_file_quiet -- opens and closes
    without reading (fw.img is multiple MB)."""
    fsa = w.get_fsa_handle()
    ret, file_handle = w.FSA_OpenFile(fsa, path, "r")
    if ret == 0:
        w.FSA_CloseFile(fsa, file_handle)
        return True
    return False


def makedirs(w, path):
    """Console-side `mkdir -p`. wupclient.mkdir only makes one level and cpdir
    does not create its own root, so build the chain here (ignore exists)."""
    fsa = w.get_fsa_handle()
    parts = [p for p in path.split("/") if p != ""]
    cur = ""
    for p in parts:
        cur += "/" + p
        if not dir_exists(w, cur):
            w.FSA_MakeDir(fsa, cur, 0x600)  # ignore result; next step will fail loudly


def read_file_quiet(w, path):
    """Read a whole console file, or None if it can't be opened (no error print)."""
    fsa = w.get_fsa_handle()
    ret, file_handle = w.FSA_OpenFile(fsa, path, "r")
    if ret != 0:
        return None
    buf = bytearray()
    block_size = 0x400
    while True:
        ret, data = w.FSA_ReadFile(fsa, file_handle, 0x1, block_size)
        if ret == 0 or ret > block_size:
            break
        buf += data[:ret]
        if ret < block_size:
            break
    w.FSA_CloseFile(fsa, file_handle)
    return buf


def write_bytes(w, path, data):
    """Write bytes to a console file (create/truncate). Mirrors wupclient.up but
    from memory. Raises IOError on open failure or a short/failed write, so a bad
    metadata/sidecar write is caught by the caller and rolled back."""
    fsa = w.get_fsa_handle()
    ret, file_handle = w.FSA_OpenFile(fsa, path, "w")
    if ret != 0:
        raise IOError("write: cannot open %s (%08X)" % (path, ret))
    block_size = 0x400
    k = 0
    try:
        while k < len(data):
            chunk = data[k:k + block_size]
            wret = w.FSA_WriteFile(fsa, file_handle, chunk)
            if wret != len(chunk):  # short write or FSA error code
                raise IOError("write: error on %s (%08X, expected %X)"
                              % (path, wret, len(chunk)))
            k += len(chunk)
    finally:
        w.FSA_CloseFile(fsa, file_handle)
    return True


def write_json(w, path, obj):
    data = json.dumps(obj, indent=2, ensure_ascii=False).encode("utf-8")
    return write_bytes(w, path, data)


def mount_sd(w):
    handle = w.open("/dev/fsa", 0)
    ret = w.FSA_Mount(handle, "/dev/sdcard01", "/vol/storage_sdcard", 2)
    w.close(handle)
    return ret  # non-zero if already mounted; harmless


def flush_sd(w):
    """Flush the SD volume so the backup is committed to the card (FSAFlushVolume)."""
    handle = w.open("/dev/fsa", 0)
    w.FSA_FlushVolume(handle, "/vol/storage_sdcard")
    w.close(handle)


def resolve_meta(w, storage, hi, lo):
    """Return (short_name, group_id, vwii_low_id) from meta.xml, preferring the
    installed-title meta over the save meta. short_name is shortname_en (fallback
    shortname_ja), XML-decoded; falls back to the "%08x%08x" hex form when absent/empty.
    group_id (default gid for the stat file) is 0 when the meta is missing.
    vwii_low_id is <reserved_flag2> as hex (0 unless this is an inject; low 32 bits)."""
    candidates = [
        "/vol/%s/usr/title/%08x/%08x/meta/meta.xml" % (storage, hi, lo),
        "/vol/%s/usr/save/%08x/%08x/meta/meta.xml" % (storage, hi, lo),
    ]
    short_name = "%08x%08x" % (hi, lo)
    group_id = 0
    vwii_low_id = 0
    for path in candidates:
        raw = read_file_quiet(w, path)
        if raw is None:
            continue
        xml = raw.decode("utf-8", errors="replace")
        for field in ("shortname_en", "shortname_ja"):
            m = re.search(field + r"[^>]*>([^<]*)<", xml)
            if m and m.group(1).strip("\x00").strip() != "":
                short_name = decode_xml(m.group(1)).split("\x00", 1)[0]
                break
        g = re.search(r"group_id[^>]*>([^<]*)<", xml)
        if g and g.group(1).strip():
            group_id = int(g.group(1).strip(), 16)
        r = re.search(r"reserved_flag2[^>]*>([^<]*)<", xml)
        if r and r.group(1).strip():
            vwii_low_id = int(r.group(1).strip(), 16) & 0xFFFFFFFF
        return short_name, group_id, vwii_low_id
    return short_name, group_id, vwii_low_id


def build_stat_lines(w, storage, src_root, default, use_path_stat=True):
    """Reproduce StatManager's exceptions file for a save tree.

    `src_root` is the copied source dir (Wii U `.../<hi>/<lo>/user`, or vWii
    `.../<hi>/<lo>/data`). copyDir stats the root dir and every descendant
    (FSUtils.cpp:483/408). Permissions come from the **path-based** `FSA_GetStat`
    (like SaveMii's get_stat) — authoritative owner/group/mode for files AND dirs;
    if `use_path_stat` is False (self-check failed) they fall back to the
    `FSA_ReadDir` entry stat (`unk`). Only entries whose mode/owner/group differ
    from `default` are recorded; stored paths are device-relative."""
    fsa = w.get_fsa_handle()
    prefix = "/vol/%s" % storage
    rel = lambda p: p[len(prefix):]  # noqa: E731  strip /vol/<storage>
    lines = []

    def record(fsa_path, unk):
        if use_path_stat:
            ret, data = w.FSA_GetStat(fsa, fsa_path)
            if ret != 0:
                raise IOError("stat: %s (%08X)" % (fsa_path, ret))
            st = parse_stat(data)
        else:
            if unk is None:
                return  # no readdir entry to fall back on
            st = parse_stat(unk)
        if is_stat_exception(st, default):
            lines.append(stat_line(st, rel(fsa_path)))

    # the src_root dir itself (its readdir `unk`, for the fallback, lives in the
    # parent listing; path-stat ignores it)
    root_unk = None
    parent, _, leaf = src_root.rpartition("/")
    for e in w.silent_ls(parent, True) or []:
        if e["name"] == leaf and not e["is_file"]:
            root_unk = e["unk"]
            break
    record(src_root, root_unk)

    def walk(fsa_path):
        for e in w.silent_ls(fsa_path, True) or []:
            child = fsa_path + "/" + e["name"]
            record(child, e["unk"])
            if not e["is_file"]:
                walk(child)

    walk(src_root)
    return lines


def _walk_first_file(w, path):
    """First regular file under a tree (for the path-stat self-check)."""
    for e in w.silent_ls(path, True) or []:
        child = path + "/" + e["name"]
        if e["is_file"]:
            return child
        found = _walk_first_file(w, child)
        if found:
            return found
    return None


def find_sample_file(w, storages):
    """Any regular save file, to validate FSA_GetStat against FSA_GetStatFile."""
    for storage, _label in storages:
        for hi, lo in enumerate_saves(w, storage):
            f = _walk_first_file(
                w, "/vol/%s/usr/save/%08x/%08x/user" % (storage, hi, lo))
            if f:
                return f
    return None


def verify_path_stat(w, sample):
    """True if FSA_GetStat (path) agrees with FSA_GetStatFile (handle) on a file's
    mode/owner/group — validates the GetInfoByQuery command id on this server."""
    fsa = w.get_fsa_handle()
    ret, data = w.FSA_GetStat(fsa, sample)
    if ret != 0:
        return False
    pst = parse_stat(data)
    ret, fh = w.FSA_OpenFile(fsa, sample, "r")
    if ret != 0:
        return False
    ret, words = w.FSA_GetStatFile(fsa, fh)
    w.FSA_CloseFile(fsa, fh)
    if ret != 0:
        return False
    # FSA_GetStatFile: 25 words, 4-byte prefix -> mode@2, owner@3, group@4
    return (pst["mode"] == words[2] and pst["owner"] == words[3]
            and pst["group"] == words[4])


def mount_slc(w):
    """Mount the vWii SLCCMPT NAND at /vol/<SLC_STORAGE> (like wupclient's
    mount_slccmpt01). Returns True if the title dir is reachable afterwards."""
    handle = w.open("/dev/fsa", 0)
    w.FSA_Mount(handle, "/dev/slccmpt01", "/vol/%s" % SLC_STORAGE, 2)
    w.close(handle)
    return dir_exists(w, "/vol/%s/title" % SLC_STORAGE)


def resolve_vwii_name(w, hi, lo):
    """vWii shortName from the save banner (storage_slcc01:/title/<hi>/<lo>/data/
    banner.bin): UTF-16BE title, 32 code units at offset 0x20 (TitleUtils.cpp:440).
    Falls back to the hex form when the banner is missing/empty."""
    banner = read_file_quiet(
        w, "/vol/%s/title/%08x/%08x/data/banner.bin" % (SLC_STORAGE, hi, lo))
    if banner is not None and len(banner) >= 0x20 + 64:
        name = bytes(banner[0x20:0x20 + 64]).decode("utf-16-be", errors="replace")
        name = name.split("\x00", 1)[0]
        if name.strip() != "":
            return name
    return "%08x%08x" % (hi, lo)


def enumerate_vwii_saves(w):
    """Yield (hi, lo) for vWii saves, matching loadWiiTitles: only the allowed
    highIDs, minus the blacklist, that have a `data` subdir. (An unrestricted
    scan of title/ would wrongly include IOS/system titles, the system menu,
    the Homebrew Channel, disc update partitions, etc.)"""
    for hi in VWII_HIGH_IDS:
        hi_path = "/vol/%s/title/%08x" % (SLC_STORAGE, hi)
        for lo_name in list_dirs(w, hi_path):
            try:
                lo = int(lo_name, 16)
            except ValueError:
                continue
            if (hi, lo) in VWII_BLACKLIST:
                continue
            if dir_exists(w, "%s/%s/data" % (hi_path, lo_name)):
                yield hi, lo


def list_dirs(w, path):
    """Return the names of sub-directories of a console path (empty if none)."""
    entries = w.silent_ls(path, True) or []
    return [e["name"] for e in entries if not e["is_file"]]


def enumerate_saves(w, storage):
    """Yield (hi, lo) for every usr/save/<hi>/<lo> that has a `user` subdir."""
    save_root = "/vol/%s/usr/save" % storage
    for hi_name in list_dirs(w, save_root):
        try:
            hi = int(hi_name, 16)
        except ValueError:
            continue
        hi_path = "%s/%s" % (save_root, hi_name)
        for lo_name in list_dirs(w, hi_path):
            try:
                lo = int(lo_name, 16)
            except ValueError:
                continue
            if dir_exists(w, "%s/%s/user" % (hi_path, lo_name)):
                yield hi, lo


def enumerate_injects(w, storage):
    """Yield (hi, lo) for installed Wii U titles that are injects: usr/title/<hi>/<lo>
    (hi in USER_HIGH_IDS) carrying a code/fw.img (the vWii boot image). Their savedata
    is on the vWii NAND, not usr/save, so they are missed by enumerate_saves."""
    title_root = "/vol/%s/usr/title" % storage
    for hi in USER_HIGH_IDS:
        hi_path = "%s/%08x" % (title_root, hi)
        for lo_name in list_dirs(w, hi_path):
            try:
                lo = int(lo_name, 16)
            except ValueError:
                continue
            if file_exists(w, "%s/%s/code/fw.img" % (hi_path, lo_name)):
                yield hi, lo


def find_inject_vwii_high(w, vwii_lo):
    """Locate an inject's vWii title on the SLCCMPT to fix its vWiiHighID. Probes the
    three VWII_HIGH_IDS for title/<H>/<vwii_lo>, preferring the H that also has a `data`
    subdir. Returns (vwii_hi, has_data); (0, False) if not present."""
    found_hi = 0
    for H in VWII_HIGH_IDS:
        base = "/vol/%s/title/%08x/%08x" % (SLC_STORAGE, H, vwii_lo)
        if dir_exists(w, base):
            found_hi = H
            if dir_exists(w, base + "/data"):
                return H, True
    return found_hi, False


def find_empty_slot(w, title_dir):
    """First slot 0..255 whose folder does not yet exist (getEmptySlot)."""
    for slot in range(256):
        if not dir_exists(w, "%s/%d" % (title_dir, slot)):
            return slot
    return -1


def is_system_title(hi):
    """True for Wii U system-title highIDs (loadWiiUSysTitles)."""
    return hi in SYSTEM_HIGH_IDS


def storage_of_src(src):
    """/vol/<storage>/... -> <storage> (for device-relative path stripping)."""
    return src.split("/", 3)[2]


# --- orchestration ----------------------------------------------------------

def backup_all_saves(w, datetime_folder, tag=""):
    start = time.monotonic()
    w.cp_block_size = 0x100000      # 1 MB copy buffer (fewer round-trips = faster)
    w.cp_block_size_min = 0x40000   # fall back to 256 KB if the heap can't fit 1 MB
    print("mounting SD card ...")
    mount_sd(w)

    batch_root = "%s/%s" % (SD_BATCH_ROOT, datetime_folder)
    makedirs(w, batch_root)

    # USB first, then NAND: a dupe title lands USB -> slot 0, NAND -> slot 1,
    # matching SaveMii's two-pass ordering.
    storages = []
    for usb in USB_STORAGES:
        if dir_exists(w, "/vol/%s/usr/save" % usb):
            storages.append((usb, "USB"))
    storages.append((NAND_STORAGE, "NAND"))

    # Validate path-based FSA_GetStat (authoritative perms) against the known-good
    # FSA_GetStatFile on a real file; fall back to readdir stats if it disagrees.
    use_path_stat = True
    sample = find_sample_file(w, storages)
    if sample is not None:
        if verify_path_stat(w, sample):
            print("permissions: FSA_GetStat verified (real owner/group/mode)")
        else:
            use_path_stat = False
            print("WARNING: FSA_GetStat unavailable/mismatched on this server; "
                  "falling back to readdir stats (permissions may be incomplete)")

    # counts split by category, like SaveMii's separate wiiutitles /
    # wiiusystitles / wiititles(vWii) backup passes. "Inject" = vWii injects backed up.
    ok = {"WiiU": 0, "System": 0, "vWii": 0, "Inject": 0}
    ko = {"WiiU": 0, "System": 0, "vWii": 0, "Inject": 0}
    gc_injects = []           # GameCube injects: reported, never backed up
    vwii_injects_nonand = []  # vWii injects whose NAND save was unavailable

    def do_backup(cat, storage_label, src, dir_name, short_name, hi, lo,
                  default, vwii_hi=0, vwii_lo=0):
        title_dir = "%s/%s" % (batch_root, dir_name)
        slot = find_empty_slot(w, title_dir)
        if slot < 0:
            print("skip %08x-%08x : no free slot" % (hi, lo))
            ko[cat] += 1
            return
        dst = "%s/%d" % (title_dir, slot)
        print("\n[%s/%s] %s  (%08x-%08x) -> slot %d" %
              (storage_label, cat, short_name, hi, lo, slot))
        makedirs(w, dst)
        try:
            w.cpdir(src, dst)
            # StatManager exceptions sidecar (owner/group/mode for restore)
            lines = build_stat_lines(w, storage_of_src(src), src, default,
                                     use_path_stat)
            write_bytes(w, "%s/%s" % (dst, STAT_INFO_FILE),
                        (STAT_HEADER + "".join(lines)).encode("utf-8"))
            write_json(w, "%s/savemiiMeta.json" % dst,
                       slot_metadata(hi, lo, short_name, storage_label,
                                     vwii_hi, vwii_lo))
            ok[cat] += 1
        except Exception as exc:  # keep going on a single failure
            print("  backup failed: %r" % exc)
            # roll back the partial/corrupt slot so no half-written backup remains
            try:
                w.rmtree(dst)
                print("  rolled back %s" % dst)
            except Exception as rbexc:
                print("  WARNING: rollback of %s failed: %r" % (dst, rbexc))
            ko[cat] += 1

    # Wii U user + system saves (NAND + USB), USB first for slot ordering
    for storage, storage_label in storages:
        for hi, lo in enumerate_saves(w, storage):
            cat = "System" if is_system_title(hi) else "WiiU"
            src = "/vol/%s/usr/save/%08x/%08x/user" % (storage, hi, lo)
            short_name, group_id, _vwii_lo = resolve_meta(w, storage, hi, lo)
            default = {"uid": lo, "gid": group_id, "mode": DEFAULT_WIIU_MODE}
            do_backup(cat, storage_label, src, title_dir_name(short_name, hi, lo),
                      short_name, hi, lo, default)

    # The vWii SLCCMPT NAND feeds both the inject pass and the native vWii pass; mount
    # it once. vWii/inject savedata uses the vWii stat defaults on restore.
    slc_ok = mount_slc(w)
    vwii_default = {"uid": SAVEMII_TITLE_ID, "gid": VWII_DEFAULT_GID,
                    "mode": VWII_DEFAULT_MODE}

    # Injects (Wii/GameCube games repackaged as Wii U titles): installed on the WFS but
    # with their savedata on the vWii NAND. Backed up under the Wii U inject ids so
    # SaveMii restores them onto the installed inject via Wii U Save Management.
    for storage, _storage_label in storages:  # inject saves are always on the NAND
        for hi, lo in enumerate_injects(w, storage):
            short_name, _group_id, vwii_lo = resolve_meta(w, storage, hi, lo)
            product_code = vwii_inject_product_code(vwii_lo)
            vwii_hi, has_data = (find_inject_vwii_high(w, vwii_lo) if slc_ok
                                 else (0, False))
            if vwii_hi == 0:
                vwii_hi = guess_vwii_high_id(vwii_lo, product_code)

            if is_gamecube_inject(product_code, vwii_hi):
                print("\nGAMECUBE INJECT (cannot be managed - restore its save from "
                      "Nintendont): %s  (%08x-%08x)" % (short_name, hi, lo))
                gc_injects.append("%s [%08x-%08x]" % (short_name, hi, lo))
                continue
            if not slc_ok or not has_data:
                print("\nvWii INJECT not backed up (vWii NAND save unavailable): "
                      "%s  (%08x-%08x)" % (short_name, hi, lo))
                vwii_injects_nonand.append("%s [%08x-%08x]" % (short_name, hi, lo))
                continue

            src = "/vol/%s/title/%08x/%08x/data" % (SLC_STORAGE, vwii_hi, vwii_lo)
            do_backup("Inject", "NAND", src, title_dir_name(short_name, hi, lo),
                      short_name, hi, lo, vwii_default, vwii_hi=vwii_hi, vwii_lo=vwii_lo)

    # vWii saves (SLCCMPT): title/<hi>/<lo>/data, banner-based names
    if slc_ok:
        for hi, lo in enumerate_vwii_saves(w):
            src = "/vol/%s/title/%08x/%08x/data" % (SLC_STORAGE, hi, lo)
            short_name = resolve_vwii_name(w, hi, lo)
            do_backup("vWii", "NAND", src, title_dir_name(short_name, hi, lo),
                      short_name, hi, lo, vwii_default, vwii_hi=hi, vwii_lo=lo)

    titles_ok = sum(ok.values())
    titles_ko = sum(ko.values())

    # batch-root metadata makes the set appear in SaveMii's BackupSetList; the
    # tag is the summary shown there (mirrors writeBackupAllMetadata's tag,
    # BatchBackupState.cpp:154-169), split by WiiU / vWii / System.
    inj_part = ""
    if ok["Inject"] or gc_injects or vwii_injects_nonand:
        inj_part = ",inj:%d,gc:%d,inj-nonand:%d" % (
            ok["Inject"], len(gc_injects), len(vwii_injects_nonand))
    if tag == "":
        if titles_ko == 0:
            tag = "ALL OK - WU:%d,vW:%d,S:%d titles%s" % (
                ok["WiiU"], ok["vWii"], ok["System"], inj_part)
        else:
            tag = "OK/KOs > WU:%d/%d,vW:%d/%d,S:%d/%d%s" % (
                ok["WiiU"], ko["WiiU"], ok["vWii"], ko["vWii"],
                ok["System"], ko["System"], inj_part)
    try:
        write_json(w, "%s/savemiiMeta.json" % batch_root,
                   root_metadata(datetime_folder, tag))
    except Exception as exc:  # per-title backups already done; don't discard them
        print("WARNING: could not write batch metadata: %r" % exc)

    print("\nflushing SD ...")
    flush_sd(w)

    print("\ndone: WiiU %d/%d, vWii %d/%d, System %d/%d, Inject %d/%d (ok/fail)  ->  %s" %
          (ok["WiiU"], ko["WiiU"], ok["vWii"], ko["vWii"],
           ok["System"], ko["System"], ok["Inject"], ko["Inject"], batch_root))
    if gc_injects:
        print("  GameCube injects NOT backed up (restore from Nintendont): %d" %
              len(gc_injects))
        for s in gc_injects:
            print("      - %s" % s)
    if vwii_injects_nonand:
        print("  vWii injects NOT backed up (vWii NAND unavailable): %d" %
              len(vwii_injects_nonand))
        for s in vwii_injects_nonand:
            print("      - %s" % s)
    print("elapsed: %s" % fmt_elapsed(time.monotonic() - start))
    return titles_ko


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: python3 backup_saves.py <console-ip>")
        sys.exit(1)
    ip = sys.argv[1]
    w = wupclient(ip)
    backup_all_saves(w, now_date_for_folder())
