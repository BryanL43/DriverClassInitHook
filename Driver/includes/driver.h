#ifndef DRIVER_H
#define DRIVER_H

#include <ntifs.h>
#include <stdarg.h>
#include <ntstrsafe.h>

#define BUFFER_SIZE 512

extern "C" {
NTKERNELAPI NTSTATUS IoCreateDriver(
	PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
}

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

		constexpr ULONG send_log =
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

	// Global accessors
	UNICODE_STRING &getDeviceName();
	UNICODE_STRING &getSymbolicLink();
	PDEVICE_OBJECT &getDeviceObject();
	BOOLEAN isLoggingEnabled();

	// Driver functions
	VOID init();
	VOID shutdown();
	VOID sendLogMessage(ULONG level, PCSTR format, ...);
	NTSTATUS create(PDEVICE_OBJECT DeviceObject, PIRP irp);
	NTSTATUS close(PDEVICE_OBJECT DeviceObject, PIRP irp);
	NTSTATUS device_control(PDEVICE_OBJECT DeviceObject, PIRP irp);

} // namespace driver

// Logging macros
#define LOG_ERROR(fmt, ...) driver::sendLogMessage(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) driver::sendLogMessage(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) driver::sendLogMessage(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) driver::sendLogMessage(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)

#endif
