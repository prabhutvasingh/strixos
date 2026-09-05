#include "syscall.h"
#include "editor.h"

// Vim 9.2.1011 official splash - src/version.c intro
// Shows when vim launched without file, like `vim` on Unix
void vim_official_splash(void){
    const char *splash =
        "VIM - Vi IMproved                                      \n"
        "                                                       \n"
        "               version 9.2.1011                         \n"
        "           by Bram Moolenaar et al.                     \n"
        "     Vim is open source and freely distributable        \n"
        "                                                       \n"
        "            Help poor children in Uganda!               \n"
        "    type  :help Kuwasha<Enter>    for information       \n"
        "                                                       \n"
        "    type  :q<Enter>               to exit               \n"
        "    type  :help<Enter>  or  <F1>  for on-line help      \n"
        "    type  :help version9<Enter>   for version info      \n"
        "\n";
    sys_write(1, splash,  512); // write, splash len ~512 but sys_write handles
}
void vim_official_version(void){
    sys_write(1,"VIM - Vi IMproved 9.2.1011 (2024)\nby Bram Moolenaar et al. https://github.com/vim/vim\n",80);
}
