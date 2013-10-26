SET(CORELIBS ${Boost_SYSTEM_LIBRARY})

IF (${CMAKE_SYSTEM} MATCHES ".*gentoo.*")
	SET (GENTOO ON)
	SET (LINUX)
	MESSAGE("=== Detected system is Gentoo ===")
ELSE()
	MESSAGE("=== Detected system is ${CMAKE_SYSTEM} ===") 
ENDIF()

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckSymbolExists)

#TODO: check for hpp files
#somehow this does not work

MESSAGE("=== Libraries ===") 
CHECK_INCLUDE_FILES("zmq.h" HAS_ZMQ)

IF (NOT HAS_ZMQ)
	MESSAGE(SEND_ERROR "ZeroMQ CPP headers not found!")
ENDIF()

CHECK_INCLUDE_FILES("msgpack.h" HAS_MSGPACK)
IF (NOT HAS_MSGPACK)
	MESSAGE(SEND_ERROR "MessagePack CPP headers not found!")
ENDIF()

IF (GENTOO)
	IF (NOT HAS_MSGPACK)
		MESSAGE("    install dev-libs/msgpack")
	ENDIF()
	IF (NOT HAS_ZMQ)
		MESSAGE("    install net-libs/cppzmq")
	ENDIF()
ENDIF()

MESSAGE("=== Features ===") 
#MESSAGE("  * ENABLE_TROSSEN_COMMANDER   ${ENABLE_TROSSEN_COMMANDER}")

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

ADD_EXECUTABLE(hid_trossen hid_trossen.cpp)
TARGET_LINK_LIBRARIES(hid_trossen ${Boost_LIBRARIES} zmq msgpack)

INSTALL (TARGETS hid_trossen
	RUNTIME DESTINATION bin
)

