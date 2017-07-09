#include <stdlib.h> 
#include <stdio.h> 
#include "log.h" 
#include <Windows.h>
#include <time.h>       /* time_t, struct tm, time, localtime, strftime */
bool LogCreated = false;

void LogMSG(const char *fmt, ...) {
	char buffer[256], timestamp[80];
	va_list arg;
	/* Write the error message */
	va_start(arg, fmt);
	vsprintf (buffer,fmt, arg);
	#if MASTER_DEBUG
		printf(buffer);
	#endif
	va_end(arg);
	/*
	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);

	strftime (timestamp,sizeof(timestamp)-1,"[%H:%M:%S]",timeinfo);
	sprintf(buffer,"%s: %s",timestamp, buffer);
	*/
	Log(buffer);
} 
void Log(char *message) {
	FILE *file;
	if (!LogCreated) {
		file = fopen(LOGFILE, "w");
		LogCreated = true;
	} else {
		file = fopen(LOGFILE, "a");
	}
	if (file == NULL) {
		if (LogCreated) {
			LogCreated = false;
			return;
		}
	} else {
		fputs(message, file);
		fclose(file);
	}
	if (file)
		fclose(file);
}

void LogErr(char *message) {
	Log(message);
	Log("\n");
}