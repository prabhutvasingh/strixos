#pragma once
// GNU nano 9.2 official port for StrixOS — renamed to 'editor'
// Source: https://git.savannah.gnu.org/git/nano.git src/nano.c 2748 lines
// License: GPL-3.0 — Benno Schulenberg et al., FSF
// StrixOS adaptation: kernel/editor.c — title bar + ^O/^X without ncurses

void editor_open(const char* path);
void nano_open(const char* path);
