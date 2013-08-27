#pragma once
#include "servicebase.h"
#pragma once
#include "ServiceBase.h"

class CWindowsServiceImpl : public CServiceBase
{
	// Allow to call ServiceWorkerThread() directly from main() and shutdown() methods for CLI mode
	friend int wmain(int argc, wchar_t *argv[]);
	friend void shutdown(void);

public:
	CWindowsServiceImpl(PWSTR pszServiceName, 
        BOOL fCanStop = TRUE, 
        BOOL fCanShutdown = TRUE, 
        BOOL fCanPauseContinue = FALSE);
	~CWindowsServiceImpl(void);

protected:
	virtual void OnStart(DWORD dwArgc, PWSTR *pszArgv);
	virtual void OnStop();

    void ServiceWorkerThread(void);
private:
    HANDLE m_hStoppingEvent;
    HANDLE m_hStoppedEvent;
	HANDLE m_hWorkThread;
};

