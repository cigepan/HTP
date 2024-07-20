
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

//#define _WIN32_WINNT _WIN32_WINNT_WINXP
//#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <map>
#include <functional>
#include <string>
#include<time.h>
#include <fstream>



#ifdef _USE_MONGODB
#include "bsoncxx/builder/stream/document.hpp"
#include "mongocxx/instance.hpp"
#include "mongocxx/uri.hpp"
#include "mongocxx/client.hpp"
#endif 

#ifdef _USE_HPTCP
#define _UDP_DISABLED
#define _SSL_DISABLED
#define _HTTP_DISABLED
#define _ZLIB_DISABLED
#define _BROTLI_DISABLED
#include "GeneralHelper.h"
#endif

#include "Semaphore.h"
#include <windows.h>
#include <iostream>

#ifdef _WINDOWS
#include <shellapi.h>
#endif // DEBUG
#include "ThostFtdcUserApiStruct.h"
#include "CMyLib.h"
#include "CFastKey.h"
#include "CCycleQueue.h"
