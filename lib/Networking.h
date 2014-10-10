#pragma once

#include "string"
#include "SomeIP-common.h"
#include <array>

#include <netdb.h>

namespace SomeIP_Lib {

struct IPV4Address {
	IPV4Address(struct in_addr& addr) {
		numbers = addr;
	}

	void setMask(const uint8_t m[4]) {
		memcpy( mask.data(), m, sizeof(mask) );
	}

	IPV4Address() {
		numbers.s_addr = 0;
	}

	bool operator==(const IPV4Address& right) const {
		return (numbers.s_addr == right.numbers.s_addr);
	}

	/**
	 * Copy a network order
	 */
	IPV4Address& operator=(const std::array<uint8_t,4>& v) {
		numbers.s_addr = 0;
		for(int i= 0;i<4;i++) {
			numbers.s_addr <<= 8;
			numbers.s_addr |= v[i];
		}
		// The internal address if in network order so we need to convert it
		numbers.s_addr = htonl(numbers.s_addr);
		return *this;
	}

	bool isLocalAddress();

	std::string toString() const {
		auto n = toArray();
		return StringBuilder() << n[0] << "." << n[1] << "." << n[2] << "." << n[3];
	}

	std::array<uint8_t, 4> toArray() const {
		std::array<uint8_t, 4> array;
		// The internal address if in network order so we need to convert it
		auto n = htonl(numbers.s_addr);
		for(int i= 3;i>=0;i--) {
			array[i] = n;
			n >>= 8;
		}

		return array;
	}


	const in_addr& getInAddr() const {
		return numbers;
	}

	void setInterfaceName(const char* name) {
		m_interfaceName = name;
	}

	const std::string& getInterfaceName () const {
		return m_interfaceName;
	}

	std::array<uint8_t, 4> mask;
	struct in_addr numbers;
//	std::array<uint8_t, 4> numbers;
	std::string m_interfaceName;
};

//static const uint8_t m_loopbackInterfaceAddressDigits[] = {127, 0, 0, 1};
//static const IPV4Address m_loopbackInterfaceAddress(m_loopbackInterfaceAddressDigits);

inline bool IPV4Address::isLocalAddress() {
	// TODO : implement
	return false;
//	return (m_loopbackInterfaceAddress == *this);
}

typedef uint16_t TCPPort;

struct IPv4TCPEndPoint {

	IPv4TCPEndPoint() {
		m_port = 0;
	}

	IPv4TCPEndPoint(IPV4Address address, TCPPort port) :
		m_address(address), m_port(port) {
	}

	bool operator==(const IPv4TCPEndPoint& right) const {
		if ( !(m_address == right.m_address) )
			return false;
		if ( !(m_port == right.m_port) )
			return false;
		return true;
	}

	std::string toString() const {
		return StringBuilder() << m_address.toString() << "/" << m_port;
	}

	const IPV4Address& getAddress() {
		return m_address;
	}

	IPV4Address m_address;
	TCPPort m_port;
};

struct BlackListHostFilter {

	virtual ~BlackListHostFilter() {
	}

	virtual bool isBlackListed(const IPv4TCPEndPoint& server, ServiceIDs serviceID = ServiceIDs(0xFFFF, 0xFFFF)) const = 0;
};

}
