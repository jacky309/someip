#pragma once

#include "CommonAPI-SomeIP.h"
#include "SomeIP-Serialization.h"

namespace CommonAPI {
namespace SomeIP {

template<typename ... _Arguments>
struct SerializableArguments;

template<>
struct SerializableArguments<> {
	static inline bool serialize(SomeIPOutputStream& outputStream) {
		return true;
	}

	static inline bool deserialize(SomeIPInputStream& inputStream) {
		return true;
	}
};

template<typename _ArgumentType>
struct SerializableArguments<_ArgumentType> {
	static inline bool serialize(SomeIPOutputStream& outputStream, const _ArgumentType& argument) {
		outputStream << argument;
		return !outputStream.hasError();
	}

	static inline bool deserialize(SomeIPInputStream& inputStream, _ArgumentType& argument) {
		inputStream >> argument;
		return !inputStream.hasError();
	}
};

template<typename _ArgumentType, typename ... _Rest>
struct SerializableArguments<_ArgumentType, _Rest ...> {
	static inline bool serialize(SomeIPOutputStream& outputStream, const _ArgumentType& argument, const _Rest& ... rest) {
		outputStream << argument;
		const bool success = !outputStream.hasError();
		return success ? SerializableArguments<_Rest ...>::serialize(outputStream, rest ...) : false;
	}

	static inline bool deserialize(SomeIPInputStream& inputStream, _ArgumentType& argument, _Rest& ... rest) {
		inputStream >> argument;
		const bool success = !inputStream.hasError();
		return success ? SerializableArguments<_Rest ...>::deserialize(inputStream, rest ...) : false;
	}
};

}     // namespace HPerf
} // namespace CommonAPI
