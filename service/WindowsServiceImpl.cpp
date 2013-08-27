// WindowsServiceImpl.cpp : Entry point when started from SCM
//

#pragma region Includes
#include "WindowsServiceImpl.h"
#include "ThreadPool.h"
#include "RedisDll.h"
#pragma endregion

#define OPERATION_TIMEOUT 1000
#define SERVICE_STOP_TIMEOUT 25000

CWindowsServiceImpl::CWindowsServiceImpl(PWSTR pszServiceName, 
                               BOOL fCanStop, 
                               BOOL fCanShutdown, 
                               BOOL fCanPauseContinue) : CServiceBase(pszServiceName, fCanStop, fCanShutdown, fCanPauseContinue)
{
	m_hStoppingEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Create a manual-reset event that is not signaled at first to indicate 
    // the stopped signal of the service.
    m_hStoppedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hWorkThread = INVALID_HANDLE_VALUE;
}


CWindowsServiceImpl::~CWindowsServiceImpl(void)
{
	if (m_hStoppingEvent)
	{
		CloseHandle(m_hStoppingEvent);
		m_hStoppingEvent = NULL;
	}

	if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        m_hStoppedEvent = NULL;
    }
}

void CWindowsServiceImpl::OnStart(DWORD dwArgc, LPWSTR *lpszArgv)
{
    // Log a service start message to the Application log.
    WriteEventLogEntry(L"Redis is starting...", 
        EVENTLOG_INFORMATION_TYPE);

    // Queue the main service function for execution in a worker thread.
    CThreadPool::QueueUserWorkItem(&CWindowsServiceImpl::ServiceWorkerThread, this);
}

void CWindowsServiceImpl::ServiceWorkerThread(void)
{
	// Determine current thread handle and store it for future use
	m_hWorkThread = GetCurrentThread();

	// Start Redis
	startRedisServer();

    // Periodically check if the service is stopping.
    while (WaitForSingleObject(m_hStoppingEvent, OPERATION_TIMEOUT) != WAIT_OBJECT_0)
    {
        // Keep loop running only to have thread alive
		// Redis is working in parallel thread(s)
		Sleep(1);
    }

	// Stop Redis
	stopRedisServer();

    // Signal the stopped event.
    SetEvent(m_hStoppedEvent);
}

void CWindowsServiceImpl::OnStop()
{
    // Log a service stop message to the Application log.
    WriteEventLogEntry(L"Redis is stopping...", EVENTLOG_INFORMATION_TYPE);

    // Indicate that the service is stopping and wait for the finish of the 
    // main service function (ServiceWorkerThread).
	SetEvent(m_hStoppingEvent);

	// Wait until service loop finishes
	DWORD result = WaitForSingleObject(m_hStoppedEvent, SERVICE_STOP_TIMEOUT);

	switch (result)
	{
		case WAIT_TIMEOUT:
			// Terminate thread if not stopped within timeout limits
			if (m_hWorkThread != INVALID_HANDLE_VALUE)
				TerminateThread(m_hWorkThread, S_OK);

			WriteEventLogEntry(L"Unable to stop Redis within time limits, terminating...", EVENTLOG_ERROR_TYPE);
			break;
		case WAIT_OBJECT_0:
			break;
		default:
			break;
	};

	WriteEventLogEntry(L"Redis stopped", EVENTLOG_INFORMATION_TYPE);
}