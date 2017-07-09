#ifndef LOG_H
#define LOG_H
#define LOGFILE			"masterlist.log"		// all Log(); messages will be appended to this file
#define MASTER_DEBUG	1						//Used to see the debugging messages (Server added, removed, etc)

extern bool LogCreated;							// keeps track whether the log file is created or not

void LogMSG(const char *fmt, ...);
void Log (char *message);						// logs a message to LOGFILE
void LogErr (char *message);					// logs a message; execution is interrupted
#endif