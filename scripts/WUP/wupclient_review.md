# Code Review — `wupclient.py`

Review of `savemii/scripts/wupclient.py`, a Wii U FSA/MCP RPC client that talks
to a homebrew server over a TCP socket. Findings are grouped by severity. Each
entry gives the problem, the risk it poses, and how to fix it. Status reflects
what has already been patched in the working tree.

Legend: ✅ fixed · ⬜ open

---

## Correctness

### 1. Instance methods used the module global `w` instead of `self` ✅
- **Location:** `mkdir` (~L400), `chmod` (~L411), `mkquota` (~L419), `chown` (~L427)
- **Problem:** These methods called `w.FSA_*(...)` — the module-level global
  instance — instead of `self.FSA_*(...)`.
- **Risk:** Only works because a global `w` happens to exist and is the same
  object. Any second instance (`c = wupclient(); c.mkquota(...)`) raises
  `NameError`, and if `w` is ever rebound the call silently targets the wrong
  socket.
- **Fix:** Replace `w.` with `self.` in all four methods. *(applied)*

### 2. `fr` looped forever past EOF and over-read ✅
- **Location:** `fr` (~L623–640)
- **Problem:** The loop's only exit was `if len(buffer) >= size`. On a file
  shorter than `size`, `FSA_ReadFile` returns 0 at EOF, `buffer` stops growing,
  and the loop spins forever. It also always requested a full `block_size`,
  reading up to `block_size-1` bytes past `size`.
- **Risk:** Hard hang when reading past end-of-file; over-read of adjacent data.
- **Fix:** Cap each read at the remaining bytes and break on EOF/error:
  ```python
  while len(buffer) < size:
      cur_size = min(block_size, size - len(buffer))
      ret, data = self.FSA_ReadFile(fsa_handle, file_handle, 0x1, cur_size)
      if ret == 0 or ret > cur_size:  # EOF or error code
          break
      buffer += data[:ret]
      if ret < cur_size:              # short read = EOF
          break
  ```
  *(applied)*

### 3. `dump_syslog` crashed on non-ASCII bytes ✅
- **Location:** `dump_syslog` (~L388)
- **Problem:** `print(data.decode("ascii"))` on a mostly-binary 0x40000-byte
  region raises `UnicodeDecodeError` on the first byte > 0x7F.
- **Risk:** The syslog dump aborts almost immediately with an exception.
- **Fix:** `data.decode("ascii", errors="replace")`. *(applied)*

### 4. Unchecked `cd` returns in `restore_mii_maker` ✅
- **Location:** `restore_mii_maker` (~L1090, L1096, L1102, L1110)
- **Problem:** `cd` returns `-1` and leaves `cwd` unchanged on failure, but the
  returns were ignored. A failed `cd` left later `chmod`/`chown`/`mkquota`/`up`
  running against the previous directory.
- **Risk:** Destructive save-data restore could quota/chown/upload into the
  wrong directory.
- **Fix:** Guard each `cd` and bail with a message on non-zero, matching the
  pattern already in `wipe_mii_maker`. *(applied)*

### 5. `send` / `send_and_exit` used `socket.send` (partial-write risk) ✅
- **Location:** `send` (~L49), `send_and_exit` (~L67)
- **Problem:** `socket.send()` returns the number of bytes queued and may do a
  short write, silently dropping the tail of a command frame.
- **Risk:** Larger `write()`/`load_buffer` requests can be truncated on the
  wire, corrupting the command. Latent today because current frames are small.
- **Fix:** Use `self.s.sendall(request)` on both call sites. *(applied)*

---

## Correctness (continued)

### 6. `recv` read the reply with a single `recv(0x600)` ✅
- **Location:** `send` (~L50), `read` (~L58)
- **Problem:** A single `recv` assumes the whole response arrives in one TCP
  segment. A reply split across segments (or larger than 0x600) returns short.
- **Risk:** Truncated payloads / `struct.unpack` failures for large reads. Of the
  six commands, only `read` (command 1) returns a large, variable-length payload
  (`4 + len` bytes); every other command returns a tiny fixed reply that arrives
  whole.
- **Fix:** `send` now takes an optional `resp_len`; when set, it loops `recv`
  until exactly that many bytes are received. `read` passes `4 + len`, so it
  consumes precisely the status word plus payload — no truncation, and no stray
  bytes left to desync the next reply. Small commands keep the single-`recv`
  path. *(applied)*

---

## Open — robustness

### 7. `fw` ignores its `offset` parameter ⬜
- **Location:** `fw` (~L642)
- **Problem:** `fw(self, filename, offset, buffer)` opens `"r+"` and writes from
  position 0 — `offset` is never used (no seek primitive exists in the file).
  `fr` had the same dead-parameter issue (its loop was fixed, but `offset` there
  is likewise still unused).
- **Risk:** A caller expecting a patch-at-offset write silently clobbers from the
  start of the file.
- **Fix:** Add an `FSA_SetPosFile` wrapper (FSA ioctl `0x18`) and seek before
  writing/reading; or, if random access isn't needed, drop the `offset`
  parameter so the signature stops implying it.

