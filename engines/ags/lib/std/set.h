/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AGS_STD_SET_H
#define AGS_STD_SET_H

#include "common/array.h"

namespace AGS3 {
namespace std {

/**
 * Derives the ScummVM SortedArray to match the std::set class
 */
template<class T, class Comparitor = Common::Less<T> >
class set : public Common::SortedArray<T, const T &> {
private:
	static int ComparatorFn(const T &a, const T &b) {
		return Comparitor().operator()(a, b) ? -1 : 0;
	}

	static bool CompareEq(const T &a, const T &b) {
		return !ComparatorFn(a, b) && !ComparatorFn(b, a);
	}

public:
	struct Entry {
		const T &_value;
		Entry(const T &item) : _value(item) {
		}
	};
public:
	using iterator = typename Common::SortedArray<T, const T &>::iterator;
	using const_iterator = typename Common::SortedArray<T, const T &>::const_iterator;

	/**
	 * Constructor
	 */
	set() : Common::SortedArray<T, const T & >(ComparatorFn) {}

	/**
	 * Locate an item in the set
	 */
	iterator find(const T &item) {
		iterator it;
		for (it = this->begin(); it != this->end() && !CompareEq(*it, item); ++it) {
		}

		return it;
	}

	/**
	 * Insert an element at the sorted position.
	 */
	Entry insert(const T &item) {
		Common::SortedArray<T, const T &>::insert(item);
		return Entry(item);
	}

	/**
	 * Removes the element at the given iterator
	 */
	void erase(iterator item) {
		Common::SortedArray<T, const T &>::erase(item);
	}

	/**
	 * Removes the elements at the specified range
	 */
	void erase(iterator first, iterator last) {
		Common::SortedArray<T, const T &>::erase(first, last);
	}

	/**
	 * Removes the elements equal to the given item.
	 * Returns the number of elements removed
	 */
	size_t erase(const T &item) {
		iterator first = find(item);
		if (first == this->end())
			return 0;
		iterator end = first + 1;
		while (end != this->end() && CompareEq(*first, *end)) {
			++end;
		}
		size_t erased = Common::distance(first, end);
		this->erase(first, end);
		return erased;
	}

	/**
	 * Returns the number of keys that match the specified key
	 */
	size_t count(const T item) const {
		size_t total = 0;
		for (const_iterator it = this->begin(); it != this->end(); ++it) {
			if (CompareEq(*it, item))
				++total;
			else if (!ComparatorFn(item, *it))
				// Passed beyond possibility of matches
				break;
		}

		return total;
	}
};

} // namespace std
} // namespace AGS3

#endif
