#!/bin/sh
# Copyright (C) Gabriel Taillon, 2025

splint mace.h -boolops -nullstate -compdef -type -unqualifiedtrans -mustfreeonly -compdestroy -fullinitblock -predboolint -macroredef -unsignedcompare -nullret -exportlocal -mustfreefresh -temptrans -shiftnegative -usereleased

# cppcheck --enable=warning --check-level=exhaustive --std=c99 tnecs.c tnecs.h
