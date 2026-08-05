#!/usr/bin/env python3
# GMFix — verify a GameMaker 8.0-built exe has the gm8x_fix patches applied.
#   usage:  python verify_patched_exe.py <game.exe> [--verbose]
# Reads the patch tables straight out of gmfix_patches.h so the check is
# always against the exact bytes the DLL applies.
import re, sys

HDR = r"C:\Project\C++\GMFix\GMFix\gmfix_patches.h"

def parse_arrays(src):
    arrays = {}
    for m in re.finditer(r"static\s+const\s+GMPatchByte\s+(\w+)\[\]\s*=\s*\{(.*?)\n\};", src, re.S):
        items = []
        for pm in re.finditer(r"\{\s*(0x[0-9A-Fa-f]+|-1)\s*,\s*0x([0-9A-Fa-f]+)\s*,\s*0x([0-9A-Fa-f]+)\s*\}", m.group(2)):
            pos = int(pm.group(1), 16)
            if pos != -1:
                items.append((pos, int(pm.group(2),16), int(pm.group(3),16)))
        arrays[m.group(1)] = items
    return arrays

def parse_names(src):
    # g_gm80_patches[] entries: { <name>, "label" }
    out = {}
    m = re.search(r"g_gm80_patches\[\]\s*=\s*\{(.*?)\};", src, re.S)
    if m:
        for pm in re.finditer(r"\{\s*(\w+)\s*,\s*\"([^\"]+)\"", m.group(1)):
            out[pm.group(1)] = pm.group(2)
    return out

def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    exe_path, verbose = sys.argv[1], ("--verbose" in sys.argv)
    h = open(HDR, encoding="utf-8").read()
    arrays, names = parse_arrays(h), parse_names(h)

    exe = open(exe_path, "rb").read()
    print(f"checking {exe_path} ({len(exe)} bytes)")
    all_done = True
    for name, label in names.items():
        items = arrays.get(name, [])
        if not items:
            continue
        # discriminate only on bytes that actually change (orig != new)
        changed = [(pos, o, n) for pos, o, n in items if o != n]
        miss = [pos for pos, o, n in changed if pos >= len(exe) or exe[pos] != n]
        if not miss:
            print(f"  [OK]    {label} ({name}) - applied")
        else:
            all_done = False
            print(f"  [MISS]  {label} ({name}) - {len(changed)-len(miss)}/{len(changed)} changed bytes patched")
            if verbose:
                for pos in miss[:8]:
                    got = exe[pos] if pos < len(exe) else None
                    print(f"          @0x{pos:X}: got {got:02X}")
    print("\nALL PATCHES APPLIED OK" if all_done else "\nSOME PATCHES MISSING")
    return 0 if all_done else 1

if __name__ == "__main__":
    sys.exit(main())
