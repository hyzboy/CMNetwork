SET(NETWORK_BASE_SOURCE
    IPAddress.cpp
    Socket.cpp
    )

SET(NETWORK_UDP_SOURCE
    UdpSocket.cpp)

IF(BUILD_NETWORK_UDP_LITE)
    SET(NETWORK_UDP_SOURCE ${NETWORK_UDP_SOURCE} UdpLiteSocket.cpp)
ENDIF()

SET(NETWORK_TCP_COMMON_SOURCE
    TCPSocket.cpp
    SocketInputStream.cpp
    SocketOutputStream.cpp
)

SET(NETWORK_TCP_CLIENT_SOURCE
    TCPClient.cpp)

SET(NETWORK_TCP_SERVER_SOURCE
    ServerSocket.cpp
    AcceptServer.cpp
    MultiThreadAccept.cpp
    TCPServer.cpp
    TCPAccept.cpp
    TCPAcceptPacket.cpp
    SocketManage.cpp
)

SET(NETWORK_SCTP_SOURCE
    SCTPSocket.cpp
    SCTPO2OClient.cpp
    SCTPO2OServer.cpp
    SCTPO2MServer.cpp
)

SET(NETWORK_HTTP_SOURCE
    HTTPInputStream.cpp
)

SET(NETWORK_WEBSOCKET_SOURCE
    WebSocket.cpp
    WebSocketAccept.cpp)

SOURCE_GROUP("Base"                     FILES ${NETWORK_BASE_SOURCE})
SOURCE_GROUP("Transport\\UDP"           FILES ${NETWORK_UDP_SOURCE})
SOURCE_GROUP("Transport\\TCP"           FILES ${NETWORK_TCP_COMMON_SOURCE})
SOURCE_GROUP("Transport\\TCP\\Client"   FILES ${NETWORK_TCP_CLIENT_SOURCE})
SOURCE_GROUP("Transport\\TCP\\Server"   FILES ${NETWORK_TCP_SERVER_SOURCE})
SOURCE_GROUP("Application\\HTTP"        FILES ${NETWORK_HTTP_SOURCE})
SOURCE_GROUP("Application\\WebSocket"	FILES ${NETWORK_WEBSOCKET_SOURCE})

IF(BUILD_NETWORK_SCTP)
    SOURCE_GROUP("Transport\\SCTP"             FILES ${NETWORK_SCTP_SOURCE})

    SET(NETWORK_BASE_SOURCE ${NETWORK_BASE_SOURCE} ${NETWORK_SCTP_SOURCE})
ENDIF(BUILD_NETWORK_SCTP)

IF(WIN32)
    SET(NETWORK_OS_SOURCE
        SocketManageSelect.cpp
        WinFireWall.cpp)
ENDIF(WIN32)

IF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(NETWORK_OS_SOURCE SocketManageEpoll.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    SET(NETWORK_OS_SOURCE SocketManageKqueue.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    SET(NETWORK_OS_SOURCE SocketManageKqueue.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    SET(NETWORK_OS_SOURCE SocketManageKqueue.cpp)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "NetBSD")
    SET(NETWORK_OS_SOURCE SocketManageKqueue.cpp)
endif()

SET(NETWORK_OS_SOURCE_GROUP ${CMAKE_SYSTEM_NAME})
SOURCE_GROUP(${NETWORK_OS_SOURCE_GROUP} FILES ${NETWORK_OS_SOURCE})

SET(CM_NETWORK_ALL_SOURCE
    ${NETWORK_BASE_SOURCE}
    ${NETWORK_UDP_SOURCE}
    ${NETWORK_TCP_COMMON_SOURCE}
    ${NETWORK_TCP_CLIENT_SOURCE}
    ${NETWORK_TCP_SERVER_SOURCE}
    ${NETWORK_WEBSOCKET_SOURCE}
    ${NETWORK_OS_SOURCE}
    WebapiUserAgent.cpp)

add_cm_library(CMNetwork "CM" ${CMNETWORK_ROOT_INCLUDE_PATH} ${CM_NETWORK_ALL_SOURCE} ${NETWORK_HTTP_SOURCE})

#find_package(unofficial-gumbo CONFIG REQUIRED)
#target_link_libraries(CMNetwork PRIVATE unofficial::gumbo::gumbo)
