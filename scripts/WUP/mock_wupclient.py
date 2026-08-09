# Filesystem-backed mock of wupclient, for testing backup_saves.py on a PC
# without a real Wii U / wupserver.
#
# It maps FSA volume paths (/vol/storage_mlc01, /vol/storage_sdcard, ...) onto
# directories under a local root, and implements exactly the method surface that
# backup_saves.py and its helpers use. Because backup_all_saves() takes the
# client as a parameter, this drives the real backup logic unmodified.
#
# A Linux copy of a Wii U NAND has no IOSU owner/group/mode/quota, so FSA_ReadDir
# stats are synthesized (see _make_unk): by default they equal SaveMii's Wii U
# default (so nothing is a stat "exception"); an optional overlay lets a test
# inject exceptions.
#
# Usage: see run_mock_backup.py

import os
import re
import shutil
import struct

# FSStatFlags (coreinit/filesystem.h)
FS_STAT_DIRECTORY = 0x80000000
FS_STAT_QUOTA = 0x60000000
FS_STAT_FILE = 0x01000000

DEFAULT_MODE = 0x660

# vWii stat defaults (set_default_stat_cfg_for_vwii_savedata)
VWII_DEFAULT_MODE = 0x666
VWII_DEFAULT_GID = 0x400
SAVEMII_TITLE_ID = 0

_SAVE_RE = re.compile(r"^/usr/save/([0-9a-fA-F]{8})/([0-9a-fA-F]{8})(?:/|$)")
_VWII_RE = re.compile(r"^/title/[0-9a-fA-F]{8}/[0-9a-fA-F]{8}(?:/|$)")


