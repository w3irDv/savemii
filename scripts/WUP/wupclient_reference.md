# `wupclient.py` — Function Reference

`wupclient.py` is a Python client (inspired by plutoo's ctrrpc) that drives a
homebrew RPC server running on a Wii U over a TCP socket. It lets a PC read and
write the console's memory, invoke IOS syscalls, and — through the FSA and MCP
IOS modules — manipulate the console's filesystem and installed titles.

The RPC server exposes six low-level commands, keyed by the first word of every
request:

| cmd | meaning |
|-----|---------|
| 0 | write memory |
| 1 | read memory |
| 2 | invoke a syscall (`svc`) |
| 3 | kill the server |
| 4 | `memcpy` in console memory |
| 5 | repeated-word write (memset-like) |

Everything else is built on top of these: syscalls open IOS device handles
(`/dev/fsa`, `/dev/mcp`, `/dev/nim`), and `ioctl`/`ioctlv` on those handles
expose the FSA_* and MCP_* operations.

**Conventions used below**
- *handle* = an IOS device handle returned by `open`.
- *fsa_handle* = a handle to `/dev/fsa` (obtained via `get_fsa_handle`).
- Return values from FSA/MCP calls are IOS status codes: `0` = success,
  non-zero = error (usually printed in hex).
- `mode`/`flags` for files and directories are POSIX-style octal permission
  words (e.g. `0x660`, `0x644`).
- Owner/group IDs are IOS title/permission IDs, not Unix uids.

---

## 1. Module-level byte-buffer helpers

These build the fixed-layout request buffers that FSA/MCP ioctls expect.

| Function | Arguments | Purpose |
|----------|-----------|---------|
| `buffer(size)` | `size` — length in bytes | Return a zero-filled `bytearray` of `size` bytes. |
| `copy_string(buffer, s, offset)` | `buffer` target, `s` string, `offset` | Write NUL-terminated ASCII `s` into `buffer` at `offset`. |
| `copy_word(buffer, w, offset)` | `buffer`, `w` 32-bit int, `offset` | Write `w` as a big-endian 4-byte word at `offset`. |
| `copy_dword(buffer, w, offset)` | `buffer`, `w` 64-bit int, `offset` | Write `w` as a big-endian 8-byte value at `offset`. |
| `get_string(buffer, offset)` | `buffer`, `offset` | Read a NUL-terminated UTF-8 string starting at `offset`. |
| `mkdir_p(path)` | `path` — local PC path | Create a local directory tree, ignoring "already exists" (like `mkdir -p`). |

---

## 2. `class wupclient`

### Lifecycle

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `__init__(ip, port)` | `ip` (default `'YOUR.IP.HE.RE'`), `port` (default `1337`) | Open the TCP socket to the console, reset the FSA handle, set `cwd` to `/vol/storage_mlc01`. |
| `__del__()` | — | On garbage collection, close the FSA handle if one is open. |

### Fundamental comms

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `send(command, data, resp_len=None)` | `command` — cmd id; `data` — payload bytes; `resp_len` — expected reply length | Send a request and return `(status, payload)`. If `resp_len` is given, loop `recv` until exactly that many bytes arrive (used for large replies); otherwise do a single `recv`. |
| `send_and_exit(command, data)` | `command`, `data` | Send a final request, close the socket, and terminate the process. Used for commands (shutdown/reset) after which the server won't reply. |

### Core commands (map 1:1 to the six server opcodes)

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `read(addr, len)` | `addr` — memory address; `len` — byte count | Read `len` bytes of console memory at `addr`. Returns the bytes, or `None` on error. |
| `write(addr, data)` | `addr`; `data` — bytes | Write `data` into console memory at `addr`. |
| `svc(svc_id, arguments)` | `svc_id` — IOS syscall number; `arguments` — list of 32-bit ints | Invoke an IOS syscall and return its (single 32-bit) result. |
| `svc_and_exit(svc_id, arguments)` | as `svc` | Invoke a syscall then exit (for shutdown/reset). |
| `kill()` | — | Tell the RPC server to terminate. |
| `memcpy(dst, src, len)` | `dst`, `src` addresses; `len` bytes | Copy memory within the console. |
| `repeatwrite(dst, val, n)` | `dst` address; `val` 32-bit word; `n` count | Write `val` `n` times starting at `dst` (memset-style fill). |

### Memory / handle helpers ("derivatives")

Built on `svc` to manage console-side memory and IOS handles.

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `alloc(size, align=None)` | `size`; optional `align` | Allocate console memory (svc 0x27/0x28). Returns the address. |
| `free(address)` | `address` | Free previously allocated console memory (svc 0x29). |
| `load_buffer(b, align=None)` | `b` — bytes; optional `align` | Allocate console memory and copy `b` into it; returns the address. |
| `load_string(s, align=None)` | `s` — string; optional `align` | Like `load_buffer` for a NUL-terminated ASCII string. |
| `open(device, mode)` | `device` — e.g. `/dev/fsa`; `mode` | Open an IOS device handle (svc 0x33). |
| `close(handle)` | `handle` | Close an IOS handle (svc 0x34). |
| `ioctl(handle, cmd, inbuf, outbuf_size)` | `handle`; `cmd` ioctl number; `inbuf` bytes; `outbuf_size` | Issue an IOS `ioctl`. Returns `(status, out_bytes)`. |
| `iovec(vecs)` | `vecs` — list of `(addr, size)` | Build an IOS iovec array in console memory; returns its address. |
| `ioctlv(handle, cmd, inbufs, outbuf_sizes, inbufs_ptr=[], outbufs_ptr=[])` | see below | Issue an IOS `ioctlv` (vectored ioctl). Returns `(status, [out_buffers])`. |

`ioctlv` arguments: `inbufs` are byte buffers copied to the console; `outbuf_sizes`
are sizes to allocate for outputs; `inbufs_ptr`/`outbufs_ptr` are pre-existing
`(addr, size)` console-memory regions used directly (used by `FSA_ReadFilePtr`/
`FSA_WriteFilePtr` to stream through a fixed buffer without re-copying).

### FSA operations (filesystem, via `/dev/fsa`)

Each wraps one FSA ioctl. All take a `handle` to `/dev/fsa` as the first arg.

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `FSA_Mount(handle, device_path, volume_path, flags)` | device node, mount point, flags | Mount a storage device at a volume path. |
| `FSA_Unmount(handle, path, flags)` | volume path, flags | Unmount a volume. |
| `FSA_RawOpen(handle, device)` | device node | Open a device for raw block access; returns `(status, raw_handle)`. |
| `FSA_OpenDir(handle, path)` | directory path | Open a directory; returns `(status, dir_handle)`. |
| `FSA_ReadDir(handle, dir_handle)` | dir handle | Read the next directory entry; returns `(status, {name, is_file, unk})`. |
| `FSA_CloseDir(handle, dir_handle)` | dir handle | Close a directory handle. |
| `FSA_OpenFile(handle, path, mode)` | path, mode (`"r"`, `"w"`, `"r+"`) | Open a file; returns `(status, file_handle)`. |
| `FSA_MakeDir(handle, path, flags)` | path, permission flags | Create a directory. |
| `FSA_ReadFile(handle, file_handle, size, cnt)` | file handle, element size, count | Read `size*cnt` bytes into a server buffer; returns `(count, data)`. |
| `FSA_WriteFile(handle, file_handle, data)` | file handle, bytes | Write `data` to a file (element size 1, count = len). |
| `FSA_ReadFilePtr(handle, file_handle, size, cnt, ptr)` | ...`ptr` — console buffer addr | Read directly into an existing console-memory buffer (used by `cp`). |
| `FSA_WriteFilePtr(handle, file_handle, size, cnt, ptr)` | ...`ptr` — console buffer addr | Write directly from an existing console-memory buffer (used by `cp`/`df`). |
| `FSA_GetStatFile(handle, file_handle)` | file handle | Stat an open file; returns `(status, tuple of 25 words)` — flags, mode, owner, group, size, quota, ... |
| `FSA_CloseFile(handle, file_handle)` | file handle | Close a file handle. |
| `FSA_ChangeMode(handle, path, mode)` | path, permission mode | `chmod` a path (mask `0x777`). |
| `FSA_MakeQuota(handle, path, mode, size)` | path, mode, quota size (bytes) | Create a quota'd save directory of a given size. *(added by qwii)* |
| `FSA_ChangeOwner(handle, path, owner, group)` | path, owner id, group id | `chown` a path. *(added by qwii)* |
| `FSA_Remove(handle, path)` | path | Delete a file or (empty) directory. |
| `FSA_Rename(handle, old, new)` | old path, new path | Atomically rename/move a file or directory (`FSARequestRename`, ioctl `0x09`). |
| `FSA_FlushVolume(handle, path)` | volume path | Flush a volume's pending writes to storage. |

### MCP operations (title management, via `/dev/mcp`)

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `MCP_InstallGetInfo(handle, path)` | source path | Query install info for a title package; returns `(status, fields)`. |
| `MCP_Install(handle, path)` | source path | Start installing a title from `path`. |
| `MCP_InstallGetProgress(handle)` | — | Poll install progress; returns `(status, 9 words)`. |
| `MCP_DeleteTitle(handle, path, flush)` | title path, flush flag | Delete an installed title. |
| `MCP_CopyTitle(handle, path, dst_device_id, flush)` | source path, target device, flush | Copy a title to another device (NAND/USB). |
| `MCP_InstallSetTargetDevice(handle, device)` | device id | Set install target device. |
| `MCP_InstallSetTargetUsb(handle, device)` | device id | Set whether the install target is USB. |

### Debug

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `dump_syslog()` | — | Read the IOS syslog buffer from a fixed memory address and print it (0x40000 bytes, non-ASCII replaced). |

### File-management convenience layer

These are the interactive, high-level commands. They auto-open the FSA handle
(`get_fsa_handle`) and resolve relative paths against `cwd`.

| Method | Arguments | Purpose |
|--------|-----------|---------|
| `get_fsa_handle()` | — | Return the cached `/dev/fsa` handle, opening it on first use. |
| `mkdir(path, flags)` | remote path, permission flags | Create a directory on the console. |
| `rename(old, new)` | remote src, remote dst | Atomically rename/move a file or directory (wraps `FSA_Rename`); raises on failure. |
| `chmod(filename, flags)` | remote path, mode | Change a path's permissions. |
| `mkquota(filename, flags, size)` | remote path, mode, size | Create a quota'd directory. *(added by qwii)* |
| `chown(filename, owner, group)` | remote path, owner, group | Change a path's owner/group. *(added by qwii)* |
| `cd(path)` | remote path | Change working directory (verifies it exists); prints an error if not. Returns `0`/`-1`. |
| `silent_cd(path)` | remote path | Like `cd` but prints nothing on failure. |
| `ls(path=None, return_data=False)` | remote path (default `cwd`); `return_data` | List a directory. Prints entries, or (if `return_data`) returns a list of entry dicts. |
| `silent_ls(path=None, return_data=False)` | as `ls` | Like `ls` but silent on open errors. |
| `dldir(path)` | remote path | Recursively download a directory tree to the PC. |
| `cpdir(srcpath, dstpath)` | remote src, remote dst | Recursively copy a directory tree on the console. |
| `pwd()` | — | Return the current working directory string. |
| `cp(filename_in, filename_out)` | remote src, remote dst | Copy a file on the console, streaming through a 64 KB console buffer. |
| `df(filename_out, src, size)` | remote dst, memory addr, size | Dump `size` bytes of console memory at `src` into a file on the console. |
| `dl_buf(filename, show_progress=True)` | remote path; progress flag | Download a console file into a Python `bytearray` and return it. |
| `dl(filename, directorypath=None, local_filename=None)` | remote path; optional local dir/name | Download a console file to the PC (path derived from args or the remote name). |
| `fr(filename, offset, size)` | remote path; `offset` (currently unused); `size` | Read up to `size` bytes from a console file and return them. |
| `fw(filename, offset, buffer)` | remote path; `offset` (currently unused); `buffer` bytes | Write `buffer` into an existing console file (opened `r+`). |
| `stat(filename)` | remote path | Print a file's flags, mode, owner, group, size and quota. |
| `askyesno()` | — | Prompt on stdin and return `True`/`False` (used to confirm destructive ops). |
| `rm(filename)` | remote path | Delete a file after a y/n confirmation (warns about brick risk). |
| `rmdir(path)` | remote path | Delete an empty directory after a y/n confirmation. |
| `silent_rm(filename)` | remote path | Delete a file with no prompt/output. |
| `silent_rmdir(path)` | remote path | Delete an empty directory with no prompt/output. |
| `up(local_filename, filename=None)` | local path; optional remote path | Upload a PC file to the console (remote name derived if omitted). |

> Note: `mkdir_p(path)` also appears as a method inside the class (a duplicate of
> the module-level helper) but is defined without `self`, so it's only usable as
> the module-level function.

---

## 3. Module-level operations

These operate on the global instance `w` (created in `__main__`). Each opens the
device it needs, performs the operation, prints results, and closes the handle.

### Mount / unmount helpers

`mount_sd` / `unmount_sd`, `mount_slccmpt01` / `unmount_slccmpt01`,
`mount_odd_content` / `unmount_odd_content`, `mount_odd_update` /
`unmount_odd_update`, `mount_odd_tickets` / `unmount_odd_tickets` — all take no
arguments and mount/unmount their respective device at a fixed `/vol/...` path:

| Pair | Device | Volume |
|------|--------|--------|
| `*_sd` | `/dev/sdcard01` | `/vol/storage_sdcard` |
| `*_slccmpt01` | `/dev/slccmpt01` | `/vol/storage_slccmpt01` (vWii SLC) |
| `*_odd_content` | `/dev/odd03` | `/vol/storage_odd_content` (disc) |
| `*_odd_update` | `/dev/odd02` | `/vol/storage_odd_update` (disc) |
| `*_odd_tickets` | `/dev/odd01` | `/vol/storage_odd_tickets` (disc) |

### Title / system operations

| Function | Arguments | Purpose |
|----------|-----------|---------|
| `get_tik_keys()` | — | Walk `/vol/system/rights/ticket/apps`, parse every `.tik` (including packed multi-tickets), and print unique `titleId titleKey` pairs. |
| `copy_title(path, installToUsb=0, flush=0)` | title root path; target; flush | Copy a title to NAND or USB (target must be deleted there first). |
| `install_title(path, installToUsb=0)` | SD-relative folder; target | Install a title from the SD card (get-info → set target → install). |
| `delete_title(path, flush=0)` | full title path; flush | Delete an installed title via MCP. |
| `ios_shutdown()` | — | Shut down the console (svc 0x72 arg 0) and exit. |
| `ios_reset()` | — | Reboot the console (svc 0x72 arg 1) and exit. |
| `get_nim_status()` | — | Query `/dev/nim` status and print the raw response. |
| `read_and_print(adr, size)` | address, size | Read console memory and print it as big-endian hex words, 4 per line. |
| `read_and_dump(adr, size)` | address, size | Read console memory and dump it to a local `dump.bin`. |
| `flush_mlc()` | — | Flush the internal NAND volume (`/vol/storage_mlc01`). |

### Mii Maker save recovery (this project's purpose)

Region argument is `"eur"`, `"usa"`, or `"jpn"`, mapped to the Mii Maker save
title id (`1004a200` / `1004a100` / `1004a000`).

| Function | Arguments | Purpose |
|----------|-----------|---------|
| `wipe_mii_maker(region)` | region | Delete the Mii Maker save database files and `common`/`db` directories, then flush. |
| `check_mii_maker_is_wiped(region)` | region | Verify the Mii Maker `user` folder is empty; otherwise list what remains. |
| `restore_mii_maker(region)` | region | Recreate the Mii Maker save: set perms/owner on `user`, create the `common` quota, upload `stadio.sav` and the `db/*.dat` files from a local `mm_bckp/` backup, fix perms/owner, then flush. Aborts if any `cd` fails. |

---

## 4. Entry point

`if __name__ == '__main__':` creates the global client `w = wupclient()` (edit
the default IP in `__init__` first) and calls `mount_sd()`. The remaining calls
(`ls`, `dump_syslog`, `install_title`, `kill`, ...) are commented-out examples;
the script is meant to be used interactively (e.g. `python -i wupclient.py`) or
edited to call the specific recovery function you need.

---

### Related documents
- `wupclient_review.md` — code-review findings (bugs, risks, fixes) for this file.
