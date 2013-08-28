#pragma region "Includes"
#include <stdio.h>
#include <windows.h>
#include "ServiceInstaller.h"
#pragma endregion

void InstallService(PWSTR pszServiceName, 
                    PWSTR pszDisplayName,
					PWSTR pszDisplayDescription,
                    DWORD dwStartType,
                    PWSTR pszDependencies, 
                    PWSTR pszAccount, 
                    PWSTR pszPassword)
{
    wchar_t szPath[MAX_PATH];
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;

    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
    {
        wprintf(L"GetModuleFileName failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Open the local default service control manager database
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);

    if (hSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Install the service into SCM by calling CreateService
    hService = CreateService(
							hSCManager,						// SCManager database
							pszServiceName,                 // Name of service
							pszDisplayName,                 // Name to display
							SERVICE_QUERY_STATUS,           // Desired access
							SERVICE_WIN32_OWN_PROCESS,      // Service type
							dwStartType,                    // Service start type
							SERVICE_ERROR_NORMAL,           // Error control type
							szPath,                         // Service's binary
							NULL,                           // No load ordering group
							NULL,                           // No tag identifier
							pszDependencies,                // Dependencies
							pszAccount,                     // Service running account
							pszPassword                     // Password of the account
							);

    if (hService != NULL)
	{
		SERVICE_DESCRIPTION sd;
		sd.lpDescription = pszDisplayDescription;
		ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, (void*)&sd);

		wprintf(L"%s successfully installed.\n", pszServiceName);
	}
	else
    {
		// Try to analyze the fault and update existing service registration if already exists
		DWORD error = GetLastError();

		if (error == ERROR_SERVICE_EXISTS)
		{
			UpdateServiceInformation(pszServiceName, 
									pszDisplayName,
									pszDisplayDescription,
									dwStartType,
									pszDependencies, 
									pszAccount, 
									pszPassword);
		}
		else
		{
			wprintf(L"CreateService failed w/err 0x%08lx\n", GetLastError());
			goto Cleanup;
		}
    }

Cleanup:
    // Centralized cleanup for all allocated resources.
	if (hService)
    {
        CloseServiceHandle(hService);
        hService = NULL;
    }

    if (hSCManager)
    {
        CloseServiceHandle(hSCManager);
        hSCManager = NULL;
    }
}

void UninstallService(PWSTR pszServiceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ssSvcStatus = {};

    // Open the local default service control manager database
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
    {
        wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Open the service with delete, stop, and query status permissions
    hService = OpenService(hSCManager,
							pszServiceName,
							SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);

    if (hService == NULL)
    {
        wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    // Try to stop the service
    if (ControlService(hService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        wprintf(L"Stopping %s.", pszServiceName);
        Sleep(1000);

        while (QueryServiceStatus(hService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                wprintf(L".");
                Sleep(1000);
            }
            else break;
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            wprintf(L"\n%s is stopped.\n", pszServiceName);
        }
        else
        {
            wprintf(L"\n%s failed to stop.\n", pszServiceName);
        }
    }

    // Now remove the service by calling DeleteService.
    if (!DeleteService(hService))
    {
        wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
        goto Cleanup;
    }

    wprintf(L"%s is removed.\n", pszServiceName);

Cleanup:
    // Centralized cleanup for all allocated resources.
	if (hService)
    {
        CloseServiceHandle(hService);
        hService = NULL;
    }

    if (hSCManager)
    {
        CloseServiceHandle(hSCManager);
        hSCManager = NULL;
    }
}

void UpdateServiceInformation(PWSTR pszServiceName, 
								PWSTR pszDisplayName,
								PWSTR pszDisplayDescription,
								DWORD dwStartType,
								PWSTR pszDependencies, 
								PWSTR pszAccount, 
								PWSTR pszPassword)
{
	wchar_t szPath[MAX_PATH];
	SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;

    DWORD result = GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));

	if (result == 0)
	{
		wprintf(L"Unable to update service information w/err 0x%08lx\n", GetLastError());
		return;
	}

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
	{
		hService = OpenService(hSCManager,
								pszServiceName,
								GENERIC_ALL);

		if (hService != NULL)
		{
			ChangeServiceConfig(hService,
								SERVICE_WIN32_OWN_PROCESS,      // Service type
								dwStartType,					// Service start type
								SERVICE_ERROR_NORMAL,           // Error control type
								szPath,                         // Service's binary
								NULL,                           // No load ordering group
								NULL,                           // No tag identifier
								pszDependencies,                // Dependencies
								pszAccount,                     // Service running account
								pszPassword,                    // Password of the account
								pszDisplayName					// Name to display
								);

			SERVICE_DESCRIPTION sd;
			sd.lpDescription = pszDisplayDescription;
			ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, (void*)&sd);

			wprintf(L"%s service registration successfully updated.\n", pszServiceName);
		}
	}

	// Cleanup
	if (hSCManager)
    {
        CloseServiceHandle(hSCManager);
        hSCManager = NULL;
    }

    if (hService)
    {
        CloseServiceHandle(hService);
        hService = NULL;
    }
}