#include "imports.h"

pfnPsGetProcessPeb PsGetProcessPeb = nullptr;

NTSTATUS InitializeFunctionPointers() {
	UNICODE_STRING funcName;
	RtlInitUnicodeString(&funcName, L"PsGetProcessPeb");
	PsGetProcessPeb = reinterpret_cast<pfnPsGetProcessPeb>(MmGetSystemRoutineAddress(&funcName));

	return PsGetProcessPeb ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}
