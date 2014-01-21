find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS} -include qdbus_gen_includes.hpp")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(${Qt5Core_DEFINITIONS})
add_definitions(${Qt5DBus_DEFINITIONS})

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5DBus_INCLUDE_DIRS})

find_path(TELEPATHY_QT5_INCLUDE_DIRS TelepathyQt PATHS /usr/include/telepathy-qt5)
find_library(TELEPATHY_QT5_LIBRARIES telepathy-qt5)
find_library(TELEPATHY_QT5_SERVICE_LIBRARIES telepathy-qt5-service)

include_directories(${TELEPATHY_QT5_INCLUDE_DIRS})

set(pipe_xml ${PipesTp_SOURCE_DIR}/interfaces/org.freedesktop.Telepathy.Pipe.xml)
message(${pipe_xml})

set(PipesTp_SRCS 
    contact_list.cpp
    simple_presence.cpp
    connection.cpp
    connection_manager.cpp
    approver.cpp
    protocol.cpp
)

qt5_add_dbus_interface(PipesTp_SRCS ${pipe_xml} pipe_interface)

add_library(PipesTp STATIC ${PipesTp_SRCS})

qt5_use_modules(PipesTp Core DBus Xml Network)
target_link_libraries(PipesTp ${TELEPATHY_QT5_LIBRARIES})
target_link_libraries(PipesTp ${TELEPATHY_QT5_SERVICE_LIBRARIES})

add_executable(telepathy-pipes main.cpp)
target_link_libraries(telepathy-pipes PipesTp)

install(TARGETS telepathy-pipes DESTINATION lib)
