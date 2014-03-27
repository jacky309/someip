#pragma once

#include <string>
#include "common.h"
#include "SomeIP-log.h"

namespace SomeIP_utils {

LOG_IMPORT_CONTEXT(serializationContext);

/**
 * Template serializer class which supports optional conversion to network byte order, and data alignment.
 */
template<bool convertToNetworkByteOrder, bool alignmentNeeded>
class Serializer {

	LOG_SET_CLASS_CONTEXT(serializationContext);

public:
	Serializer(ResizeableByteArray& byteArray) :
		payload_(byteArray) {
	}

	void writeValue(bool v) {
		writeBasicTypeValue(v);
	}

	void writeValue(const std::string& s) {
		writeValue( s.c_str(), s.size() );
	}

	void writeValue(const char* s, size_t length) {
		writeBasicTypeValue(length);
		writeRawData(s, length);
	}

	void writeValue(int8_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(int16_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(int32_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(int64_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(uint8_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(uint16_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(uint32_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(uint64_t v) {
		writeBasicTypeValue(v);
	}

	void writeValue(float v) {
		writeBasicTypeValue(v);
	}

	void writeValue(double v) {
		writeBasicTypeValue(v);
	}

	template<typename EnumType>
	void writeEnum(const EnumType& v) {
		typedef typename std::underlying_type<EnumType>::type UnderlyingIntType;
		UnderlyingIntType valueAsInt = static_cast<UnderlyingIntType>(v);
		writeValue(valueAsInt);
	}

	ResizeableByteArray& getContent() {
		return payload_;
	}

	template<typename _BasicType>
	void writeBasicTypeValue(const _BasicType& basicValue) {
		if (alignmentNeeded)
			alignToBoundary( sizeof(_BasicType) );

		_BasicType networkOrderedValue = (convertToNetworkByteOrder ? NativeToNetworkOrder(basicValue) : basicValue);

		log_verbose( "Writing simple type at index %zu , %s", payload_.size(),
			     byteArrayToString( &networkOrderedValue, sizeof(networkOrderedValue) ).c_str() );

		writeRawData( reinterpret_cast<const char*>(&networkOrderedValue), sizeof(networkOrderedValue) );
	}

	template<typename _BasicType>
	void writeBasicTypeValueAt(const _BasicType& basicValue, size_t position) {
		if (alignmentNeeded)
			alignToBoundary( sizeof(_BasicType) );

		_BasicType networkOrderedValue = (convertToNetworkByteOrder ? NativeToNetworkOrder(basicValue) : basicValue);

		log_verbose( "Writing simple type at index %zu , %s", position,
			     byteArrayToString( &networkOrderedValue, sizeof(networkOrderedValue) ).c_str() );

		writeRawDataAt(reinterpret_cast<const char*>(&networkOrderedValue), sizeof(networkOrderedValue), position);
	}

	void writeRawData(const void* rawDataPtr, size_t sizeInByte) {
		payload_.append(rawDataPtr, sizeInByte);
	}

	void writeValueAt(const uint32_t& v, size_t position) {
		writeBasicTypeValueAt(v, position);
	}

	void skip(size_t length) {
		log_verbose("Skipping %zu bytes", length);
		payload_.skip(length);
	}

	void alignToBoundary(size_t alignBoundary) {
		size_t paddingSize = payload_.size() % alignBoundary;
		payload_.skip(paddingSize);
		log_verbose("Inserted %zu bytes padding", paddingSize);
	}

protected:
	void writeRawDataAt(const void* rawDataPtr, size_t sizeInByte, size_t position) {
		memcpy(payload_.getData() + position, rawDataPtr, sizeInByte);
	}

	ResizeableByteArray& payload_;

};

/**
 * Template deserializer class which supports optional conversion from network byte order, and data alignment.
 */
template<bool convertToNetworkByteOrder, bool alignmentNeeded>
class Deserializer {

	LOG_SET_CLASS_CONTEXT(serializationContext);

public:
	void readValue(std::string& v) {
		size_t length;
		readBasicTypeValue(length);
		v.resize(length);
		readRawData(&v[0], length);
	}

	void readValue(bool& v) {
		readBasicTypeValue(v);
	}

	void readValue(int8_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(int16_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(int32_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(int64_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(uint8_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(uint16_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(uint32_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(uint64_t& v) {
		readBasicTypeValue(v);
	}

	void readValue(float& v) {
		readBasicTypeValue(v);
	}

	void readValue(double& v) {
		readBasicTypeValue(v);
	}

	template<typename EnumType>
	void readEnum(EnumType& v) {
		typedef typename std::underlying_type<EnumType>::type UnderlyingIntType;
		UnderlyingIntType valueAsInt;
		readValue(valueAsInt);
		v = static_cast<EnumType>(valueAsInt);
	}

	Deserializer(const void* content, size_t length) : currentDataPosition_(0),
		payload_( static_cast<const unsigned char*>(content) ), m_length(length) {
	}

	void alignToBoundary(size_t alignBoundary) {
		size_t paddingSize = currentDataPosition_ % alignBoundary;
		currentDataPosition_ += paddingSize;
		log_verbose("Skipped %zu bytes padding", paddingSize);
	}

	const unsigned char* readRawData(size_t numBytesToRead) {
		auto v = &payload_[currentDataPosition_];
		currentDataPosition_ += numBytesToRead;
		return v;
	}

	void readRawData(void* destinationBuffer, size_t numBytesToRead) {
		memcpy(destinationBuffer, readRawData(numBytesToRead), numBytesToRead);
	}

	bool hasMoreData() const {
		return (remainingBytesCount() > 0);
	}

	size_t remainingBytesCount() const {
		return m_length - currentDataPosition_;
	}

	size_t position() const {
		return currentDataPosition_;
	}

	template<typename Type>
	void readBasicTypeValue(Type& val) {
		if (alignmentNeeded)
			alignToBoundary( sizeof(Type) );

		auto index = currentDataPosition_;
		(void) index;
		auto p = readRawData( sizeof(Type) );
		val = *( reinterpret_cast<const Type*>(p) );

		log_verbose( "Reading simple type from index %zu, %s, currentDataPosition_", index,
			     byteArrayToString( p, sizeof(val) ).c_str() );

		if (convertToNetworkByteOrder)
			val = NetworkToNativeOrder(val);
	}

	size_t currentDataPosition_;

protected:
	const unsigned char* payload_;
	size_t m_length;

};

class NetworkSerializer : public Serializer<true, false> {
public:
	NetworkSerializer(ResizeableByteArray& byteArray) :
		Serializer<true, false>(byteArray) {
	}

	template<typename T>
	NetworkSerializer& operator<<(T& v) {
		writeValue(v);
		return *this;
	}

};

class NetworkDeserializer : public Deserializer<true, false> {
public:
	NetworkDeserializer(const void* content, size_t length) :
		Deserializer<true, false>(content, length) {
	}

	template<typename T>
	NetworkDeserializer& operator>>(T& v) {
		readValue(v);
		return *this;
	}
};

/**
 * Reserve some space in a stream to store a value of ValueType, and let you write the value afterwards, by calling writeValue().
 */
template<typename ValueType, typename Serializer>
class PlaceHolder {
public:
	PlaceHolder(Serializer& serializer) :
		m_serializer(serializer) {
		m_basePosition = m_serializer.getContent().size();
		m_serializer.skip( sizeof(ValueType) );
	}

	/**
	 * Write the given value to the stream, at the position saved at the construction of this object.
	 */
	void writeValue(ValueType value) {
		m_serializer.writeBasicTypeValueAt(value, m_basePosition);
	}

	Serializer& m_serializer;
	size_t m_basePosition;
};

/**
 * An object of that class reserves some space in the given stream, to store a length information. After having written some data
 * to the stream, the length of this data will be written by the writeLength() method, which is automatically called for
 * you by the constructor, unless you already called it explicitly before.
 */
template<typename LengthType, typename Serializer>
class LengthPlaceHolder : private PlaceHolder<LengthType, Serializer> {
public:
	/**
	 * Constructor
	 * @param offset : Should be set to a non-zero value if the length to be written should not cover some of the bytes which
	 * have been written after this LengthPlaceHolder instance has been created.
	 */
	LengthPlaceHolder(Serializer& serializer, size_t offset = 0) :
		PlaceHolder<LengthType, Serializer>(serializer) {
		m_offset = offset;
	}

	/**
	 * Destructor. writeLength() is called here, unless it has previously been called already
	 */
	~LengthPlaceHolder() {
		if (!m_isWritten)
			writeLength();
	}

	/**
	 * Write the actual length of the data written to the stream since the constructor was called.
	 */
	void writeLength() {
		auto length =
			PlaceHolder<LengthType,
				    Serializer>::m_serializer.getContent().size() -
			PlaceHolder<LengthType, Serializer>::m_basePosition
			- m_offset - sizeof(LengthType);
		PlaceHolder<LengthType, Serializer>::writeValue(length);
		m_isWritten = true;
	}

	size_t m_offset;
	bool m_isWritten = false;
};

}
