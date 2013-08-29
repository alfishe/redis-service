// dll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "RedisDll.h"

extern "C"
{
	#include "redis.h"
	#include "zmalloc.h"
	#include "win32fixes.h"

	// Declare functions missed from original redis.h
	void redisOutOfMemoryHandler(size_t allocation_size);
	int checkForSentinelMode(int argc, char **argv);
	void initServerConfig();
	void initServer();
	void redisAsciiArt(void);
	void loadDataFromDisk(void);
	void beforeSleep(struct aeEventLoop *eventLoop);
	void callsigtermHandler(int sig);
}

HANDLE hWorkerThread = NULL;
HANDLE hStartedEvent = NULL;

REDIS_DLL_API int startRedisServer()
{
	int result = S_FALSE;

	hStartedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	hWorkerThread = CreateThread(
		NULL,                   // default security attributes
        0,                      // use default stack size  
        ThreadWorker,			// thread function name
        NULL,					// argument to thread function 
        0,                      // use default creation flags 
        NULL);   // returns the thread identifier

	if (hWorkerThread != NULL)
	{
		result = S_OK;
	}

	return result;
}

REDIS_DLL_API int stopRedisServer()
{
	// Redis already stopped or currently is shutting down
	if (hWorkerThread == NULL || server.shutdown_asap == 1)
		return S_FALSE;

	// Shutdown Redis server (this will interrupt main processing loop
	server.shutdown_asap = 1;

	// Wait for Redis thread to stop
	DWORD result = WaitForSingleObject(hWorkerThread, INFINITE);

	if (result == WAIT_OBJECT_0)
	{
		if (server.el)
		{
			// Cleanup
			aeDeleteEventLoop(server.el);
		}

		CloseHandle(hWorkerThread);
		hWorkerThread = NULL;
	}

	if (hStartedEvent != NULL)
	{
		CloseHandle(hStartedEvent);
		hStartedEvent = NULL;
	}

	return S_OK;
}

DWORD WINAPI ThreadWorker(LPVOID lpParam) 
{
	struct timeval tv;

	// DEBUG
	/*
	if (executedasservice())
	{
		static char message[200];

		// Notify calling thread immediately to prevent SCM timeout
		SetEvent(hStartedEvent);

		// Make a delay to attach debugger
		for (int i = 30; i >= 0; i--)
		{
			sprintf_s(message, "Service functionality will be resumed in... %02d secs\n", i);
			OutputDebugString(message);

			Sleep(1000);
		}
	}
	*/
	// END OF DEBUG

	// We need to initialize our libraries, and the server configuration.
	zmalloc_enable_thread_safeness();
	zmalloc_set_oom_handler(redisOutOfMemoryHandler);
	srand((unsigned int)time(NULL) ^ getpid());
	gettimeofday(&tv, NULL);

	dictSetHashFunctionSeed(tv.tv_sec ^ tv.tv_usec ^ getpid());

	// TODO: Allow to use sentinel mode
	server.sentinel_mode = 0; // checkForSentinelMode(0, NULL);

	initServerConfig();

	// We need to init sentinel right now as parsing the configuration file
	// in sentinel mode will have the effect of populating the sentinel
	// data structures with master nodes to monitor.
	if (server.sentinel_mode)
	{
		initSentinelConfig();
		initSentinel();
	}

    initServer();
    cowInit();

	redisAsciiArt();

	if (!server.sentinel_mode)
	{
        // Things only needed when not running in Sentinel mode.
        redisLog(REDIS_WARNING,"Server started, Redis version " REDIS_VERSION);

        loadDataFromDisk();

		if (server.ipfd > 0)
            redisLog(REDIS_NOTICE,"The server is now ready to accept connections on port %d", server.port);

        if (server.sofd > 0)
            redisLog(REDIS_NOTICE,"The server is now ready to accept connections at %s", server.unixsocket);
    }

    // Warning the user about suspicious maxmemory setting.
    if (server.maxmemory > 0 && server.maxmemory < 1024*1024)
	{
        redisLog(REDIS_WARNING,"WARNING: You specified a maxmemory value that is less than 1MB (current value is %llu bytes). Are you sure this is what you really want?", server.maxmemory);
    }

    aeSetBeforeSleepProc(server.el, beforeSleep);

	// Notify executing thread about redis thread started successfully
	SetEvent(hStartedEvent);

	// Execute Redis main processing loop
    aeMain(server.el);
    
	// Cleanup in case main loop exits (which never happens)
	aeDeleteEventLoop(server.el);

	return S_OK;
}

// This is the constructor of a class that has been exported.
// see dll.h for the class definition
Cdll::Cdll()
{
	return;
}
