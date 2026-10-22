/* Hand-written replacements of SILENT HILL 4.exe functions, by address: tools/regen.sh passes src/game/*.c to the
 * recompiler (--exclude-manual), which then leaves these functions out of src/recomp/gen. */
#include "../runtime/rt.h"
#include "../runtime/host.h"

/* The static CRT's memcpy (memmove semantics) and its second copy: their jump tables sit past the end
 * the disassembler finds and are indexed backwards, so the lifted copies cannot follow them. */
static void crt_memmove(void) {
    uint32_t d = ARG(0), s = ARG(1), n = ARG(2);
    memmove((void *)XBOX_PTR(d), (const void *)XBOX_PTR(s), n);
    g_eax = d;
    g_esp += 4;  /* ret: cdecl */
}
void sub_00576BC0(void) { crt_memmove(); }
void sub_0057FE90(void) { crt_memmove(); }

/* Called once after the exe is loaded. */
void game_init(void) {}

/* Indirect calls and the dispatch table reach the generated bodies (sub_X_gen) of wrapped functions: calls
 * by address are routed here first. */
recomp_func_t movie_lookup(uint32_t va);  /* movie.c */
recomp_func_t recomp_lookup_manual(uint32_t va) {
    switch (va) {
    case 0x00576BC0u: return sub_00576BC0;
    case 0x0057FE90u: return sub_0057FE90;
    }
    return movie_lookup(va);
}
