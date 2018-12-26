/*
    webalizer - a web server log analysis program

    Copyright (c) 2004-2019, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information

    console.h
*/
#ifndef CONSOLE_H
#define CONSOLE_H

void set_ctrl_c_handler(void (*ctrl_c_handler)(void));
void reset_ctrl_c_handler(void);

#endif // CONSOLE_H