class MockWupclient:
    def __init__(self, root="/mnt/c/Users/Xavier/repo/cemu", stat_overlay=None,
                 mounts=None):
        self.root = root
        self.mounts = {
            "storage_mlc01": os.path.join(root, "mlc01"),
            "storage_sdcard": os.path.join(root, "SD"),
            "storage_usb01": os.path.join(root, "usb01"),
            "storage_usb02": os.path.join(root, "usb02"),
            "storage_slccmpt01": os.path.join(root, "slccmpt01"),
        }
        if mounts:  # override individual volumes (e.g. redirect SD/USB in tests)
            self.mounts.update(mounts)
        self.fsa_handle = 1
        self._files = {}       # file handle -> python file object
        self._file_paths = {}  # file handle -> fsa path (for FSA_GetStatFile)
        self._next_fh = 100
        self._dirs = {}        # dir handle -> resolved path
        self._next_dh = 100
        self._overlay = stat_overlay or {}   # device-rel path -> {mode,owner,group,quota}
        self._group_cache = {}               # (hi,lo) -> group_id

    # --- path mapping -------------------------------------------------------

    def _resolve(self, fsa_path):
        """Map an FSA path to a local path, or None if the volume is unknown."""
        if not fsa_path.startswith("/vol/"):
            return None
        rest = fsa_path[len("/vol/"):]
        parts = rest.split("/", 1)
        vol = parts[0]
        if vol not in self.mounts:
            return None
        sub = parts[1] if len(parts) > 1 else ""
        base = self.mounts[vol]
        return os.path.join(base, sub) if sub else base

    def _device_rel(self, fsa_path):
        """Strip /vol/<storage> -> device-relative path (as SaveMii stores)."""
        rest = fsa_path[len("/vol/"):]
        parts = rest.split("/", 1)
        return "/" + parts[1] if len(parts) > 1 else "/"

    # --- handles / mounts (no-ops on Linux) ---------------------------------

    def get_fsa_handle(self):
        return self.fsa_handle

    def open(self, device, mode):
        return self.fsa_handle  # /dev/fsa, /dev/mcp, ... -> dummy handle

    def close(self, handle):
        return 0

    def FSA_Mount(self, handle, device_path, volume_path, flags):
        return 0  # volumes are always "mounted" via the mount table

    def FSA_FlushVolume(self, handle, path):
        return 0

    # --- directories --------------------------------------------------------

    def FSA_MakeDir(self, handle, path, flags):
        if path == "/vol":
            return 0
        local = self._resolve(path)
        if local is None:
            return 0xFFFFFFFF
        try:
            os.mkdir(local)
            return 0
        except FileExistsError:
            return 0
        except OSError:
            return 0xFFFFFFFF

    def FSA_OpenDir(self, handle, path):
        if path == "/vol":
            dh = self._next_dh
            self._next_dh += 1
            self._dirs[dh] = None
            return (0, dh)
        local = self._resolve(path)
        if local is None or not os.path.isdir(local):
            return (0xFFFFFFFF, 0)
        dh = self._next_dh
        self._next_dh += 1
        self._dirs[dh] = local
        return (0, dh)

    def FSA_CloseDir(self, handle, dir_handle):
        self._dirs.pop(dir_handle, None)
        return 0

    # --- files --------------------------------------------------------------

    def FSA_OpenFile(self, handle, path, mode):
        local = self._resolve(path)
        if local is None:
            return (0xFFFFFFFF, 0)
        pymode = {"r": "rb", "w": "wb", "r+": "rb+"}.get(mode, "rb")
        try:
            f = open(local, pymode)
        except OSError:
            return (0xFFFFFFFF, 0)
        fh = self._next_fh
        self._next_fh += 1
        self._files[fh] = f
        self._file_paths[fh] = path
        return (0, fh)

    def FSA_ReadFile(self, handle, file_handle, size, cnt):
        f = self._files.get(file_handle)
        if f is None:
            return (0xFFFFFFFF, bytearray())
        data = f.read(size * cnt)
        return (len(data), bytearray(data))

    def FSA_WriteFile(self, handle, file_handle, data):
        f = self._files.get(file_handle)
        if f is None:
            return 0xFFFFFFFF
        f.write(bytes(data))
        return len(data)

    def FSA_CloseFile(self, handle, file_handle):
        f = self._files.pop(file_handle, None)
        self._file_paths.pop(file_handle, None)
        if f is not None:
            f.close()
        return 0

    def FSA_GetStatFile(self, handle, file_handle):
        """Handle-based stat -> 25 words (4-byte prefix + FSAStat), matching the
        real client, synthesized from the same _make_unk as FSA_GetStat/readdir."""
        path = self._file_paths.get(file_handle)
        if path is None:
            return (0xFFFFFFFF, tuple([0] * 25))
        unk = self._make_unk(path, False)
        words = struct.unpack(">25I", b"\x00\x00\x00\x00" + bytes(unk[0:0x60]))
        return (0, words)

    # --- stat synthesis -----------------------------------------------------

    def _group_id(self, hi, lo):
        """Read group_id from a save's meta.xml (cached), matching resolve_meta."""
        key = (hi, lo)
        if key in self._group_cache:
            return self._group_cache[key]
        gid = 0
        meta = self._resolve("/vol/storage_mlc01/usr/save/%s/%s/meta/meta.xml" % (hi, lo))
        try:
            with open(meta, "rb") as f:
                xml = f.read().decode("utf-8", errors="replace")
            m = re.search(r"group_id[^>]*>([^<]*)<", xml)
            if m and m.group(1).strip():
                gid = int(m.group(1).strip(), 16)
        except OSError:
            pass
        self._group_cache[key] = gid
        return gid

    def _make_unk(self, fsa_path, is_dir):
        """Build a big-endian FSAStat (0x64 bytes) for an entry."""
        rel = self._device_rel(fsa_path)
        ov = self._overlay.get(rel)

        flags = FS_STAT_DIRECTORY if is_dir else FS_STAT_FILE
        if ov and ov.get("quota"):
            flags |= FS_STAT_QUOTA

        if ov is not None:
            mode = ov.get("mode", DEFAULT_MODE)
            owner = ov.get("owner", 0)
            group = ov.get("group", 0)
            quota = ov.get("quota", 0)
        else:
            # default: equal to SaveMii's default for that save type so the
            # entry is not recorded as an exception
            mode = DEFAULT_MODE
            owner = 0
            group = 0
            quota = 0
            m = _SAVE_RE.match(rel)
            if m:  # Wii U: uid=lowID, gid=group_id, mode=0x660
                hi, lo = m.group(1).lower(), m.group(2).lower()
                owner = int(lo, 16)
                group = self._group_id(hi, lo)
            elif _VWII_RE.match(rel):  # vWii: uid=savemii_title_id, gid=0x400, 0x666
                mode = VWII_DEFAULT_MODE
                owner = SAVEMII_TITLE_ID
                group = VWII_DEFAULT_GID

        # FSStat: flags, mode, owner, group, size, allocSize (6x u32), quotaSize (u64)
        return bytearray(
            struct.pack(">IIIIII", flags, mode, owner, group, 0, 0)
            + struct.pack(">Q", quota)
            + b"\x00" * (0x64 - 32)
        )

    def FSA_GetStat(self, handle, path):
        """Path-based stat: synthesize the same FSAStat as the readdir entry."""
        local = self._resolve(path)
        if local is None or not os.path.exists(local):
            return (0xFFFFFFFF, bytearray(0x64))
        return (0, self._make_unk(path, os.path.isdir(local)))

    # --- listing / copy -----------------------------------------------------

    def silent_ls(self, path=None, return_data=False):
        if path is None:
            path = "/"
        local = self._resolve(path)
        if local is None or not os.path.isdir(local):
            return [] if return_data else None
        entries = []
        for name in sorted(os.listdir(local)):
            child_local = os.path.join(local, name)
            is_dir = os.path.isdir(child_local)
            if not return_data:
                print("     %s%s" % (name, "" if is_dir else ""))
                continue
            entries.append({
                "name": name,
                "is_file": not is_dir,
                "unk": self._make_unk(path.rstrip("/") + "/" + name, is_dir),
            })
        return entries if return_data else None

    def cp(self, filename_in, filename_out):
        src = self._resolve(filename_in)
        dst = self._resolve(filename_out)
        if src is None or dst is None:
            raise IOError("cp: unmapped path %s -> %s" % (filename_in, filename_out))
        shutil.copyfile(src, dst)  # raises OSError on any read/write failure

    def cpdir(self, srcpath, dstpath):
        src = self._resolve(srcpath)
        dst = self._resolve(dstpath)
        if src is None or dst is None:
            raise IOError("cpdir: unmapped path %s -> %s" % (srcpath, dstpath))
        # shutil.copytree raises OSError/shutil.Error on any read/write failure
        shutil.copytree(src, dst, dirs_exist_ok=True)

    def rmtree(self, path):
        # recursive, non-interactive delete (used for backup rollback)
        local = self._resolve(path)
        if local is not None:
            shutil.rmtree(local, ignore_errors=True)

    def rename(self, path_old, path_new):
        # atomic move (mirrors wupclient.rename / FSA_Rename)
        src = self._resolve(path_old)
        dst = self._resolve(path_new)
        if src is None or dst is None:
            raise IOError("rename: unmapped path %s -> %s" % (path_old, path_new))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        os.rename(src, dst)