### 8. `silent_cd` fell back to the noisy `cd` ✅
- **Location:** `silent_cd` (~L445)
- **Problem:** For a relative path it did `return self.cd(...)`, which prints
  `"cd error : ..."` on failure — defeating the "silent" contract.
- **Risk:** Harmless today (every caller passes an absolute path) but a trap for
  the next caller.
- **Fix:** Recurse into `self.silent_cd(self.cwd + "/" + path)` instead. *(applied)*

### 9. FSA error codes treated as byte counts in transfer loops — `cp` ✅ / `dl_buf` ⬜
- **Location:** `cp`, `dl_buf`
- **Problem:** The FSA return value is used directly as a length (`k += ret`,
  `data[:ret]`, `if ret < block_size`). On a mid-transfer error the code comes
  back as a large unsigned value (from `struct.unpack(">I", ...)`), which is not
  `< block_size`.
- **Risk:** A transient FSA error turns into an infinite loop or a bad slice
  instead of a clean failure.
- **Fix:** `cp` now raises `IOError` when the read count is `> block_size` (error
  code) or a write is short, breaks on the read count for EOF, and releases the
  buffer/handles in `finally`. *(applied to `cp`; `dl_buf` still uses the old
  pattern but isn't on the backup path.)*

### 10. Local file handles not closed in `dl` / `up` ⬜
- **Location:** `dl` (~L605, L611), `up` (~L760)
- **Problem:** `open(local_filename, "wb").write(buf)` and
  `f = open(local_filename, "rb")` are never explicitly closed.
- **Risk:** Works under CPython refcounting but leaks handles on other runtimes
  and isn't guaranteed to flush.
- **Fix:** Use `with open(...) as f:` blocks.

### 11. `copy_string` can silently grow the buffer past its fixed size ⬜
- **Location:** `copy_string` (~L14–16)
- **Problem:** `buffer[offset:offset+len(s)] = bytearray(...)` is a slice
  assignment; a string longer than the intended field extends the bytearray
  beyond its allocated size (e.g. past 0x520) and shifts every following field.
- **Risk:** An over-long path corrupts the ioctl struct layout rather than
  erroring. Low likelihood — all paths here are controlled — but there is no
  bounds guard.
- **Fix:** Assert/clamp `len(s)` against the field width, or write into a fixed
  slice and raise if the string doesn't fit.

---

## Open — minor / cosmetic

### 12. Parameters named `len` shadow the builtin ⬜
- **Location:** `read` (~L56), `memcpy` (~L103)
- **Fix:** Rename to `length`/`size` for clarity. No functional impact.

### 13. `read_and_print` throws on sizes not divisible by 4 ⬜
- **Location:** `read_and_print` (~L1005)
- **Problem:** `struct.unpack(">%dI" % (len(data)//4), data)` fails if `len(data)`
  isn't a multiple of 4.
- **Fix:** Truncate `data` to `len(data) - (len(data) % 4)` before unpacking, or
  require callers to pass word-aligned sizes.

### 14. `get_tik_keys` doesn't length-check the first ticket ⬜
- **Location:** `get_tik_keys` (~L908–914)
- **Problem:** The concatenated-ticket path guards with `if len(curTik) > 0x354`,
  but the initial parse slices the titleID/titleKey with no length check.
- **Risk:** A short/truncated `.tik` yields a silently truncated title key
  (security-relevant data) instead of an error.
- **Fix:** Verify `len(curTik) >= 0x1E4` before extracting, and skip/warn
  otherwise.

### 15. Inconsistent FSA out-buffer sizes ⬜
- **Location:** `FSA_MakeQuota` (~L304, `0x297`), `FSA_ChangeOwner` (~L316, `0x29B`)
- **Problem:** Most FSA calls use `0x293`; these two differ.
- **Risk:** None — just over-allocates the output buffer. Cosmetic inconsistency.
- **Fix:** Normalize to `0x293`, or add a comment if the sizes came from a
  specific reference.

---

## Summary

| # | Finding | Severity | Status |
|---|---------|----------|--------|
| 1 | Global `w` instead of `self` | High | ✅ |
| 2 | `fr` infinite loop / over-read | High | ✅ |
| 3 | `dump_syslog` decode crash | Medium | ✅ |
| 4 | Unchecked `cd` in `restore_mii_maker` | High | ✅ |
| 5 | `send`/`send_and_exit` partial write | Medium | ✅ |
| 6 | Single-`recv` reply read | Medium | ✅ |
| 7 | `fw` ignores `offset` | Medium | ⬜ |
| 8 | `silent_cd` noisy fallback | Low | ✅ |
| 9 | Error codes used as byte counts | Medium | ✅ `cp` / ⬜ `dl_buf` |
| 10 | Unclosed local file handles | Low | ⬜ |
| 11 | `copy_string` unbounded slice write | Low | ⬜ |
| 12 | `len` parameter shadowing | Cosmetic | ⬜ |
| 13 | `read_and_print` non-word-aligned size | Cosmetic | ⬜ |
| 14 | `get_tik_keys` missing length check | Low | ⬜ |
| 15 | Inconsistent FSA out-buffer sizes | Cosmetic | ⬜ |

Line numbers are approximate (`~`) since the file is being edited; search by
method name for the exact location.
