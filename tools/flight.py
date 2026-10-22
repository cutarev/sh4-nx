#!/usr/bin/env python3
"""flight.py sh4-flight.bin [last N records per thread, default 3000]: prints the flight recorder's rings
(src/runtime/rt_core.c, SH4_FLIGHT=1) oldest first, indented by guest stack depth.
E = function entry (return address, 3 args), X = its exit (eax), A = bridged call (return address, 3 args);
xN = the record repeated N times in a loop (only the first one's registers are kept)."""
import struct, sys

d = open(sys.argv[1], 'rb').read()
last = int(sys.argv[2]) if len(sys.argv) > 2 else 3000
assert d[:4] == b'SH4F'
n_names, = struct.unpack_from('<I', d, 4)
p, names = 8, []
for _ in range(n_names):
    e = d.index(b'\0', p)
    names.append(d[p:e].decode())
    p = e + 1
n_threads, = struct.unpack_from('<I', d, p)
p += 4
for _ in range(n_threads):
    tid, count, n, current = struct.unpack_from('<4I', d, p)
    p += 16
    recs = [struct.unpack_from('<IIi7I4II', d, p + 60 * k) for k in range(max(0, n - last), n)]
    p += 60 * n
    print(f'=== thread {tid:x}: {count} records{" (the one that crashed / exited)" if current else ""}')
    top = max((r[9] for r in recs), default=0)  # indent by how deep the guest stack is
    for va, kind, frame, eax, ecx, edx, ebx, esi, edi, esp, ret, a1, a2, a3, rep in recs:
        regs = (f' x{rep + 1}' if rep else '') + f'  eax={eax:08X} ecx={ecx:08X} edx={edx:08X} ebx={ebx:08X} esi={esi:08X} edi={edi:08X} esp={esp:08X}'
        depth = min((top - esp) // 48, 40)
        if kind == 0:
            print(f'f{frame} {"  " * depth}E {va:08X}({a1:08X}, {a2:08X}, {a3:08X}) <- {ret:08X}{regs}')
        elif kind == 1:
            print(f'f{frame} {"  " * depth}X {va:08X} -> {eax:08X}{regs}')
        else:
            name = names[va] if va < len(names) else f'#{va}'
            print(f'f{frame} {"  " * depth}A {name}({a1:08X}, {a2:08X}, {a3:08X}) <- {ret:08X}{regs}')
