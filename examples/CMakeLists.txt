# SPDX-FileCopyrightText: 2023 Carl Schwan <carl.schwan@gnupg.com>
# SPDX-License-Identifier: BSD-3-Clause

if (Qt${KF_MAJOR_VERSION}Quick_FOUND)
    add_subdirectory(qml)
endif()

if (Qt${KF_MAJOR_VERSION}Widgets_FOUND)
    add_subdirectory(widgets)
endif()
