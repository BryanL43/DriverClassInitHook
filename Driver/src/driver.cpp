#include <driver.h>

namespace {
	// Private global states for the driver (Singleton)
	PDEVICE_OBJECT g_DeviceObject = nullptr;
	UNICODE_STRING g_DeviceName, g_SymbolicLink;
	BOOLEAN g_LoggingEnabled = FALSE;

	// Log node structure
	typedef struct _LOG_NODE {
		LIST_ENTRY Entry;
		LOG_MESSAGE Msg;
	} LOG_NODE, *PLOG_NODE;

	// Linked-list log message queue
	KSPIN_LOCK g_LogLock;
	LIST_ENTRY g_LogQueue;

	_IRQL_requires_max_(DISPATCH_LEVEL) void EnqueueLog(const LOG_MESSAGE *src) {
		auto node = (PLOG_NODE)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(LOG_NODE), 'golM');
		if (!node) {
			return;
		}
		node->Msg = *src;

		KIRQL irql;
		KeAcquireSpinLock(&g_LogLock, &irql);
		InsertTailList(&g_LogQueue, &node->Entry);
		KeReleaseSpinLock(&g_LogLock, irql);
	}

	_IRQL_requires_max_(DISPATCH_LEVEL) bool DequeueLog(LOG_MESSAGE *dst) {
		KIRQL irql;
		KeAcquireSpinLock(&g_LogLock, &irql);
		bool empty = IsListEmpty(&g_LogQueue) != FALSE;
		PLIST_ENTRY e = empty ? nullptr : RemoveHeadList(&g_LogQueue);
		KeReleaseSpinLock(&g_LogLock, irql);

		if (empty) {
			return false;
		}

		auto node = CONTAINING_RECORD(e, LOG_NODE, Entry);
		*dst = node->Msg;
		ExFreePoolWithTag(node, 'golM');
		return true;
	}

} // namespace

namespace driver {
	// Singleton so that only one instance of the driver is created
	constexpr wchar_t kDeviceName[] = L"\\Device\\DriverClassInitHook";
	constexpr wchar_t kSymbolicLink[] = L"\\DosDevices\\DriverClassInitHook";

	// Global accessors
	UNICODE_STRING &getDeviceName() {
		return g_DeviceName;
	}

	UNICODE_STRING &getSymbolicLink() {
		return g_SymbolicLink;
	}

	PDEVICE_OBJECT &getDeviceObject() {
		return g_DeviceObject;
	}

	BOOLEAN isLoggingEnabled() {
		return g_LoggingEnabled;
	}

	// Driver functions
	VOID init() {
		KeInitializeSpinLock(&g_LogLock);
		InitializeListHead(&g_LogQueue);
		RtlInitUnicodeString(&g_DeviceName, kDeviceName);
		RtlInitUnicodeString(&g_SymbolicLink, kSymbolicLink);
		g_LoggingEnabled = TRUE;
	}

	VOID shutdown() {
		g_LoggingEnabled = FALSE;
		// Drain the log queue
		for (;;) {
			LOG_MESSAGE tmp;
			if (!DequeueLog(&tmp)) {
				break;
			}
		}

		// Cleanup driver
		if (g_DeviceObject) {
			IoDeleteSymbolicLink(&g_SymbolicLink);
			IoDeleteDevice(g_DeviceObject);
			g_DeviceObject = nullptr;
		}
	}

	VOID sendLogMessage(ULONG level, PCSTR fmt, ...) {
		if (!g_LoggingEnabled) {
			return;
		}

		CHAR line[BUFFER_SIZE];
		va_list args;
		va_start(args, fmt);
		RtlStringCbVPrintfA(line, sizeof(line), fmt, args);
		va_end(args);

        DbgPrintEx(DPFLTR_IHVDRIVER_ID, level, "%s\n", line);

		LOG_MESSAGE m{};
		m.level = level;
		RtlStringCbCopyA(m.message, sizeof(m.message), line);
		EnqueueLog(&m);
	}

	NTSTATUS create(PDEVICE_OBJECT DeviceObject, PIRP irp) {
		UNREFERENCED_PARAMETER(DeviceObject);
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	NTSTATUS close(PDEVICE_OBJECT DeviceObject, PIRP irp) {
		UNREFERENCED_PARAMETER(DeviceObject);
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	NTSTATUS device_control(PDEVICE_OBJECT DeviceObject, PIRP irp) {
		UNREFERENCED_PARAMETER(DeviceObject);

		// Determines which code is passed through (holds attach, read, or write)
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
		NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
		ULONG_PTR bytes = 0; // For IoStatus.Information
		static PEPROCESS targetProcess = nullptr;

		// Validate stack
		if (!stack) {
			status = STATUS_INVALID_PARAMETER;
			irp->IoStatus.Status = status;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		switch (stack->Parameters.DeviceIoControl.IoControlCode) {
			case codes::attach: {
				if (!irp->AssociatedIrp.SystemBuffer
					|| stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(Request)) {
					status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

				// Cast system buffer to Request and attach to process
				auto *request = reinterpret_cast<Request *>(irp->AssociatedIrp.SystemBuffer);
				status = PsLookupProcessByProcessId(request->pid, &targetProcess);
				break;
			}

			case codes::send_log: {
				if (!irp->AssociatedIrp.SystemBuffer
					|| stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(LOG_MESSAGE)) {
					status = STATUS_BUFFER_TOO_SMALL;
					break;
				}

				// Cast system buffer to LOG_MESSAGE and drain log queue
				auto *out = reinterpret_cast<LOG_MESSAGE *>(irp->AssociatedIrp.SystemBuffer);
				if (!DequeueLog(out)) {
					status = STATUS_NO_MORE_ENTRIES;
					break;
				}
				bytes = sizeof(LOG_MESSAGE);
				status = STATUS_SUCCESS;
				break;
			}

            case codes::self_destruct: {
				shutdown();
				status = STATUS_SUCCESS;
				break;
            }

			default: {
				status = STATUS_INVALID_DEVICE_REQUEST;
				break;
			}
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = bytes;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}

} // namespace driver
