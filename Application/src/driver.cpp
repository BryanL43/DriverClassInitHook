#include "driver.h"

namespace driver {
	bool attachToProcess(HANDLE driverHandle, const DWORD pid) {
		Request r;
		r.pid = reinterpret_cast<HANDLE>(pid);

		return DeviceIoControl(
			driverHandle, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr);
	}

	DWORD receiveLogMessage(HANDLE driverHandle, LOG_MESSAGE &out) {
		DWORD bytes = 0;
		BOOL ok = DeviceIoControl(
			driverHandle, codes::recieve_log, nullptr, 0, &out, sizeof(out), &bytes, nullptr);
		return ok ? bytes : 0;
	}

	bool destroy(HANDLE driverHandle) {
		DWORD bytes = 0; // Unused
		return DeviceIoControl(
			driverHandle, codes::self_destruct, nullptr, 0, nullptr, 0, &bytes, nullptr);
	}

} // namespace driver
