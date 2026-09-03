#ifndef EDITOR_H
#define EDITOR_H
// vim-like editor + nano fallback
// Preferred vim modal editing, nano keys also work
void editor_open(const char* path);
void editor_open_nano(const char* path);
#endif
