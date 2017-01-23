/**
* @file pvtl.h
* @brief Polarity Viewer Template Library
* 
* Improvement and expansion upon the STL (Standard Template Library)
*
* $LicenseInfo:firstyear=2014&license=viewerlgpl$
* Polarity Viewer Source Code
* Copyright (C) 2017 Xenhat Liamano
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* The Polarity Viewer Project
* http://www.polarityviewer.org
* $/LicenseInfo$
*/

#pragma once
#if !LL_PVTL_H
#define LL_PVTL_H 1

// re-usable algorithm by Sebastian Mach (http://stackoverflow.com/a/6693128/1570096)
// Follow the signature of std::getline. Allows us to stay completely
// type agnostic.
template <typename Stream, typename Iter, typename Infix>
inline Stream& infix(Stream &os, Iter from, Iter to, Infix infix_) {
	if (from == to) return os;
	os << *from;
	for (++from; from != to; ++from) {
		os << infix_ << *from;
	}
	return os;
}

template <typename Stream, typename Iter>
inline Stream& vector_to_string(Stream &os, Iter from, Iter to) {
	return infix(os, from, to, ", ");
}

inline long version_string_as_long(const std::string& version_in)
{
	// do not modify the incoming string as the rest of the program may
	// expect it to be intact.
	std::string string_copy = version_in;
	string_copy.erase(std::remove(string_copy.begin(), string_copy.end(), '.'), string_copy.end());
	long version_int = strtoul(string_copy.c_str(), nullptr, 0);
	llassert(version_int != 0);
	return version_int;
}

#endif // LL_PVTL_H
