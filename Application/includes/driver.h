#ifndef DRIVER_H
#define DRIVER_H

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

#define BUFFER_SIZE 512

// Logging definitions
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_TRACE 2
#define LOG_LEVEL_INFO 3

// Log message structure for IOCTL
typedef struct _LOG_MESSAGE {
	ULONG level;
	CHAR message[BUFFER_SIZE];
} LOG_MESSAGE, *PLOG_MESSAGE;

namespace driver {
	namespace codes { // Holds the ioctl codes
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		constexpr ULONG recieve_log = // equivalent to send_log in kernel mode
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_READ_ACCESS);

		constexpr ULONG self_destruct =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_ANY_ACCESS);
	} // namespace codes

	// Share between usermode and kernel mode
	struct Request {
		HANDLE pid;
		PVOID target;
		PVOID buffer;

		SIZE_T size;
		SIZE_T returnSize;
	};

	bool attachToProcess(HANDLE driverHandle, const DWORD pid);
	DWORD receiveLogMessage(HANDLE driverHandle, LOG_MESSAGE &out);
    bool destroy(HANDLE driverHandle);

} // namespace driver

#endif
