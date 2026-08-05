// GMFix self-test: applies g_gm80_patches to a game exe using the SAME
// logic as gmfix_hook.cpp's gmfix_apply_patches, then the output is compared
// byte-for-byte against a reference built from patches.c.
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include "gmfix_patches.h"

static int apply_all(const char* path) {
    FILE* f = fopen(path, "rb+");
    if (!f) { printf("open fail\n"); return -1; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return -2; }
    std::vector<uint8_t> buf((size_t)sz);
    if (fread(buf.data(), 1, (size_t)sz, f) != (size_t)sz) { fclose(f); return -3; }
    bool changed = false;
    for (const GMPatch* patch = g_gm80_patches; patch->bytes; patch++) {
        bool ok = true;
        for (const GMPatchByte* p = patch->bytes; p->pos != -1; p++) {
            if ((size_t)p->pos >= buf.size() || buf[(size_t)p->pos] != p->orig_byte) { ok = false; break; }
        }
        if (!ok) { printf("skip '%s' (mismatch)\n", patch->name); continue; }
        for (const GMPatchByte* p = patch->bytes; p->pos != -1; p++) buf[(size_t)p->pos] = p->new_byte;
        changed = true;
        printf("apply '%s'\n", patch->name);
    }
    if (changed) {
        fseek(f, 0, SEEK_SET);
        fwrite(buf.data(), 1, buf.size(), f);
        fflush(f);
    }
    fclose(f);
    return changed ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc < 2) return 3;
    return apply_all(argv[1]);
}
