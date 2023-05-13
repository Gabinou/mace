#!/bin/sh

# style.sh
# Copyright (C) Gabriel Taillon, 2023
# Automatic formatting with astyle for mace 

astyle --options=style.txt --verbose test.c mace.c mace.h installer.c
