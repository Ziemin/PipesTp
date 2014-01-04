configure_file(PipesCM-telepathy.service.in org.freedesktop.Telepathy.ConnectionManager.pipes.service)

install (FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.ConnectionManager.pipes.service DESTINATION share/dbus-1/services)
