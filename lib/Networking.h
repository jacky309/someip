#pragma once

#include "string"
#include "SomeIP-common.h"
#include <array>

namespace SomeIP_Lib {

struct IPV4Address {
	IPV4Address(const uint8_t n[4]) {
		memcpy( numbers.data(), n, sizeof(numbers) );
	}

	IPV4Address() {
		numbers[0] = numbers[1] = numbers[2] = numbers[3] = 0;
	}

	bool operator==(const IPV4Address& right) const {
		for (size_t i = 0; i < numbers.size(); i++)
			if (numbers[i] != right.numbers[i])
				return false;
		return true;
	}

	bool isLocalAddress();

	std::string toString() const {
		char buffer[1000];
		snprintf(buffer, sizeof(buffer), "%i.%i.%i.%i", numbers[0], numbers[1], numbers[2], numbers[3]);
		return buffer;
	}

	std::array<uint8_t, 4> numbers;
};

static const uint8_t m_loopbackInterfaceAddressDigits[] = {127, 0, 0, 1};
static const IPV4Address m_loopbackInterfaceAddress(m_loopbackInterfaceAddressDigits);

inline bool IPV4Address::isLocalAddress() {
	return (m_loopbackInterfaceAddress == *this);
}

typedef uint16_t TCPPort;

struct IPv4TCPServerIdentifier {

	IPv4TCPServerIdentifier() {
		m_port = 0;
	}

	IPv4TCPServerIdentifier(IPV4Address address, TCPPort port) :
		m_address(address), m_port(port) {
	}

	bool operator==(const IPv4TCPServerIdentifier& right) const {
		if ( !(m_address == right.m_address) )
			return false;
		if ( !(m_port == right.m_port) )
			return false;
		return true;
	}

	std::string toString() const {
		char buffer[1000];
		snprintf(buffer, sizeof(buffer), "%s/%i", m_address.toString().c_str(), m_port);
		return buffer;
	}

	IPV4Address m_address;
	TCPPort m_port;
};

}
