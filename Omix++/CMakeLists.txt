cmake_minimum_required(VERSION 3.5)
project(omix++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(SGX_USE_SGXSSL 1)
set(SGX_MODE "${SGX_MODE}")

set(SGXSSL_DIR "/home/kaichi/repo/graphos-javad/Omix++/sgxssl/Linux/package")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
find_package(SGX REQUIRED)

include_directories(${SGX_INCLUDE_DIR} Enclave Enclave/OMAP App App/OMAP Common)
set(EDL_SEARCH_PATHS Enclave Enclave/OMAP)

set(E_SRCS Enclave/Enclave.cpp)
set(T_SRCS Enclave/OMAP/AES.cpp Enclave/OMAP/AVLTree.cpp
        Enclave/OMAP/Bid.cpp Enclave/OMAP/LocalRAMStore.cpp
        Enclave/OMAP/ObliviousOperations.cpp Enclave/OMAP/OMAP.cpp
        Enclave/OMAP/ORAM.cpp Enclave/OMAP/ORAMEnclaveInterface.cpp)
set(LDS Enclave/Enclave.lds)

add_trusted_library(omap_lib
        USE_SGXSSL
        SRCS ${T_SRCS}
        EDL Enclave/OMAP/OMAP.edl
        EDL_SEARCH_PATHS Enclave/OMAP)

add_enclave_library(enclave
        USE_SGXSSL
        SRCS ${E_SRCS}
        TRUSTED_LIBS omap_lib
        EDL Enclave/Enclave.edl
        EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS}
        USE_PREFIX
        LDSCRIPT ${LDS})

enclave_sign(enclave
        KEY Enclave/Enclave_private.pem
        CONFIG Enclave/Enclave.config.xml)

set(SRCS App/App.cpp App/OMAP/AES.cpp App/OMAP/Bid.cpp
        App/OMAP/Node.cpp App/OMAP/RAMStore.cpp App/OMAP/Utilities.cpp)

add_untrusted_executable(app
        SRCS ${SRCS}
        USE_SGXSSL
        EDL Enclave/Enclave.edl
        EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})

add_dependencies(app enclave-sign)