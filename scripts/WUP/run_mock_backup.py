#!/usr/bin/env python3
# Drive backup_saves.py against a local Wii U NAND copy using MockWupclient,
# writing a real batch backup into <root>/SD for inspection. No console needed.
#
# Usage:
#   python3 run_mock_backup.py [root] [--limit N]
#
#   root      local dir containing mlc01/ (and where SD/ is written);
#             default /mnt/c/Users/Xavier/repo/cemu
#   --limit N back up at most N saves per storage (fast smoke test)
#
# If a mock_stat.json (overlay of permission exceptions) sits next to this
# script, it is loaded so the StatManager sidecar records those exceptions.

import json
import os
import sys

import backup_saves as b
from mock_wupclient import MockWupclient

DEFAULT_ROOT = "/mnt/c/Users/Xavier/repo/cemu"


def load_overlay():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mock_stat.json")
    if not os.path.exists(path):
        return None
    with open(path) as f:
        raw = json.load(f)
    # allow hex strings ("0x660") or ints for the numeric fields
    overlay = {}
    for rel, fields in raw.items():
        overlay[rel] = {k: (int(v, 0) if isinstance(v, str) else v)
                        for k, v in fields.items()}
    print("loaded stat overlay: %d entries" % len(overlay))
    return overlay


def apply_limit(limit):
    """Monkeypatch enumerate_saves to yield at most `limit` saves per storage."""
    original = b.enumerate_saves

    def limited(w, storage):
        n = 0
        for pair in original(w, storage):
            if n >= limit:
                return
            n += 1
            yield pair

    b.enumerate_saves = limited


def main():
    root = DEFAULT_ROOT
    limit = None
    args = sys.argv[1:]
    i = 0
    while i < len(args):
        if args[i] == "--limit":
            limit = int(args[i + 1])
            i += 2
        else:
            root = args[i]
            i += 1

    if not os.path.isdir(os.path.join(root, "mlc01")):
        print("error: %s/mlc01 not found" % root)
        sys.exit(1)

    if limit is not None:
        apply_limit(limit)

    w = MockWupclient(root, stat_overlay=load_overlay())
    print("mock root: %s   ->  SD at %s/SD" % (root, root))
    b.backup_all_saves(w, b.now_date_for_folder())


if __name__ == "__main__":
    main()
