#pragma once

#include <vector>
#include "stdint.h"
#include "ServiceDiscovery.h"

#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

typedef uint16_t TCPPort;

class DaemonConfiguration {

public:
	static const uint16_t DEFAULT_TCP_SERVER_PORT = 10032;

	DaemonConfiguration() {
	}

	TCPPort getDefaultLocalTCPPort() const {
		return m_tcpPort;
	}

	void setDefaultLocalTCPPort(int port) {
		m_tcpPort = port;
	}

private:
	TCPPort m_tcpPort = DEFAULT_TCP_SERVER_PORT;

};

const DaemonConfiguration& getConfiguration();
