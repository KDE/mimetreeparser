#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: CC0-1.0

$XGETTEXT `find -name \*.cpp -o -name \*.qml` -o $podir/mimetreeparser6.pot
