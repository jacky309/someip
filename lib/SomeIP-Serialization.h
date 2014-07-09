#pragma once

#include "CommonAPI/SerializableVariant.h"
#include "SomeIP-common.h"
#include "utilLib/serialization.h"

namespace SomeIP_Lib {

using CommonAPI::Variant;

using SomeIP_utils::Serializer;
using SomeIP_utils::Deserializer;

class SomeIPOutputStream;
class SomeIPInputStream;

inline bool isAlignmentNeeded() {
	return false;
}

typedef SomeIP_utils::Serializer<true, false> SomeIPSerializer;
typedef SomeIP_utils::Deserializer<true, false> SomeIPDeserializer;

class SomeIPOutputStream : public SomeIPSerializer {

	LOG_DECLARE_CLASS_CONTEXT("SIOS", "SomeIPOutputStream");

public:
	using SomeIPSerializer::writeValue;
	using SomeIPSerializer::writeRawData;

	SomeIPOutputStream(ResizeableByteArray& byteArray) : SomeIPSerializer(byteArray) {
	}

	struct OutputStreamWriteVisitor {
public:
		OutputStreamWriteVisitor(SomeIPOutputStream& outputStream) :
			outputStream_(outputStream) {
		}

		template<typename _Type>
		void operator()(const _Type& value) const {
			outputStream_.writeValue(value);
		}

		void writeType(uint32_t typeIndex) const {
			outputStream_.writeValue(typeIndex);
		}

private:
		SomeIPOutputStream& outputStream_;
	};

	bool hasError() const {
		return false;
	}

protected:
};

template<typename T>
SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const T& v) {
	stream.writeValue(v);
	return stream;
}

inline SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const char& v) {
	stream.writeValue(v);
	return stream;
}

template<typename ... _Types>
SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const Variant<_Types ...>& v) {

	LengthPlaceHolder<uint32_t, SomeIPSerializer> lengthPlaceHolder(stream);

	uint32_t type = v.getValueType();
	stream << type;

	writeToOutputStream(v, stream);
	stream.alignToBoundary(4);

	return stream;
}

inline SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const char* stringValue) {
	uint32_t stringLength = strlen(stringValue);
	stream.writeValue(stringLength);
	stream.writeRawData(stringValue, stringLength);
	return stream;
}

inline SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const std::string& stringValue) {
	uint32_t stringLength = stringValue.size();
	stream.writeValue(stringLength);
	stream.writeRawData(&stringValue[0], stringLength);

	return stream;
}

template<typename T>
SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const std::vector<T>& v) {
	uint32_t size = v.size();
	stream.writeValue(size);
	for (auto& element : v) {
		stream << element;
	}

	return stream;
}

template<typename KeyType, typename ValueType>
SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const std::pair<KeyType, ValueType>& v) {
	stream << v.first;
	stream << v.second;
	return stream;
}

template<typename KeyType, typename ValueType, typename Hasher>
SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const std::unordered_map<KeyType, ValueType, Hasher>& v) {
	uint32_t size = v.size();
	stream.writeValue(size);
	for (auto& element : v) {
		stream << element;
	}

	return stream;
}


class SomeIPInputStream : public SomeIPDeserializer {

	LOG_DECLARE_CLASS_CONTEXT("SIIS", "SomeIPInputStream");

public:
	using SomeIPDeserializer::readValue;

	SomeIPInputStream(const void* content, size_t length) : SomeIPDeserializer(content, length) {
	}

	bool hasError() const {
		return false;
	}

};

template<typename T>
SomeIPInputStream& operator>>(SomeIPInputStream& stream, T& v) {
	stream.readValue(v);
	return stream;
}

inline SomeIPInputStream& operator>>(SomeIPInputStream& stream, std::string& stringValue) {
	uint32_t stringLength;
	stream.readValue(stringLength);
	stringValue.resize(stringLength);
	stream.readRawData(&(stringValue[0]), stringLength);

	return stream;
}

inline SomeIPInputStream& operator>>(SomeIPInputStream& stream, CommonAPI::Version& v) {
	return stream;
}

template<typename T> SomeIPInputStream& operator>>(SomeIPInputStream& stream, std::vector<T>& v) {
	uint32_t size;
	stream.readValue(size);
	for (size_t i = 0; i < size; i++) {
		T element;
		stream >> element;
		v.push_back(element);
	}
	return stream;
}

template<typename ... _Types>
SomeIPInputStream& operator>>(SomeIPInputStream& stream, Variant<_Types ...>& v) {

	uint32_t length;
	stream.readValue(length);

	auto initialPosition = stream.currentDataPosition_;

	uint32_t typeIndex;
	stream.readValue(typeIndex);

	readFromInputStream(v, typeIndex, stream);

	stream.currentDataPosition_ = initialPosition + length;
	return stream;
}

template<typename KeyType, typename ValueType, typename Hasher>
SomeIPInputStream& operator>>(SomeIPInputStream& stream, std::unordered_map<KeyType, ValueType, Hasher>& v) {
	uint32_t size;
	stream.readValue(size);
	for (size_t i = 0; i < size; i++) {
		KeyType key;
		ValueType value;
		stream >> key;
		stream >> value;
		v[key] = value;
	}
	return stream;
}

struct OutputStreamWriteVisitor {
public:
	OutputStreamWriteVisitor(SomeIPOutputStream& outputStream) :
		outputStream_(outputStream) {
	}

	template<typename _Type>
	void operator()(const _Type& value) const {
		outputStream_ << value;
	}

private:
	SomeIPOutputStream& outputStream_;
};

template<typename ... _Types>
void writeToOutputStream(const Variant<_Types ...>& variant, SomeIPOutputStream& outputStream) {
	OutputStreamWriteVisitor visitor(outputStream);
	CommonAPI::ApplyVoidVisitor<OutputStreamWriteVisitor, Variant<_Types ...>, _Types ...>::visit(visitor, variant);
}

template<class Variant, typename ... _Types> struct SomeIPVariantDeserializer;

template<class Variant>
struct SomeIPVariantDeserializer<Variant> {
	static const uint8_t index = 0;

	static void deserialize(Variant&, SomeIPInputStream& inputStream, uint8_t typeIndex) {
		//won't be called
		assert(false);
	}

	static void deserialize(const Variant&, SomeIPInputStream& inputStream, uint8_t typeIndex) {
		//won't be called
		assert(false);
	}
};

template<class Variant, typename _Type, typename ... _Types>
struct SomeIPVariantDeserializer<Variant, _Type, _Types ...> {

	/// The index which this template specialization handle
	static const uint8_t index = SomeIPVariantDeserializer<Variant, _Types ...>::index + 1;

	static void deserialize(Variant& var, SomeIPInputStream& inputStream, uint8_t typeIndex) {
		if (typeIndex == index) {
			_Type value;
			inputStream >> value;
			var = value;
		} else {
			SomeIPVariantDeserializer<Variant, _Types ...>::deserialize(var, inputStream, typeIndex);
		}
	}

};

template<typename ... _Types>
void readFromInputStream(Variant<_Types ...>& variant, uint8_t typeIndex, SomeIPInputStream& inputStream) {
	SomeIPVariantDeserializer<Variant<_Types ...>, _Types ...>::deserialize(variant, inputStream, typeIndex);
}

}
