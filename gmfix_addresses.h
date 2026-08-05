// GMFix — GameMaker 8.0 addresses (IDA-verified).
// GM 8.0 is a Delphi 7 IDE, image base 0x400000. Addresses below are IDA
// start-of-function addresses; the plugin computes them as base + RVA.
#pragma once

// sub_5D418C — the standalone exe-write core. Loads the embedded "rundata"
// runner + "dxdata" resources, appends the compiled gamedata, updates the
// MAINICON resource, and writes the finished .exe to its first argument (EAX).
// Called ONLY from GM80_OpenProject (0x5D453C). Entry: EAX = .exe output path
// (Delphi AnsiString data ptr), ECX = arg2. On return the output file is
// complete and closed.
//
// GMFix hooks THIS function, not 0x5D453C: GMSave already owns the 0x5D453C
// entry hook (patch_jmp), so a second hook there would clobber it. Both DLLs
// coexist: GMSave intercepts .gm80 loads before the trampoline (sub_5D418C
// never runs), and the compile path reaches sub_5D418C with EAX = the .exe.
#define GM80_SUB_5D418C_RVA 0x1D418C
