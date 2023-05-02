# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/larsleimbach/esp/esp-idf/components/bootloader/subproject"
  "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader"
  "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix"
  "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix/tmp"
  "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix/src"
  "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/larsleimbach/Documents/Pra-IoT/initial_blink/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
