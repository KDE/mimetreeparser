# SPDX-FileCopyrightText: 2013 Sandro Knauß <mail@sandroknauss.de>
# SPDX-License-Identifier: BSD-3-Clause

find_path(GPGME_INCLUDE_DIR NAMES gpgme.h)
find_path(GPGERROR_INCLUDE_DIR NAMES gpg-error.h)
find_library(GPGME_LIBRARY NAMES gpgme)
find_library(GPGERROR_LIBRARY NAMES gpg-error)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gpgme DEFAULT_MSG GPGME_INCLUDE_DIR GPGERROR_INCLUDE_DIR GPGME_LIBRARY GPGERROR_LIBRARY)

mark_as_advanced(GPGME_INCLUDE_DIR GPGME_LIBRARY GPGME_INCLUDE_DIR GPGME_LIBRARY)

set(GPGME_LIBRARIES ${GPGME_LIBRARY} ${GPGERROR_LIBRARY})
set(GPGME_INCLUDE_DIRS ${GPGME_INCLUDE_DIR} ${GPGERROR_INCLUDE_DIR})

if (GPGME_FOUND AND NOT TARGET Gpgme::Gpgme)
    add_library(Gpgme::Gpgme INTERFACE IMPORTED)
    set_target_properties(Gpgme::Gpgme PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES  "${GPGME_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${GPGME_LIBRARIES}"
        )
endif()
