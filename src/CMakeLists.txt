SET(SOURCE_FILES
mytest.cpp
block.cpp
super_block.cpp
inode.cpp
common.cpp
bitmap.cpp)

ADD_LIBRARY(mytestlib SHARED ${SOURCE_FILES})
INSTALL(TARGETS mytestlib LIBRARY DESTINATION lib)