# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/norman/esp-idf/components/bootloader/subproject"
  "/home/norman/esp_projects/smart_lock/build/bootloader"
  "/home/norman/esp_projects/smart_lock/build/bootloader-prefix"
  "/home/norman/esp_projects/smart_lock/build/bootloader-prefix/tmp"
  "/home/norman/esp_projects/smart_lock/build/bootloader-prefix/src/bootloader-stamp"
  "/home/norman/esp_projects/smart_lock/build/bootloader-prefix/src"
  "/home/norman/esp_projects/smart_lock/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/norman/esp_projects/smart_lock/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/norman/esp_projects/smart_lock/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
