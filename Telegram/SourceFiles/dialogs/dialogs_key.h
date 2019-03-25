/*
This file is part of Bettergram.

For license and copyright information please follow this link:
https://github.com/bettergram/bettergram/blob/master/LEGAL
*/
#pragma once

#include "base/value_ordering.h"

class History;

namespace Data {
class Feed;
} // namespace Data

namespace Dialogs {

class Entry;

class Key {
public:
	Key() = default;
	Key(History *history) : _value(history) {
	}
	Key(not_null<History*> history) : _value(history) {
	}
	Key(Data::Feed *feed) : _value(feed) {
	}
	Key(not_null<Data::Feed*> feed) : _value(feed) {
	}

	explicit operator bool() const {
		return !!_value;
	}
	not_null<Entry*> entry() const;
	History *history() const;
	Data::Feed *feed() const;
	PeerData *peer() const;

	uint64 id() const;

	inline bool operator<(const Key &other) const {
		return _value < other._value;
	}
	inline bool operator>(const Key &other) const {
		return (other < *this);
	}
	inline bool operator<=(const Key &other) const {
		return !(other < *this);
	}
	inline bool operator>=(const Key &other) const {
		return !(*this < other);
	}
	inline bool operator==(const Key &other) const {
		return _value == other._value;
	}
	inline bool operator!=(const Key &other) const {
		return !(*this == other);
	}
	bool isNull() {
		if (const not_null<History*> *p = base::get_if<not_null<History*>>(&_value)) {
			return p->get() == nullptr;
		}
		else if (const not_null<Data::Feed*> *p = base::get_if<not_null<Data::Feed*>>(&_value)) {
			return p->get() == nullptr;
		}
		else {
			return true;
		}
	}

	base::optional_variant<
		not_null<History*>,
		not_null<Data::Feed*>> raw() const {
		return _value;
	}

	// Not working :(
	//friend inline auto value_ordering_helper(const Key &key) {
	//	return key.value;
	//}

private:
	base::optional_variant<not_null<History*>, not_null<Data::Feed*>> _value;

};

struct RowDescriptor {
	RowDescriptor() = default;
	RowDescriptor(Key key, FullMsgId fullId) : key(key), fullId(fullId) {
	}

	Key key;
	FullMsgId fullId;

};

inline bool operator==(const RowDescriptor &a, const RowDescriptor &b) {
	return (a.key == b.key)
		&& ((a.fullId == b.fullId) || (!a.fullId.msg && !b.fullId.msg));
}

inline bool operator!=(const RowDescriptor &a, const RowDescriptor &b) {
	return !(a == b);
}

inline bool operator<(const RowDescriptor &a, const RowDescriptor &b) {
	if (a.key < b.key) {
		return true;
	} else if (a.key > b.key) {
		return false;
	}
	return a.fullId < b.fullId;
}

inline bool operator>(const RowDescriptor &a, const RowDescriptor &b) {
	return (b < a);
}

inline bool operator<=(const RowDescriptor &a, const RowDescriptor &b) {
	return !(b < a);
}

inline bool operator>=(const RowDescriptor &a, const RowDescriptor &b) {
	return !(a < b);
}

} // namespace Dialogs
