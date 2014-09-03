cmake_minimum_required(VERSION 2.8)
project( DetectImage )
find_package( OpenCV REQUIRED )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
# find tesseract
find_path(TESSERACT_INCLUDE_DIR baseapi.h
    /opt/local/include
    /usr/local/include
    /usr/include/tesseract
    )
find_library(TESSERACT_LIBRARY 
    NAMES tesseract
    PATHS 
    /opt/local/lib/
    /usr/local/lib/
    /usr/lib
    )
message(STATUS "looked for tesseract library.")
message(STATUS "Include file detected: [${TESSERACT_INCLUDE_DIR}].")
message(STATUS "Lib file detected: [${TESSERACT_LIBRARY}].")
add_library(tesseract STATIC IMPORTED)
set_property(TARGET tesseract PROPERTY IMPORTED_LOCATION 
    ${TESSERACT_LIBRARY}
    )


include_directories(/usr/include/tesseract /usr/include/leptonica/)
link_directories(/usr/lib)

add_executable( ocrJohn john/test.cpp )
add_executable( ocrSaintMichel1 saintmichel/ocrSaintMichel1.cpp)
target_link_libraries( ocrJohn ${OpenCV_LIBS} tesseract lept)
target_link_libraries( ocrSaintMichel1 ${OpenCV_LIBS} tesseract lept)
