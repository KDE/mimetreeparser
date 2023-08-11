# SPDX-FileCopyrightText: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
# SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-3-Clause

set(AUTOMOC ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata" )
add_definitions(-DMAILTEMPLATE_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../templatetestdata" )


include(ECMAddTests)
include(${CMAKE_SOURCE_DIR}/cmake/modules/add_gpg_crypto_test.cmake)

function(add_mimetreeparser_class_unittest _name _additionalSource)
    add_executable(${_name} ${_name}.cpp setupenv.cpp ${_additionalSource})
    target_link_libraries(${_name} PRIVATE
        Qt::Test
        mimetreeparserprivate
        KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
    )
    target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    add_test(NAME ${_name} COMMAND $<TARGET_FILE:${_name}>)
endfunction()

function(add_mimetreeparser_crypto_unittest _name)
    add_executable(${_name} ${_name}.cpp setupenv.cpp)
    if (QT_MAJOR_VERSION STREQUAL "6")
        target_link_libraries(${_name} PRIVATE QGpgmeQt6 Qt6::Core5Compat)
    else()
        target_link_libraries(${_name} PRIVATE Gpgme::Gpgme)
    endif()
    target_include_directories(${_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${GPGME_INCLUDE_DIRS}
    )
    target_link_libraries(${_name} PRIVATE
        Qt::Test
        mimetreeparserprivate
        KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
    )
    add_gpg_crypto_test(${_name} mimetreeparser-${_name})
endfunction()

add_subdirectory(gnupg_home)
add_mimetreeparser_crypto_unittest(attachmenttest)
add_mimetreeparser_class_unittest(cryptohelpertest "../cryptohelper.cpp")

add_definitions( -DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata" )
include(${CMAKE_SOURCE_DIR}/cmake/modules/add_gpg_crypto_test.cmake)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
include_directories(${GPGME_INCLUDE_DIRS})

include(ECMAddTests)

add_executable(mimetreeparsertest mimetreeparsertest.cpp)
add_gpg_crypto_test(mimetreeparsertest mimetreeparsertest)
target_link_libraries(mimetreeparsertest PUBLIC
    Qt::Test
    mimetreeparserprivate
    KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
)

add_executable(gpgerrortest gpgerrortest.cpp)
target_link_libraries(gpgerrortest PUBLIC
    Qt::Test
    mimetreeparserprivate
    KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
)
add_test(NAME gpgerrortest COMMAND $<TARGET_FILE:gpgerrortest>)

## PartModel test
add_executable(partmodeltest partmodeltest.cpp)
add_gpg_crypto_test(partmodeltest partmodeltest)
target_link_libraries(partmodeltest
    Qt::Test
    mimetreeparserprivate
    KPim${KF_MAJOR_VERSION}::MimeTreeParserCore
)
