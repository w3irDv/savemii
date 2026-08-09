#!/usr/bin/env python3
# Standalone probe: can USB save storage be reached / mounted via the wupserver?
#
#     python3 test_usb.py <console-ip>
#
# Run it from the folder that contains wupclient.py (it imports it).
# READ-ONLY: it only lists directories and *attempts* mounts. It never writes,
# deletes, or formats anything.

import sys

try:
    from wupclient import wupclient
except ImportError:
    print("error: run this from the folder that contains wupclient.py")
    sys.exit(1)

USB_VOLS = ["storage_usb01", "storage_usb02"]
USB_DEVS = ["/dev/usb01", "/dev/usb02"]


def opendir(w, fsa, path):
    """Try to open a directory; print the result and (if reachable) its entries."""
    try:
        ret, dh = w.FSA_OpenDir(fsa, path)
    except Exception as exc:
        print("    %-30s : ERROR %r" % (path, exc))
        return False
    if ret == 0:
        w.FSA_CloseDir(fsa, dh)
        entries = w.silent_ls(path, True) or []
        names = ", ".join(e["name"] for e in entries[:12])
        print("    %-30s : REACHABLE  (%d entries: %s)" % (path, len(entries), names))
        return True
    print("    %-30s : not reachable (ret=%08X)" % (path, ret))
    return False


def main():
    if len(sys.argv) < 2:
        print("usage: python3 test_usb.py <console-ip>")
        sys.exit(1)

    w = wupclient(sys.argv[1])
    fsa = w.get_fsa_handle()
    if not isinstance(fsa, int):
        print("error: could not open /dev/fsa (handle=%r)" % fsa)
        return
    print("connected. /dev/fsa handle = %s\n" % hex(fsa))

    reachable = False

    print("== 1) is USB already mounted by the recovery environment? ==")
    for vol in USB_VOLS:
        if opendir(w, fsa, "/vol/%s" % vol):
            if opendir(w, fsa, "/vol/%s/usr/save" % vol):
                reachable = True

    print("\n== 2) try to FSA_Mount the USB device nodes ==")
    for dev, vol in zip(USB_DEVS, USB_VOLS):
        volpath = "/vol/%s" % vol
        try:
            h = w.open("/dev/fsa", 0)
            ret = w.FSA_Mount(h, dev, volpath, 2)
            w.close(h)
        except Exception as exc:
            print("    mount %-12s -> %-22s : ERROR %r" % (dev, volpath, exc))
            continue
        print("    mount %-12s -> %-22s : ret=%08X" % (dev, volpath, ret))
        if ret == 0 and opendir(w, fsa, volpath + "/usr/save"):
            reachable = True

    print("\n== verdict ==")
    if reachable:
        print("USB save storage IS reachable -> USB backup can be supported.")
    else:
        print("USB save storage NOT reachable in this environment.")
        print("(No USB attached, or this recovery IOSU doesn't expose/mount the")
        print(" encrypted USB volume.)")


if __name__ == "__main__":
    main()
