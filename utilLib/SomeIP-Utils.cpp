#include "SomeIP-log.h"
#include <string>
#include <dirent.h>
#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

namespace SomeIP_utils {

LOG_DECLARE_DEFAULT_CONTEXT(libContext, "lib", "lib");

LOG_DECLARE_CONTEXT(serializationContext, "PSer", "Serialization");

std::string getProcessName(pid_t pid) {

	DIR* dir;
	FILE* file;
	char path[64];
	char processName[256];
	//	int n;

	if (pid < 0)
		return NULL;

	dir = opendir("/proc");
	if (!dir)
		return "Unknown process";

	snprintf(path, sizeof(path), "/proc/%i/cmdline", pid);
	file = fopen(path, "r");

	if (file == NULL) {
		closedir(dir);
		return "Unknown process";
	}

	fread(processName, 1, sizeof(processName) - 1, file);
	fclose(file);
	closedir(dir);

	return processName;
}


pid_t getPidFromFiledescriptor(int fd) {
#ifdef __linux__
	/* get the PID of the client application */
	struct ucred peercred;
	socklen_t so_len = sizeof(peercred);
	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peercred, &so_len) == -1) {
		log_error() << "Cannot get PID of client process!";
		return 0;
	}
	return peercred.pid;

#else
	return 0;
#endif
}

}
