# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"mimetreeparser\")

add_subdirectory(core)

if (Qt${KF_MAJOR_VERSION}Quick_FOUND)
    add_subdirectory(quick)
endif()

if (Qt${KF_MAJOR_VERSION}Widgets_FOUND)
    add_subdirectory(widgets)
endif()

