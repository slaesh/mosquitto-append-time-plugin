include_directories(${mosquitto_SOURCE_DIR} ${mosquitto_SOURCE_DIR}/include
			${STDBOOL_H_PATH} ${STDINT_H_PATH})

add_library(append_timestamp SHARED append_timestamp.c)
set_target_properties(append_timestamp PROPERTIES
	POSITION_INDEPENDENT_CODE 1
)
set_target_properties(append_timestamp PROPERTIES PREFIX "")

# Don't install, these are example plugins only.
#install(TARGETS append_timestamp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
