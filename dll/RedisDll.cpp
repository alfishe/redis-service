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

REDIS_DLL_API int startRedisServer()
{
	struct timeval tv;

	// We need to initialize our libraries, and the server configuration.
	zmalloc_enable_thread_safeness();
	zmalloc_set_oom_handler(redisOutOfMemoryHandler);
	srand((unsigned int)time(NULL) ^ getpid());
	gettimeofday(&tv,NULL);

	dictSetHashFunctionSeed(tv.tv_sec^tv.tv_usec^getpid());

	// TODO: read sentinel settings from config or accept as DLL call parameter
	server.sentinel_mode = checkForSentinelMode(0, NULL);

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
    aeMain(server.el);
    aeDeleteEventLoop(server.el);

	return S_OK;
}

REDIS_DLL_API int stopRedisServer()
{
	// Shutdown Redis server
	callsigtermHandler(1);

	return S_OK;
}

// This is the constructor of a class that has been exported.
// see dll.h for the class definition
Cdll::Cdll()
{
	return;
}
