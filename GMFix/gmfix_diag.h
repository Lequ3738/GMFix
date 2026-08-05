// User-facing diagnostics collected while patching a built exe, presented once
// in a single modal MessageBox afterwards. Tolerant by design: a skipped patch
// never fails the build — these are warnings, not errors. Identical messages
// are de-duplicated so one broken runner yields one line.
#pragma once

void gmfix_diag_reset();
void gmfix_diag_add(const char* fmt, ...);
bool gmfix_diag_any();
void gmfix_diag_show(const char* title);
