# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"mimetreeparser6\")
add_subdirectory(core)

if (Qt6Quick_FOUND)
    add_subdirectory(quick)
endif()

if (Qt6Widgets_FOUND)
    add_subdirectory(widgets)
endif()

