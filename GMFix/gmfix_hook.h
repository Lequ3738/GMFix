#pragma once

// Install / uninstall the exe-write hook. Call once from DllMain.
// `base` = GetModuleHandle(NULL) of GameMaker 8.0.exe.
bool gmfix_install(void* base);
void gmfix_uninstall();
