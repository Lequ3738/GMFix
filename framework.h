#pragma once
// GMFix — Applies gm8x_fix runtime fixes to GameMaker 8.0-built executables.
// Hooks the IDE's exe-write core: after GM 8.0 finishes a standalone .exe,
// the runner fixes (input lag, joystick, scheduler, 2GB, DirectPlay,
// keyboard release delay) are applied to the output file.
#define GMFIX_VERSION 100
