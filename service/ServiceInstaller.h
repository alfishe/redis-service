#pragma once
#include <windows.h>

void InstallService(PWSTR pszServiceName, 
					PWSTR pszDisplayName, 
					PWSTR pszDisplayDescription,
					DWORD dwStartType,
					PWSTR pszDependencies, 
					PWSTR pszAccount, 
					PWSTR pszPassword);

void UninstallService(PWSTR pszServiceName);

void UpdateServiceInformation(PWSTR pszServiceName, 
								PWSTR pszDisplayName,
								PWSTR pszDisplayDescription,
								DWORD dwStartType,
								PWSTR pszDependencies, 
								PWSTR pszAccount, 
								PWSTR pszPassword);