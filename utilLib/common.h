#pragma once

#include "cstdio"
#include <string>
#include <exception>
#include <vector>
#include <memory.h>
//#include <atomic>
#include <errno.h>
#include <type_traits>

#include <assert.h>

namespace SomeIP_utils {

std::string byteArrayToString(const void* buffer, size_t length);

inline size_t getPointerAsInt(const void* p) {
	return reinterpret_cast<size_t>(p);
}

/**
 * Returns an enumeration value which matches the string given as parameter.
 * The following values should be defined as part of the enumeration : MIN, MAX, INVALID
 */
template<typename EnumType>
inline EnumType string2Enum(const std::string& str) {
	for ( EnumType t = EnumType::MIN; t < EnumType::MAX; t =
		      static_cast<EnumType>(static_cast<int>(t) + 1) ) {
		if (toString(t) == str)
			return t;
	}
	return EnumType::INVALID;
}

/**
 * Returns an enumeration value which matches the string given as parameter.
 * The following values should be defined as part of the enumeration : MIN, MAX, INVALID
 */
template<typename EnumType>
inline EnumType string2Enum(const char* str) {
	return string2Enum<EnumType>( std::string(str) );
}

#define RETURN_ENUM_NAME_IF_EQUAL(value, type, v) if (value == type::v) return # v

/**
 * Remove the first occurrence of the given element from the vector
 */
template<typename T> void removeFromVector(std::vector<T*>& v, T* elementToRemove) {
	for (auto i = v.begin(); i < v.end(); i++) {
		T* existingFromVector = *i;
		if (elementToRemove == existingFromVector) {
			v.erase(i);
			return;
		}
	}
}

inline bool isNativeBigEndian() {
	short int number = 1;
	char* numPtr = (char*) &number;
	return (numPtr[0] != 1);
}

inline void swapBytes(void* data, size_t length) {
	char* chars = reinterpret_cast<char*>(data);
	for (size_t i = 0; i < length / 2; i++)
		std::swap(chars[i], chars[length - i - 1]);
}

template<typename T>
inline T NativeToNetworkOrder(const T& v) {
	if ( isNativeBigEndian() )
		return v;
	else {
		T networkValue = v;
		swapBytes( &networkValue, sizeof(networkValue) );
		return networkValue;
	}
}

template<typename T>
inline T NetworkToNativeOrder(const T& v) {
	if ( isNativeBigEndian() )
		return v;
	else {
		T networkValue = v;
		swapBytes( &networkValue, sizeof(networkValue) );
		return networkValue;
	}
}

/**
 * A vector-like class which uses a statically allocated buffer if the data is small enough.
 */
class ByteArray {

public:
	ByteArray() {
		m_dynamicData = NULL;
		m_length = 0;

#ifndef NDEBUG
		memset( m_staticData, 0, sizeof(m_staticData) );
#endif
	}

	ByteArray(const ByteArray& b) {
		// duplicate content
		append( b.getData(), b.size() );
	}

	ByteArray& operator=(const ByteArray& right) {
		resize( right.size() );
		memcpy( getData(), right.getData(), right.size() );
		return *this;
	}

	~ByteArray() {
		if ( !usesStaticBuffer() )
			delete (m_dynamicData);
	}

	size_t size() const {
		return ( usesStaticBuffer() ? m_length : m_dynamicData->size() );
	}

	bool usesStaticBuffer() const {
		return (m_dynamicData == NULL);
	}

	unsigned char* getData() {
		if ( usesStaticBuffer() )
			return m_staticData;
		else
			return &( (*m_dynamicData)[0] );
	}

	unsigned const char* getData() const {
		if ( usesStaticBuffer() )
			return m_staticData;
		else
			return &( (*m_dynamicData)[0] );
	}

	unsigned char& operator[](size_t index) {
		//		assert(index < m_length);
		return getData()[index];
	}

	void resize(size_t size) {

		if ( !usesStaticBuffer() ) {
			m_dynamicData->resize(size);
		} else {
			if ( size <= sizeof(m_staticData) ) {
				m_length = size;
			} else {
				m_dynamicData = new std::vector<unsigned char>();
				m_dynamicData->resize(size);
				memcpy(m_dynamicData->data(), m_staticData, m_length);
				m_length = -1; // this field is not relevant anymore
			}
		}

	}

	void writeAt(size_t position, const void* rawDataPtr, size_t sizeInByte) {
		assert(m_length >= position + sizeInByte);
		memcpy(&(getData()[position]), rawDataPtr, sizeInByte);
	}

	void append(const void* rawDataPtr, size_t sizeInByte) {
		auto i = size();
		resize(size() + sizeInByte);
		memcpy(getData() + i, rawDataPtr, sizeInByte);
	}

	void append(unsigned char v) {
		auto i = size();
		resize(size() + 1);
		getData()[i] = v;
	}

	/**
	 * Reserve space for a block of the given size and return the position of the block
	 */
	size_t skip(size_t length) {
		auto returnValue = size();
		char dummyData[length];
		memset(dummyData, 0, length);
		append( dummyData, sizeof(dummyData) );
		return returnValue;
	}

	std::string toString() const {
		return byteArrayToString( getData(), size() );
	}

private:
	std::vector<unsigned char>* m_dynamicData = nullptr;
	unsigned char m_staticData[1024];
	size_t m_length = 0;
};

typedef ByteArray ResizeableByteArray;

class Exception : public std::exception {
public:
	Exception(const char* msg) :
		m_err(msg) {
	}

	void setMessage(std::string msg) {
		m_err = msg;
	}

	const char* what() const throw (){
		return m_err.c_str();
	}

private:
	std::string m_err;
};

class ConnectionException : public Exception {
public:
	ConnectionException(const char* msg) :
		Exception(msg) {
	}

};

class ConnectionExceptionWithErrno : public ConnectionException {
public:
	ConnectionExceptionWithErrno(const char* msg) :
		ConnectionException(msg) {
		std::string s(msg);
		s += " Error: ";
		s += strerror(errno);
		setMessage(s);
	}
};

class OperationUnsupportedException : public std::exception {

public:
	OperationUnsupportedException(const char* message) {
		m_err = message;
	}

	virtual ~OperationUnsupportedException() throw (){
	}

	const char* what() const throw (){
		return m_err.c_str();
	}

private:
	std::string m_err;

};

std::string getProcessName(pid_t pid);
pid_t getPidFromFiledescriptor(int fd);

}
