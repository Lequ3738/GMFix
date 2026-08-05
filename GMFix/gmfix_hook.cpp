// GMFix — post-build exe patcher for GameMaker 8.0.
//
// Hooks sub_5D418C, the standalone-exe write core. GM 8.0 routes every exe
// build (Create Executable AND the F5 run-temp exe) through GM80_OpenProject
// (0x5D453C) -> sub_5D418C, which writes the finished .exe to its EAX argument.
// GMSave already owns the 0x5D453C hook, so GMFix hooks the layer below.
//
// The hook: run the original sub_5D418C (via a 6-byte trampoline), then, if
// the output path ends in ".exe", apply the gm8x_fix patch tables to the file.
// Any patch whose orig bytes don't all match is skipped with a diagnostic —
// a drifted runner is never corrupted. The original's EAX status is preserved
// for GM80_OpenProject's error check.
#include "pch.h"
#include "delphi.h"
#include "gmfix_addresses.h"
#include "gmfix_patches.h"
#include "gmfix_diag.h"
#include "gm_log.h"
#include <vector>

// sub_5D418C prologue (IDA-verified):  push ebp; mov ebp,esp; push ecx;
// mov ecx,5  (5 bytes). First instruction boundary >= jmp size is 9 bytes
// (after `mov ecx,5`). Copying only 6 bytes splits `mov ecx,5` in half —
// the trampoline then decodes into the jmp displacement and crashes.
static constexpr int kHookCopyBytes = 9;

static void*   g_base = nullptr;
static void*   g_trampoline = nullptr;
static uint8_t g_orig_prologue[kHookCopyBytes] = {};
static const char* g_exe_path = nullptr;  // Delphi AnsiString data ptr (hook entry EAX)

// Copy a Delphi AnsiString (data pointer -> NUL-terminated C buffer).
static bool read_ansi_string(const char* data, char* out, size_t outsz) {
    if (!data || (uintptr_t)data < 0x10000) return false;
    uint32_t len = (uint32_t)*(int32_t*)(data - 4);
    if (len == 0 || len + 1 > outsz) return false;
    memcpy(out, data, len);
    out[len] = '\0';
    return true;
}

// Apply every patch in g_gm80_patches to the file at `path`. Each patch is
// verified (all orig bytes present) before any byte is written.
static void gmfix_apply_patches() {
    char path[MAX_PATH];
    if (!read_ansi_string(g_exe_path, path, sizeof(path))) {
        gm_log("GMFix: cannot read output path (ptr %p)", g_exe_path);
        return;
    }
    size_t plen = strlen(path);
    if (plen < 4 || _stricmp(path + plen - 4, ".exe") != 0) {
        gm_log("GMFix: not an .exe output (%s) — skip", path);
        return;
    }

    FILE* f = fopen(path, "rb+");
    if (!f) { gm_log("GMFix: cannot open %s", path); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return; }

    std::vector<uint8_t> buf((size_t)sz);
    if (fread(buf.data(), 1, (size_t)sz, f) != (size_t)sz) { fclose(f); return; }

    gmfix_diag_reset();
    bool changed = false;
    for (const GMPatch* patch = g_gm80_patches; patch->bytes; patch++) {
        bool ok = true;
        for (const GMPatchByte* p = patch->bytes; p->pos != -1; p++) {
            if ((size_t)p->pos >= buf.size() || buf[(size_t)p->pos] != p->orig_byte) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            gm_log("GMFix: skip '%s' (byte mismatch)", patch->name);
            gmfix_diag_add("跳过补丁 \"%s\": 运行器字节不匹配(可能不是标准 GM8 运行器)", patch->name);
            continue;
        }
        for (const GMPatchByte* p = patch->bytes; p->pos != -1; p++)
            buf[(size_t)p->pos] = p->new_byte;
        changed = true;
        gm_log("GMFix: applied '%s'", patch->name);
    }

    if (changed) {
        fseek(f, 0, SEEK_SET);
        fwrite(buf.data(), 1, buf.size(), f);
        fflush(f);
    }
    fclose(f);
    if (changed) gm_log("GMFix: patched %s", path);
}

// Entry hook for sub_5D418C (Delphi __usercall: EAX = .exe path, ECX = arg2).
// Runs the original via the trampoline, then patches the written file. The
// original's EAX status survives for GM80_OpenProject's error handling.
__declspec(naked) static void gmfix_hook_5D418C() {
    __asm {
        push eax
        mov  [g_exe_path], eax
        call dword ptr [g_trampoline]
        push eax                 // save original status
        pushad
        call gmfix_apply_patches
        popad
        pop  eax                 // restore status
        add  esp, 4              // drop saved path
        ret
    }
}

bool gmfix_install(void* base) {
    g_base = base;
    if (!g_base) return false;

    uint8_t* fn = (uint8_t*)g_base + GM80_SUB_5D418C_RVA;
    // Prologue must be `push ebp; mov ebp,esp; push ecx; mov ecx,5` — bail if
    // the address drifted (covers the first 5 bytes of the 9-byte copy).
    static const uint8_t kExpected[5] = { 0x55, 0x8B, 0xEC, 0x51, 0xB9 };
    if (memcmp(fn, kExpected, sizeof(kExpected)) != 0) {
        gm_log("GMFix: sub_5D418C prologue mismatch (0x%02X 0x%02X 0x%02X 0x%02X 0x%02X)",
               fn[0], fn[1], fn[2], fn[3], fn[4]);
        return false;
    }

    memcpy(g_orig_prologue, fn, kHookCopyBytes);
    g_trampoline = VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!g_trampoline) return false;
    memcpy(g_trampoline, g_orig_prologue, kHookCopyBytes);
    uint8_t* t = (uint8_t*)g_trampoline;
    t[kHookCopyBytes] = 0xE9;                    // jmp rel32 back into the function
    int32_t rel = (int32_t)(fn + kHookCopyBytes - (t + kHookCopyBytes + 5));
    memcpy(t + kHookCopyBytes + 1, &rel, 4);

    patch_jmp(fn, (void*)gmfix_hook_5D418C);
    FlushInstructionCache(GetCurrentProcess(), fn, kHookCopyBytes);
    gm_log("GMFix: hooked 0x%X (copy %d bytes) -> trampoline 0x%p",
           (uint32_t)(uintptr_t)fn, kHookCopyBytes, t);
    return true;
}

void gmfix_uninstall() {
    if (g_base && g_trampoline) {
        patch_bytes((uint8_t*)g_base + GM80_SUB_5D418C_RVA, g_orig_prologue, kHookCopyBytes);
        VirtualFree(g_trampoline, 0, MEM_RELEASE);
        g_trampoline = nullptr;
    }
}
