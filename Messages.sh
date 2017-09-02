#!/bin/sh

# Extract strings from all source files.
# EXTRACT_TR_STRINGS extracts strings with lupdate and convert them to .pot with
# lconvert.
$EXTRACT_TR_STRINGS `find demo -name \*.cpp -o -name \*.h -o -name \*.ui -o -name \*.qml` -o $podir/Demo_qt.pot
