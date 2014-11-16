/**
 * thread safe logger
 */
#ifndef _KMQLOG_h
#define _KMQLOG_h

#include <iostream>
#include <sys/syscall.h>
#include <sharelib/util/log4cpp_wrapper.h>
#include "os.h"
#include "decr.h"


KMQ_DECLARATION_START

using namespace std;
using namespace sharelib;

static inline pid_t __tid()
{
    return syscall(SYS_gettid);
}

enum SHARELIB_LOG_LEVEL{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
};

extern ThreadSafeLog *kmq_logger;

#define KMQLOG_CONFIG(filename) do {					\
	try {								\
	    sharelib::Log4cppWrapper::Init(filename);			\
	} catch(std::exception &e) {					\
	    std::cerr << "Error!!! Failed to configure logger"		\
		      << e.what() << std::endl;				\
	    exit(-1);							\
	}								\
	kmq_logger = sharelib::Log4cppWrapper::GetLog("kmq");	\
    }while(0)



#define KMQLOG_SHUTDOWN() sharelib::Log4cppWrapper::Shutdown()
#define KMQLOG_SHUTDOWN_BEFORE() sharelib::Log4cppWrapper::ShutdownBeforeLogr()


#define __LOG_DEBUG(logger, fmt, ...) do {			\
	(logger)->Debug("tid:%d %s:%d %s(), "#fmt, __tid(),	\
			basename(__FILE__),			\
			__LINE__, __func__, ##__VA_ARGS__);	\
    } while (0) 

#define __LOG_INFO(logger, fmt, ...) do {			\
	(logger)->Info("tid:%d %s:%d %s(), "#fmt, __tid(),	\
		       basename(__FILE__),			\
		       __LINE__, __func__, ##__VA_ARGS__);	\
    } while (0) 

#define __LOG_NOTICE(logger, fmt, ...) do {			\
	(logger)->Notice("tid:%d %s:%d %s(), "#fmt, __tid(),	\
			 basename(__FILE__),			\
			 __LINE__, __func__, ##__VA_ARGS__);	\
    } while (0) 


#define __LOG_WARN(logger, fmt, ...) do {			\
	(logger)->Warn("tid:%d %s:%d %s(), "#fmt, __tid(),	\
		       basename(__FILE__),			\
		       __LINE__, __func__, ##__VA_ARGS__);	\
    } while (0) 


#define __LOG_ERROR(logger, fmt, ...) do {			\
	(logger)->Error("tid:%d %s:%d %s(), "#fmt, __tid(),	\
			basename(__FILE__),			\
			__LINE__, __func__, ##__VA_ARGS__);	\
    } while (0) 


#define KMQLOG_DEBUG(format, args...) __LOG_DEBUG(kmq_logger, format, ##args)
#define KMQLOG_INFO(format, args...) __LOG_INFO(kmq_logger, format, ##args)
#define KMQLOG_NOTICE(format, args...) __LOG_NOTICE(kmq_logger, format, ##args)
#define KMQLOG_WARN(format, args...) __LOG_WARN(kmq_logger, format, ##args)
#define KMQLOG_ERROR(format, args...) __LOG_ERROR(kmq_logger, format, ##args)





}
#endif

