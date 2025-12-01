#!/bin/sh

# style.sh
# Copyright (C) Gabriel Taillon, 2023
# Automatic formatting with astyle for mace 

astyle --options=style.txt --verbose test/test.c mace.h installer_macefile.c example_macefile.c
