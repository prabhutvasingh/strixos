# Vim Official 9.2.1011

Vendored from https://github.com/vim/vim

This directory will contain official Vim source for StrixOS port.

To fetch:

```bash
git clone --depth 1 https://github.com/vim/vim.git /tmp/vim_official
cp -r /tmp/vim_official/src kernel/vim/src
cp /tmp/vim_official/LICENSE kernel/vim/LICENSE
```

StrixOS port uses `src/main.c`, `src/normal.c`, `src/edit.c` via `kernel/vim_splash.c` → `VIM - Vi IMproved` splash.
Build tiny: `cd kernel/vim/src && ./configure --with-features=tiny && make`
StrixOS `vim` without args now shows official splash (like `vim` 9.2) then falls through to StrixVim for editing.
