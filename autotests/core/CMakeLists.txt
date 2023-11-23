# SPDX-FileCopyrightText: 2017 Christian Mollekopf <mollekopf@kolabsys.com>
# SPDX-FileCopyrightText: 2022 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-3-Clause

set(AUTOMOC ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata" )

include(ECMAddTests)
include(${CMAKE_SOURCE_DIR}/cmake/modules/add_gpg_crypto_test.cmake)

function(add_mimetreeparser_class_unittest _name _additionalSource)
    add_executable(${_name} ${_name}.cpp setupenv.cpp ${_additionalSource})
    target_link_libraries(${_name} PRIVATE
        Qt::Test
        KPim6::MimeTreeParserCore
    )
    target_include_directories(${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
    add_test(NAME core-${_name} COMMAND $<TARGET_FILE:${_name}>)
endfunction()

function(add_mimetreeparser_crypto_unittest _name)
    add_executable(${_name} ${_name}.cpp ../setupenv.cpp)
    target_link_libraries(${_name} PRIVATE Gpgmepp)
    target_include_directories(${_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${GPGME_INCLUDE_DIRS}
    )
    target_link_libraries(${_name} PRIVATE
        Qt::Test
        KPim6::MimeTreeParserCore
    )
    add_gpg_crypto_test(${_name} core-${_name})
endfunction()

add_mimetreeparser_crypto_unittest(attachmenttest)

add_definitions( -DMAIL_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata" )
include(${CMAKE_SOURCE_DIR}/cmake/modules/add_gpg_crypto_test.cmake)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
include_directories(${GPGME_INCLUDE_DIRS})

include(ECMAddTests)

add_executable(cryptohelpertest cryptohelpertest.cpp)
add_gpg_crypto_test(cryptohelpertest core-cryptohelpertest)
target_link_libraries(cryptohelpertest PUBLIC
    Qt::Test
    KPim6::MimeTreeParserCore
)

add_executable(mimetreeparsertest mimetreeparsertest.cpp)
add_gpg_crypto_test(mimetreeparsertest core-mimetreeparsertest)
target_link_libraries(mimetreeparsertest PUBLIC
    Qt::Test
    KPim6::MimeTreeParserCore
    Gpgmepp
)

add_executable(gpgerrortest gpgerrortest.cpp)
target_link_libraries(gpgerrortest PUBLIC
    Qt::Test
    KPim6::MimeTreeParserCore
    Gpgmepp
)
add_test(NAME core-gpgerrortest COMMAND $<TARGET_FILE:gpgerrortest>)

add_executable(fileopenertest fileopenertest.cpp)
target_link_libraries(fileopenertest PUBLIC
    Qt::Test
    KPim6::MimeTreeParserCore
    Gpgmepp
)
add_test(NAME core-fileopenertest COMMAND $<TARGET_FILE:fileopenertest>)

## PartModel test
add_executable(partmodeltest partmodeltest.cpp)
add_gpg_crypto_test(partmodeltest core-partmodeltest)
target_link_libraries(partmodeltest
    Qt::Test
    KPim6::MimeTreeParserCore
    Gpgmepp
)

ecm_add_test(attachmentmodeltest.cpp
    LINK_LIBRARIES KPim6MimeTreeParserCore Qt::Test Gpgmepp
    NAME_PREFIX "core-"
)
