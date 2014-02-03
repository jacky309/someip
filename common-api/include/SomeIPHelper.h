#pragma once

namespace CommonAPI {
namespace SomeIP {

template<int ...>
struct index_sequence {};


template<int N, int ... S>
struct make_sequence : make_sequence<N - 1, N - 1, S ...> {};

template<int ... S>
struct make_sequence<0, S ...> {
	typedef index_sequence<S ...> type;
};


template<int N, int _Offset, int ... S>
struct make_sequence_range : make_sequence_range<N - 1, _Offset, N - 1 + _Offset, S ...> {};

template<int _Offset, int ... S>
struct make_sequence_range<0, _Offset, S ...> {
	typedef index_sequence<S ...> type;
};

}
}
