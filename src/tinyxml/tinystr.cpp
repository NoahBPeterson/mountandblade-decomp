/*
www.sourceforge.net/projects/tinyxml
Original file by Yves Berquin.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/*
 * THIS FILE WAS ALTERED BY Tyge Løvset, 7. April 2005.
 */

// TinyXML 2.4.x tinystr.cpp as linked into mount&blade.exe: 0x42d820..0x42d99f.
// The operator+ overloads are not linked (dropped by /OPT:REF) and are left out.

#define NDEBUG     // project setting (Release): the asserts of tinystr.h compile to nothing
#ifndef TIXML_USE_STL

#include <new>     // declares operator new[]/delete[]: without it cl 13.10 calls the scalar operators for the int[] reps (the original calls 0x535482/0x53570d)
#include "./tinystr.h"

// Error value for find primitive
const TiXmlString::size_type TiXmlString::npos = static_cast< size_type >(-1);   // 0x54b8f4

// Null rep.
TiXmlString::Rep TiXmlString::nullrep_ = { 0, 0, '\0' };                        // 0x5e23d0

// 0x42d820: TiXmlString::quit() (inline in tinystr.h), emitted out of line in this object. ~TiXmlString has the
// same code and is folded onto it (/OPT:ICF).

// @ 0x42d840
void TiXmlString::reserve (size_type cap)
{
	if (cap > capacity())
	{
		TiXmlString tmp;
		tmp.init(length(), cap);
		memcpy(tmp.start(), data(), length());
		swap(tmp);
	}
}


// @ 0x42d8b0
TiXmlString& TiXmlString::assign(const char* str, size_type len)
{
	size_type cap = capacity();
	if (len > cap || cap > 3*(len + 8))
	{
		TiXmlString tmp;
		tmp.init(len);
		memcpy(tmp.start(), str, len);
		swap(tmp);
	}
	else
	{
		memmove(start(), str, len);
		set_size(len);
	}
	return *this;
}


// @ 0x42d950
TiXmlString& TiXmlString::append(const char* str, size_type len)
{
	size_type newsize = length() + len;
	if (newsize > capacity())
	{
		reserve (newsize + capacity());
	}
	memmove(finish(), str, len);
	set_size(newsize);
	return *this;
}

#endif	// TIXML_USE_STL
