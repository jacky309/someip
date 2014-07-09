#pragma once

#include "SomeIP-Serialization.h"
#include <memory>
#include "CommonAPI-SomeIP.h"

namespace SomeIP_Lib {

template<typename T>
SomeIPInputStream& operator>>(SomeIPInputStream& stream, std::shared_ptr<T>& v) {
	stream >> *v.get();
	return stream;
}

template<typename T>
SomeIPOutputStream& operator<<(SomeIPOutputStream& stream, const std::shared_ptr<T>& v) {
	stream << *v.get();
	return stream;
}

}
