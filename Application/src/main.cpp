#include <driver.h>

#include <psapi.h> // Import after driver.h for dependencies
#include <thread>
#include <atomic>

const wchar_t *ProcName = L"GenshinImpact.exe";

std::atomic<bool> g_Running{true};
HANDLE g_DriverHandle = INVALID_HANDLE_VALUE;

DWORD getPID() {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(hSnapshot, &entry)) {
		do {
			if (wcscmp(entry.szExeFile, ProcName) == 0) {
				CloseHandle(hSnapshot);
				return entry.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &entry));
	}

	CloseHandle(hSnapshot);
	return 0;
}

void MonitorKernelLogs() {
	while (g_Running) {
		if (g_DriverHandle == INVALID_HANDLE_VALUE) {
			Sleep(100);
			continue;
		}

		LOG_MESSAGE logMsg{};
		DWORD bytes = driver::receiveLogMessage(g_DriverHandle, logMsg);

		if (bytes >= sizeof(LOG_MESSAGE)) {
			switch (logMsg.level) {
				case LOG_LEVEL_ERROR:
					std::cerr << "[-] " << logMsg.message << std::endl;
					break;
				case LOG_LEVEL_WARNING:
					std::cout << "[!] " << logMsg.message << std::endl;
					break;
				case LOG_LEVEL_TRACE:
					std::cout << "[*] " << logMsg.message << std::endl;
					break;
				case LOG_LEVEL_INFO:
					std::cout << "[+] " << logMsg.message << std::endl;
					break;
				default:
					std::cout << "[?] " << logMsg.message << std::endl;
					break;
			}
		}

		Sleep(50);
	}
}

bool connectToDriver() {
	// Create a file handle to the driver
	g_DriverHandle = CreateFile(L"\\\\.\\DriverClassInitHook", GENERIC_READ | GENERIC_WRITE, 0,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (g_DriverHandle == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		if (error != ERROR_FILE_NOT_FOUND) {
			std::cerr << "Failed to open driver: " << error << std::endl;
			std::cin.get();
		}
		return false;
	}

	return true;
}

void DriverConnectionManager() {
	bool lastConnected = true;

	while (g_Running) {
		bool nowConnected = (g_DriverHandle != INVALID_HANDLE_VALUE);
		if (!nowConnected) {
			if (connectToDriver()) {
				nowConnected = true;
			}
		}

		if (nowConnected != lastConnected) {
			std::cout << (nowConnected ? "Connected to driver!" : "Waiting for driver...")
					  << std::endl;
			lastConnected = nowConnected;
		}

		Sleep(500);
	}
}

int main() {
	// Set up Ctrl+C handler for graceful shutdown
	SetConsoleCtrlHandler(
		[](DWORD dwCtrlType) -> BOOL {
			if (dwCtrlType == CTRL_CLOSE_EVENT) {
				std::cout << "Shutting down..." << std::endl;
				g_Running = false;
				return TRUE;
			}
			if (dwCtrlType == CTRL_C_EVENT && g_DriverHandle != INVALID_HANDLE_VALUE) {
				std::cout << "Popping driver..." << std::endl;
				if (!driver::destroy(g_DriverHandle)) {
					DWORD error = GetLastError();
					std::cerr << "Failed to pop driver: " << error << std::endl;
				}
				if (g_DriverHandle != INVALID_HANDLE_VALUE) {
					CloseHandle(g_DriverHandle);
					g_DriverHandle = INVALID_HANDLE_VALUE;
				}
				return TRUE;
			}
			return FALSE;
		},
		TRUE);

	// Start the log monitoring thread
	std::thread logThread(MonitorKernelLogs);

	// Start the connection manager thread
	std::thread connectionThread(DriverConnectionManager);

	while (g_Running) {
		Sleep(1000);
	}

	// Cleanup
	if (logThread.joinable()) {
		logThread.join();
	}

	if (connectionThread.joinable()) {
		connectionThread.join();
	}

	if (g_DriverHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(g_DriverHandle);
		g_DriverHandle = INVALID_HANDLE_VALUE;
	}

	// Acquire the process's PID
	/*DWORD pid = getPID();
	if (pid == 0) {
		std::cout << "PID GenshinImpact.exe not found!" << std::endl;
		std::cin.get();
		return 0;
	}

	std::cout << "PID: " << pid << std::endl;*/

	// Attach to the process
	/*if (!driver::attachToProcess(driver, pid)) {
		std::cout << "Failed to attach to driver!" << std::endl;
		std::cin.get();
		return 0;
	}

	std::cout << "Attached to driver!" << std::endl;*/

	return 0;
}
