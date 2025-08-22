#include <driver.h>
// #include <imports.h>

static inline VOID SleepMs(LONG ms) {
	LARGE_INTEGER interval;
	interval.QuadPart = -(LONGLONG)ms * 10'000;
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

NTSTATUS DriverMain(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	driver::init();

	// Create device
	NTSTATUS status = IoCreateDevice(DriverObject, 0, &driver::getDeviceName(), FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN, FALSE, &driver::getDeviceObject());
	if (!NT_SUCCESS(status)) {
		return status;
	}

	// Create symbolic link
	status = IoCreateSymbolicLink(&driver::getSymbolicLink(), &driver::getDeviceName());
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(driver::getDeviceObject());
		driver::getDeviceObject() = nullptr;
		return status;
	}

	// Allow us to send small amounts of data between user/kernel mode
	driver::getDeviceObject()->Flags |= DO_BUFFERED_IO;

	// Set up dispatch routines
	DriverObject->MajorFunction[IRP_MJ_CREATE] = driver::create;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

	driver::getDeviceObject()->Flags &= ~DO_DEVICE_INITIALIZING;

	SleepMs(3000);

	LOG_INFO("Dispatch message from kernel mode!");

	/*SleepMs(5000);

	driver::unload(DriverObject);

	LOG_INFO("Driver unmounted and disconnected from application!");*/

	return STATUS_SUCCESS;
}

// Main entry point for the driver
NTSTATUS DriverEntry() {
    // NULL name for a dynamic driver
	return IoCreateDriver(NULL, &DriverMain);
}
