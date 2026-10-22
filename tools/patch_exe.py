#!/usr/bin/env python3
"""Byte patches applied to sh4.exe before recompilation: code changes in the middle of a function (whole functions
are replaced in src/game/sh4.c instead). Every site is checked against the original bytes.

Usage: patch_exe.py game/sh4.exe build/sh4_ws.exe
"""
import struct, sys

BASE = 0x400000


def p32(v):
    return struct.pack('<I', v & 0xFFFFFFFF)


def call(at, target):
    return b'\xE8' + struct.pack('<i', target - (at + 5))


# (va, original bytes, replacement, what)
PATCHES = [
]


def file_offset(d, va):
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    n, opt = struct.unpack_from('<H', d, pe + 6)[0], struct.unpack_from('<H', d, pe + 20)[0]
    for i in range(n):
        vs, rva, rs, ro = struct.unpack_from('<IIII', d, pe + 24 + opt + 40 * i + 8)
        if rva <= va - BASE < rva + rs:
            return va - BASE - rva + ro
    raise ValueError(f'{va:#x} is not in the file')


def main():
    d = bytearray(open(sys.argv[1], 'rb').read())
    for va, old, new, what in PATCHES:
        o = file_offset(d, va)
        if d[o:o + len(old)] != old:
            sys.exit(f'{va:#x} ({what}): expected {old.hex()}, found {d[o:o + len(old)].hex()}')
        d[o:o + len(new)] = new
    open(sys.argv[2], 'wb').write(d)
    print(f'{len(PATCHES)} patches -> {sys.argv[2]}')


if __name__ == '__main__':
    main()
