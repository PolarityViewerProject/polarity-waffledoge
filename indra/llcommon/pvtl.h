/**
* @file pvtl.h
* @brief Polarity Viewer Template Library
* 
* Improvement and expansion upon the STL (Standard Template Library)
*
* $LicenseInfo:firstyear=2015&license=viewerlgpl$
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
