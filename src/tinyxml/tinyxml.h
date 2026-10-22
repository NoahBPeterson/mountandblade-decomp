/*
www.sourceforge.net/projects/tinyxml
Original code (2.0 and earlier )copyright (c) 2000-2006 Lee Thomason (www.grinninglizard.com)

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

// TinyXML 2.4.x (the last series with TiXmlOutStream / StreamOut; 2.5 replaced them with
// TiXmlPrinter / Accept) as linked into mount&blade.exe, non-STL build (TIXML_USE_STL undefined,
// TIXML_SAFE defined). Code: tinystr.cpp 0x42d820, tinyxml.cpp 0x42d9a0, tinyxmlparser.cpp
// 0x42f720..0x431400. Address comments name the binary's copy of a function or global; an inline
// member with an address is an out-of-line copy cl emitted where it did not inline it.
// Only the members the linked code uses or that fix a layout/vtable slot are declared; the rest of
// the public API (not linked: /OPT:REF dropped it) is left out.

#ifndef TINYXML_INCLUDED
#define TINYXML_INCLUDED

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4530 )
#pragma warning( disable : 4786 )
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Help out windows:
#if defined( _DEBUG ) && !defined( DEBUG )
#define DEBUG
#endif

#ifdef TIXML_USE_STL
	#include <string>
 	#include <iostream>
	#define TIXML_STRING	std::string
	#define TIXML_ISTREAM	std::istream
	#define TIXML_OSTREAM	std::ostream
#else
	#include "./tinystr.h"
	#define TIXML_STRING	TiXmlString
	#define TIXML_OSTREAM	TiXmlOutStream
#endif

// Deprecated library function hell. Compilers want to use the
// new safe versions. This probably doesn't fully address the problem,
// but it gets closer. There are too many compilers for me to fully
// test. If you get compilation troubles, undefine TIXML_SAFE

#define TIXML_SAFE		// TinyXml isn't fully buffer overrun protected, safe code. This is work in progress.
#ifdef TIXML_SAFE
	#if defined(_MSC_VER) && (_MSC_VER >= 1200 )
		// Microsoft visual studio, version 6 and higher.
		//#pragma message( "Using _sn* functions." )
		#define TIXML_SNPRINTF _snprintf
		#define TIXML_SNSCANF  _snscanf
	#elif defined(__GNUC__) && (__GNUC__ >= 3 )
		// GCC version 3 and higher.s
		//#warning( "Using sn* functions." )
		#define TIXML_SNPRINTF snprintf
		#define TIXML_SNSCANF  snscanf
	#endif
#endif

class TiXmlDocument;
class TiXmlElement;
class TiXmlComment;
class TiXmlUnknown;
class TiXmlAttribute;
class TiXmlText;
class TiXmlDeclaration;
class TiXmlParsingData;

const int TIXML_MAJOR_VERSION = 2;
const int TIXML_MINOR_VERSION = 4;

/*	Internal structure for tracking location of items
	in the XML file.
*/
struct TiXmlCursor
{
	TiXmlCursor()		{ Clear(); }
	void Clear()		{ row = col = -1; }

	int row;	// 0 based.
	int col;	// 0 based.
};


// Only used by Attribute::Query functions
enum
{
	TIXML_SUCCESS,
	TIXML_NO_ATTRIBUTE,
	TIXML_WRONG_TYPE
};


// Used by the parsing routines.
enum TiXmlEncoding
{
	TIXML_ENCODING_UNKNOWN,
	TIXML_ENCODING_UTF8,
	TIXML_ENCODING_LEGACY
};

const TiXmlEncoding TIXML_DEFAULT_ENCODING = TIXML_ENCODING_UNKNOWN;

/** TiXmlBase is a base class for every class in TinyXml.
	It does little except to establish that TinyXml classes
	can be printed and provide some utility functions.
	Vtable 0x54b8f8: ~TiXmlBase, Print, Parse, StreamOut.
*/
class TiXmlBase
{
	friend class TiXmlNode;
	friend class TiXmlElement;
	friend class TiXmlDocument;

public:
	TiXmlBase()	:	userData(0) {}
	virtual ~TiXmlBase()					{}                      // deleting dtor 0x42da80

	/**	All TinyXml classes can print themselves to a filestream.
	*/
	virtual void Print( FILE* cfile, int depth ) const = 0;

	static void SetCondenseWhiteSpace( bool condense )		{ condenseWhiteSpace = condense; }
	static bool IsWhiteSpaceCondensed()						{ return condenseWhiteSpace; }

	int Row() const			{ return location.row + 1; }
	int Column() const		{ return location.col + 1; }	///< See Row()

	void  SetUserData( void* user )			{ userData = user; }
	void* GetUserData()						{ return userData; }

	// Table that returs, for a given lead byte, the total number of bytes
	// in the UTF-8 sequence.
	static const int utf8ByteTable[256];                            // 0x54bd88

	virtual const char* Parse(	const char* p,
								TiXmlParsingData* data,
								TiXmlEncoding encoding /*= TIXML_ENCODING_UNKNOWN */ ) = 0;

	enum
	{
		TIXML_NO_ERROR = 0,
		TIXML_ERROR,
		TIXML_ERROR_OPENING_FILE,
		TIXML_ERROR_OUT_OF_MEMORY,
		TIXML_ERROR_PARSING_ELEMENT,
		TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME,
		TIXML_ERROR_READING_ELEMENT_VALUE,
		TIXML_ERROR_READING_ATTRIBUTES,
		TIXML_ERROR_PARSING_EMPTY,
		TIXML_ERROR_READING_END_TAG,
		TIXML_ERROR_PARSING_UNKNOWN,
		TIXML_ERROR_PARSING_COMMENT,
		TIXML_ERROR_PARSING_DECLARATION,
		TIXML_ERROR_DOCUMENT_EMPTY,
		TIXML_ERROR_EMBEDDED_NULL,
		TIXML_ERROR_PARSING_CDATA,

		TIXML_ERROR_STRING_COUNT
	};

protected:

	// See STL_STRING_BUG
	// Utility class to overcome a bug.
	class StringToBuffer
	{
	  public:
		StringToBuffer( const TIXML_STRING& str );
		~StringToBuffer();
		char* buffer;
	};

	static const char*	SkipWhiteSpace( const char*, TiXmlEncoding encoding );   // 0x42fa70
	inline static bool	IsWhiteSpace( char c )                     // 0x42fa40 (out-of-line copy, tinyxmlparser.obj)
	{
		return ( isspace( (unsigned char) c ) || c == '\n' || c == '\r' );
	}
	inline static bool	IsWhiteSpace( int c )
	{
		if ( c < 256 )
			return IsWhiteSpace( (char) c );
		return false;	// Again, only truly correct for English/Latin...but usually works.
	}

	virtual void StreamOut (TIXML_OSTREAM *) const = 0;

	/*	Reads an XML name into the string provided. Returns
		a pointer just past the last character of the name,
		or 0 if the function has an error.
	*/
	static const char* ReadName( const char* p, TIXML_STRING* name, TiXmlEncoding encoding );   // 0x42fb30

	/*	Reads text. Returns a pointer past the given end tag.
		Wickedly complex options, but it keeps the (sensitive) code in one place.
	*/
	static const char* ReadText(	const char* in,				// where to start
									TIXML_STRING* text,			// the string read
									bool ignoreWhiteSpace,		// whether to keep the white space
									const char* endTag,			// what ends this text
									bool ignoreCase,			// whether to ignore case in the end tag
									TiXmlEncoding encoding );	// the current encoding   // 0x4303f0

	// If an entity has been found, transform it into a character.
	static const char* GetEntity( const char* in, char* value, int* length, TiXmlEncoding encoding );   // 0x42fe00

	// Get a character, while interpreting entities.
	// The length can be from 0 to 4 bytes.
	inline static const char* GetChar( const char* p, char* _value, int* length, TiXmlEncoding encoding )   // 0x430360 (out-of-line copy)
	{
		assert( p );
		if ( encoding == TIXML_ENCODING_UTF8 )
		{
			*length = utf8ByteTable[ *((unsigned char*)p) ];
			assert( *length >= 0 && *length < 5 );
		}
		else
		{
			*length = 1;
		}

		if ( *length == 1 )
		{
			if ( *p == '&' )
				return GetEntity( p, _value, length, encoding );
			*_value = *p;
			return p+1;
		}
		else if ( *length )
		{
			//strncpy( _value, p, *length );	// lots of compilers don't like this function (unsafe),
												// and the null terminator isn't needed
			for( int i=0; p[i] && i<*length; ++i ) {
				_value[i] = p[i];
			}
			return p + (*length);
		}
		else
		{
			// Not valid text.
			return 0;
		}
	}

	// Puts a string to a stream, expanding entities as it goes.
	// Note this should not contian the '<', '>', etc, or they will be transformed into entities!
	static void PutString( const TIXML_STRING& str, TIXML_OSTREAM* out );

	static void PutString( const TIXML_STRING& str, TIXML_STRING* out );   // 0x42dad0

	// Return true if the next characters in the stream are any of the endTag sequences.
	// Ignore case only works for english, and should only be relied on when comparing
	// to English words: StringEqual( p, "version", true ) is fine.
	static bool StringEqual(	const char* p,
								const char* endTag,
								bool ignoreCase,
								TiXmlEncoding encoding );       // 0x42fbc0

	static const char* errorString[ TIXML_ERROR_STRING_COUNT ];  // 0x5a9070 (tinyxmlerror.cpp: data only)

	TiXmlCursor location;

    /// Field containing a generic user pointer
	void*			userData;

	// None of these methods are reliable for any language except English.
	// Good for approximation, not great for accuracy.
	static int IsAlpha( unsigned char anyByte, TiXmlEncoding encoding );      // 0x42f820
	static int IsAlphaNum( unsigned char anyByte, TiXmlEncoding encoding );
	inline static int ToLower( int v, TiXmlEncoding encoding )
	{
		if ( encoding == TIXML_ENCODING_UTF8 )
		{
			if ( v < 128 ) return tolower( v );
			return v;
		}
		else
		{
			return tolower( v );
		}
	}
	static void ConvertUTF32ToUTF8( unsigned long input, char* output, int* length );   // 0x42f720

private:
	TiXmlBase( const TiXmlBase& );				// not implemented.
	void operator=( const TiXmlBase& base );	// not allowed.

	struct Entity
	{
		const char*     str;
		unsigned int	strLength;
		char		    chr;
	};
	enum
	{
		NUM_ENTITY = 5,
		MAX_ENTITY_LENGTH = 6

	};
	static Entity entity[ NUM_ENTITY ];                             // 0x5a90b0
	static bool condenseWhiteSpace;                                 // 0x5a9068
};


/** The parent class for everything in the Document Object Model.
	(Except for attributes).
	Vtable 0x54b988 (17 slots): ~TiXmlNode, Print, Parse, StreamOut, the 12 ToXxx casts, Clone.
*/
class TiXmlNode : public TiXmlBase
{
	friend class TiXmlDocument;
	friend class TiXmlElement;

public:
	/** The types of XML nodes supported by TinyXml. (All the
			unsupported types are picked up by UNKNOWN.)
	*/
	enum NodeType
	{
		DOCUMENT,
		ELEMENT,
		COMMENT,
		UNKNOWN,
		TEXT,
		DECLARATION,
		TYPECOUNT
	};

	virtual ~TiXmlNode();                                           // 0x42e170 (deleting dtor 0x42f010)

	/** The meaning of 'value' changes for the specific type of
		TiXmlNode.
	*/
	const char *Value() const { return value.c_str (); }

	/** Changes the value of the node. Defined as:
		@verbatim
		Document:	filename of the xml file
		Element:	name of the element
		Comment:	the comment text
		Unknown:	the tag contents
		Text:		the text string
		@endverbatim
	*/
	void SetValue(const char * _value) { value = _value;}

	/// Delete all the children of this node. Does not affect 'this'.
	void Clear();

	/// One step up the DOM.
	TiXmlNode* Parent()							{ return parent; }
	const TiXmlNode* Parent() const				{ return parent; }

	const TiXmlNode* FirstChild()	const	{ return firstChild; }		///< The first child of this node. Will be null if there are no children.
	TiXmlNode* FirstChild()					{ return firstChild; }
	const TiXmlNode* FirstChild( const char * value ) const;			///< The first child of this node with the matching 'value'. Will be null if none found.
	TiXmlNode* FirstChild( const char * value );						///< The first child of this node with the matching 'value'. Will be null if none found.   // 0x42dc10

	const TiXmlNode* LastChild() const	{ return lastChild; }		/// The last child of this node. Will be null if there are no children.
	TiXmlNode* LastChild()	{ return lastChild; }

	/** Add a new node related to this. Adds a child past the LastChild.

		NOTE: the node to be added is passed by pointer, and will be
		henceforth owned (and deleted) by tinyXml. This method is efficient
		and avoids an extra copy, but should be used with care as it
		uses a different memory model than the other insert functions.

		@sa InsertEndChild
	*/
	TiXmlNode* LinkEndChild( TiXmlNode* addThis );                  // 0x42d9a0

	/// Navigate to a sibling node.
	const TiXmlNode* PreviousSibling() const			{ return prev; }
	TiXmlNode* PreviousSibling()						{ return prev; }

	/// Navigate to a sibling node.
	const TiXmlNode* NextSibling() const				{ return next; }
	TiXmlNode* NextSibling()							{ return next; }

	/** Convenience function to get through elements.
		Calls NextSibling and ToElement. Will skip all non-Element
		nodes. Returns 0 if there is not another element.
	*/
	const TiXmlElement* NextSiblingElement() const;
	TiXmlElement* NextSiblingElement();                             // 0x42dc80

	/// Convenience function to get through elements.
	const TiXmlElement* FirstChildElement()	const;
	TiXmlElement* FirstChildElement();

	/** Query the type (as an enumerated value, above) of this node.
		The possible types are: DOCUMENT, ELEMENT, COMMENT,
								UNKNOWN, TEXT, and DECLARATION.
	*/
	int Type() const	{ return type; }

	/** Return a pointer to the Document this node lives in.
		Returns null if not in a document.
	*/
	const TiXmlDocument* GetDocument() const;
	TiXmlDocument* GetDocument();                                   // 0x42d9d0

	/// Returns true if this node has no children.
	bool NoChildren() const						{ return !firstChild; }

	virtual const TiXmlDocument*    ToDocument()    const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlElement*     ToElement()     const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlComment*     ToComment()     const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlUnknown*     ToUnknown()     const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlText*        ToText()        const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual const TiXmlDeclaration* ToDeclaration() const { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.

	virtual TiXmlDocument*          ToDocument()    { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlElement*           ToElement()	    { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlComment*           ToComment()     { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlUnknown*           ToUnknown()	    { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlText*	            ToText()        { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.
	virtual TiXmlDeclaration*       ToDeclaration() { return 0; } ///< Cast to a more defined type. Will return null if not of the requested type.

	/** Create an exact duplicate of this node and return it. The memory must be deleted
		by the caller.
	*/
	virtual TiXmlNode* Clone() const = 0;

protected:
	TiXmlNode( NodeType _type );                                    // 0x42e130

	// Copy to the allocated object. Shared functionality between Clone, Copy constructor,
	// and the assignment operator.
	void CopyTo( TiXmlNode* target ) const;

	// Figure out what is at *p, and parse it. Returns null if it is not an xml node.
	TiXmlNode* Identify( const char* start, TiXmlEncoding encoding );   // 0x42ffc0

	TiXmlNode*		parent;
	NodeType		type;

	TiXmlNode*		firstChild;
	TiXmlNode*		lastChild;

	TIXML_STRING	value;

	TiXmlNode*		prev;
	TiXmlNode*		next;

private:
	TiXmlNode( const TiXmlNode& );				// not implemented.
	void operator=( const TiXmlNode& base );	// not allowed.
};


/** An attribute is a name-value pair. Elements have an arbitrary
	number of attributes, each with a unique name.

	@note The attributes are not TiXmlNodes, since they are not
		  part of the tinyXML document object model. There are other
		  suggested ways to look at this problem.
	Vtable 0x54b974.
*/
class TiXmlAttribute : public TiXmlBase
{
	friend class TiXmlAttributeSet;

public:
	/// Construct an empty attribute.
	TiXmlAttribute() : TiXmlBase()
	{
		document = 0;
		prev = next = 0;
	}

	/// Construct an attribute with a name and value.
	TiXmlAttribute( const char * _name, const char * _value )       // 0x42e070 (out-of-line copy)
	{
		name = _name;
		value = _value;
		document = 0;
		prev = next = 0;
	}
	// ~TiXmlAttribute is implicit: 0x42e0f0 (deleting dtor 0x42ed70)

	const char*		Name()  const		{ return name.c_str (); }		///< Return the name of this attribute.
	const char*		Value() const		{ return value.c_str (); }		///< Return the value of this attribute.
	const TIXML_STRING& NameTStr() const { return name; }

	/** QueryIntValue examines the value string. It is an alternative to the
		IntValue() method with richer error checking.
		If the value is an integer, it is stored in 'value' and
		the call returns TIXML_SUCCESS. If it is not
		an integer, it returns TIXML_WRONG_TYPE.

		A specialized but useful call. Note that for success it returns 0,
		which is the opposite of almost all other TinyXml calls.
	*/
	int QueryIntValue( int* _value ) const;
	/// QueryDoubleValue examines the value string. See QueryIntValue().
	int QueryDoubleValue( double* _value ) const;

	void SetName( const char* _name )	{ name = _name; }				///< Set the name of this attribute.
	void SetValue( const char* _value )	{ value = _value; }				///< Set the value.   // 0x42daa0 (out-of-line copy)

	/// Get the next sibling attribute in the DOM. Returns null at end.
	const TiXmlAttribute* Next() const;
	TiXmlAttribute* Next() {
		return const_cast< TiXmlAttribute* >( (const_cast< const TiXmlAttribute* >(this))->Next() );
	}

	/*	Attribute parsing starts: first letter of the name
						 returns: the next char after the value end quote
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );   // 0x430d30

	// Prints this Attribute to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;             // 0x42e6f0

	virtual void StreamOut( TIXML_OSTREAM * out ) const;            // 0x42e7c0
	// [internal use]
	// Set the document pointer so the attribute can report errors.
	void SetDocument( TiXmlDocument* doc )	{ document = doc; }

private:
	TiXmlAttribute( const TiXmlAttribute& );				// not implemented.
	void operator=( const TiXmlAttribute& base );	// not allowed.

	TiXmlDocument*	document;	// A pointer back to a document, for error reporting.
	TIXML_STRING name;
	TIXML_STRING value;
	TiXmlAttribute*	prev;
	TiXmlAttribute*	next;
};


/*	A class used to manage a group of attributes.
	It is only used internally, both by the ELEMENT and the DECLARATION.

	The set can be changed transparent to the Element and Declaration
	classes that use it, but NOT transparent to the Attribute
	which has to implement a next() and previous() method. Which makes
	it a bit problematic and prevents the use of STL.

	This version is implemented with circular lists because:
		- I like circular lists
		- it demonstrates some independence from the (typical) doubly linked list.
*/
class TiXmlAttributeSet
{
public:
	TiXmlAttributeSet();
	~TiXmlAttributeSet();

	void Add( TiXmlAttribute* attribute );                          // 0x42da00
	void Remove( TiXmlAttribute* attribute );

	const TiXmlAttribute* First()	const	{ return ( sentinel.next == &sentinel ) ? 0 : sentinel.next; }
	TiXmlAttribute* First()					{ return ( sentinel.next == &sentinel ) ? 0 : sentinel.next; }
	const TiXmlAttribute* Last() const		{ return ( sentinel.prev == &sentinel ) ? 0 : sentinel.prev; }
	TiXmlAttribute* Last()					{ return ( sentinel.prev == &sentinel ) ? 0 : sentinel.prev; }

	TiXmlAttribute*	Find( const TIXML_STRING& name ) const;             // 0x42df40

private:
	//*ME:	Because of hidden/disabled copy-construktor in TiXmlAttribute (sentinel-element),
	//*ME:	this class must be also use a hidden/disabled copy-constructor !!!
	TiXmlAttributeSet( const TiXmlAttributeSet& );	// not allowed
	void operator=( const TiXmlAttributeSet& );	// not allowed (as TiXmlAttribute)

	TiXmlAttribute sentinel;
};


/** The element is a container class. It has a value, the element name,
	and can contain other elements, text, comments, and unknowns.
	Elements also contain an arbitrary number of attributes.
	Vtable 0x54bbb0.
*/
class TiXmlElement : public TiXmlNode
{
public:
	/// Construct an element.
	TiXmlElement (const char * in_value);                           // 0x42f030

	virtual ~TiXmlElement();                                        // deleting dtor 0x42f660

	/** Given an attribute name, Attribute() returns the value
		for the attribute of that name, or null if none exists.
	*/
	const char* Attribute( const char* name ) const;                // 0x42f0b0

	/** QueryIntAttribute examines the attribute - it is an alternative to the
		Attribute() method with richer error checking.
		If the attribute is an integer, it is stored in 'value' and
		the call returns TIXML_SUCCESS. If it is not
		an integer, it returns TIXML_WRONG_TYPE. If the attribute
		does not exist, then TIXML_NO_ATTRIBUTE is returned.
	*/
	int QueryIntAttribute( const char* name, int* _value ) const;   // 0x42f110
	/// QueryDoubleAttribute examines the attribute - see QueryIntAttribute().
	int QueryDoubleAttribute( const char* name, double* _value ) const;   // 0x42f190

	/** Sets an attribute of name to a given value. The attribute
		will be created if it does not exist, or changed if it does.
	*/
	void SetAttribute( const char* name, const char * _value );     // 0x42f210

	const TiXmlAttribute* FirstAttribute() const	{ return attributeSet.First(); }		///< Access the first attribute in this element.
	TiXmlAttribute* FirstAttribute() 				{ return attributeSet.First(); }

	/// Creates a new Element and returns it - the returned element is a copy.
	virtual TiXmlNode* Clone() const;                               // 0x42f3b0
	// Print the Element to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;             // 0x42e1b0

	/*	Attribtue parsing starts: next char past '<'
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );   // 0x430860

	virtual const TiXmlElement*     ToElement()     const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlElement*           ToElement()	          { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

protected:

	void CopyTo( TiXmlElement* target ) const;                      // 0x42f300
	void ClearThis();	// like clear, but initializes 'this' object as well   // 0x42dcb0

	// Used to be public [internal use]
	virtual void StreamOut( TIXML_OSTREAM * out ) const;            // 0x42e300

	/*	[internal use]
		Reads the "value" of the element -- another element, or text.
		This should terminate with the current end tag.
	*/
	const char* ReadValue( const char* in, TiXmlParsingData* prevData, TiXmlEncoding encoding );   // 0x430190

private:

	TiXmlAttributeSet attributeSet;
};


/**	An XML comment.
	Vtable 0x54ba48.
*/
class TiXmlComment : public TiXmlNode
{
public:
	/// Constructs an empty comment.
	TiXmlComment() : TiXmlNode( TiXmlNode::COMMENT ) {}             // 0x42edc0 (out-of-line copy)
	virtual ~TiXmlComment()	{}                                      // deleting dtor 0x42ee00

	/// Returns a copy of this Comment.
	virtual TiXmlNode* Clone() const;                               // 0x42f470
	/// Write this Comment to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;             // 0x42de40

	/*	Attribtue parsing starts: at the ! of the !--
						 returns: next char past '>'
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );   // 0x430c60

	virtual const TiXmlComment*  ToComment() const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlComment*  ToComment() { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

protected:
	void CopyTo( TiXmlComment* target ) const;

	// used to be public
	virtual void StreamOut( TIXML_OSTREAM * out ) const;            // 0x42e960

private:

};


/** XML text. A text node can have 2 ways to output the next. "normal" output
	and CDATA. It will default to the mode it was parsed from the XML file and
	you generally want to leave it alone, but you can change the output mode with
	SetCDATA() and query it with CDATA().
	Vtable 0x54bb20.
*/
class TiXmlText : public TiXmlNode
{
	friend class TiXmlElement;
public:
	/** Constructor for text element. By default, it is treated as
		normal, encoded text. If you want it be output as a CDATA text
		element, set the parameter _cdata to 'true'
	*/
	TiXmlText (const char * initValue ) : TiXmlNode (TiXmlNode::TEXT)   // 0x42ee30 (out-of-line copy)
	{
		SetValue( initValue );
		cdata = false;
	}
	virtual ~TiXmlText() {}                                         // deleting dtor 0x42ee90

	// Write this text object to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;             // 0x42e9a0

	/// Queries whether this represents text using a CDATA section.
	bool CDATA()					{ return cdata; }
	/// Turns on or off a CDATA representation of text.
	void SetCDATA( bool _cdata )	{ cdata = _cdata; }

	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );   // 0x430eb0

	virtual const TiXmlText* ToText() const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlText*       ToText()       { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

protected :
	///  [internal use] Creates a new Element and returns it.
	virtual TiXmlNode* Clone() const;                               // 0x42f500
	void CopyTo( TiXmlText* target ) const;

	virtual void StreamOut ( TIXML_OSTREAM * out ) const;           // 0x42ea60
	bool Blank() const;	// returns true if all white space and new lines   // 0x42fdb0

private:
	bool cdata;			// true if this should be input and output as a CDATA style text element
};


/** In correct XML the declaration is the first entry in the file.
	@verbatim
		<?xml version="1.0" standalone="yes"?>
	@endverbatim

	TinyXml will happily read or write files without a declaration,
	however. There are 3 possible attributes to the declaration:
	version, encoding, and standalone.

	Note: In this version of the code, the attributes are
	handled as special cases, not generic attributes, simply
	because there can only be at most 3 and they are always the same.
	Vtable 0x54bab0.
*/
class TiXmlDeclaration : public TiXmlNode
{
public:
	/// Construct an empty declaration.
	TiXmlDeclaration()   : TiXmlNode( TiXmlNode::DECLARATION ) {}   // 0x42eec0 (out-of-line copy)

	virtual ~TiXmlDeclaration()	{}                                  // 0x42ef20 (deleting dtor 0x42ef00)

	/// Version. Will return an empty string if none was found.
	const char *Version() const			{ return version.c_str (); }
	/// Encoding. Will return an empty string if none was found.
	const char *Encoding() const		{ return encoding.c_str (); }
	/// Is this a standalone document?
	const char *Standalone() const		{ return standalone.c_str (); }

	/// Creates a copy of this Declaration and returns it.
	virtual TiXmlNode* Clone() const;                               // 0x42f560
	/// Print this declaration to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;             // 0x42de80

	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );   // 0x431050

	virtual const TiXmlDeclaration* ToDeclaration() const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlDeclaration*       ToDeclaration()       { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

protected:
	void CopyTo( TiXmlDeclaration* target ) const;                  // 0x42ec50
	// used to be public
	virtual void StreamOut ( TIXML_OSTREAM * out) const;            // 0x42eaf0

private:

	TIXML_STRING version;
	TIXML_STRING encoding;
	TIXML_STRING standalone;
};


/** Any tag that tinyXml doesn't recognize is saved as an
	unknown. It is a tag of text, but should not be modified.
	It will be written back to the XML, unchanged, when the file
	is saved.

	DTD tags get thrown into TiXmlUnknowns.
	Vtable 0x54bb68.
*/
class TiXmlUnknown : public TiXmlNode
{
public:
	TiXmlUnknown() : TiXmlNode( TiXmlNode::UNKNOWN )	{}          // 0x42ef70 (out-of-line copy)
	virtual ~TiXmlUnknown() {}                                      // deleting dtor 0x42efb0

	/// Creates a copy of this Unknown and returns it.
	virtual TiXmlNode* Clone() const;                               // 0x42f5d0
	/// Print this Unknown to a FILE stream.
	virtual void Print( FILE* cfile, int depth ) const;             // 0x42df00

	virtual const char* Parse( const char* p, TiXmlParsingData* data, TiXmlEncoding encoding );   // 0x42fce0

	virtual const TiXmlUnknown*     ToUnknown()     const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlUnknown*           ToUnknown()	    { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

protected:
	void CopyTo( TiXmlUnknown* target ) const;

	virtual void StreamOut ( TIXML_OSTREAM * out ) const;           // 0x42ecc0

private:

};


/** Always the top level node. A document binds together all the
	XML pieces. It can be saved, loaded, and printed to the screen.
	The 'value' of a document node is the xml file name.
	Vtable 0x54b9e8.
*/
class TiXmlDocument : public TiXmlNode
{
public:
	/// Create an empty document, that has no name.
	TiXmlDocument();                                                // 0x42e3e0
	/// Create a document with a name. The name of the document is also the filename of the xml.
	TiXmlDocument( const char * documentName );                     // 0x42e450

	virtual ~TiXmlDocument() {}                                     // deleting dtor 0x42efe0

	/** Load a file using the current document value.
		Returns true if successful. Will delete any existing
		document data before loading.
	*/
	bool LoadFile( TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );   // 0x42f6c0
	/// Load a file using the given filename. Returns true if successful.
	bool LoadFile( const char * filename, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );   // 0x42f3f0
	/// Load a file using the given FILE*. Returns true if successful.
	bool LoadFile( FILE*, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );   // 0x42e4f0

	/** Parse the given null terminated block of xml data. Passing in an encoding to this
		method (either TIXML_ENCODING_LEGACY or TIXML_ENCODING_UTF8 will force TinyXml
		to use that encoding, regardless of what TinyXml might otherwise try to detect.
	*/
	virtual const char* Parse( const char* p, TiXmlParsingData* data = 0, TiXmlEncoding encoding = TIXML_DEFAULT_ENCODING );   // 0x4305b0

	/** If an error occurs, Error will be set to true. Also,
		- The ErrorId() will contain the integer identifier of the error (not generally useful)
		- The ErrorDesc() method will return the name of the error. (very useful)
		- The ErrorRow() and ErrorCol() will return the location of the error (if known)
	*/
	bool Error() const						{ return error; }

	/// Contains a textual (english) description of the error if one occurs.
	const char * ErrorDesc() const	{ return errorDesc.c_str (); }

	/** Generally, you probably want the error string ( ErrorDesc() ). But if you
		prefer the ErrorId, this function will fetch it.
	*/
	int ErrorId()	const				{ return errorId; }

	/** Returns the location (if known) of the error. The first column is column 1,
		and the first row is row 1. A value of 0 means the row and column wasn't applicable
		(memory errors, for example, have no row/column) or the parser lost the error. (An
		error in the error reporting, in that case.)

		@sa SetTabSize, Row, Column
	*/
	int ErrorRow()	{ return errorLocation.row+1; }
	int ErrorCol()	{ return errorLocation.col+1; }	///< The column where the error occured. See ErrorRow()

	/// The tab size, see SetTabSize().
	int TabSize() const	{ return tabsize; }

	/** If you have handled the error, it can be reset with this call. The error
		state is automatically cleared if you Parse a new XML block.
	*/
	void ClearError()						{	error = false;
												errorId = 0;
												errorDesc = "";
												errorLocation.row = errorLocation.col = 0;
												//errorLocation.last = 0;
											}

	/** Dump the document to standard out. */
	void Print() const						{ Print( stdout, 0 ); }

	/// Print this Document to a FILE stream.
	virtual void Print( FILE* cfile, int depth = 0 ) const;         // 0x42ddd0
	// [internal use]
	void SetError( int err, const char* errorLocation, TiXmlParsingData* prevData, TiXmlEncoding encoding );   // 0x42fc70

	virtual const TiXmlDocument*    ToDocument()    const { return this; } ///< Cast to a more defined type. Will return null not of the requested type.
	virtual TiXmlDocument*          ToDocument()          { return this; } ///< Cast to a more defined type. Will return null not of the requested type.

protected :
	virtual void StreamOut ( TIXML_OSTREAM * out) const;            // 0x42de10
	// [internal use]
	virtual TiXmlNode* Clone() const;                               // 0x42e6b0

private:
	void CopyTo( TiXmlDocument* target ) const;                     // 0x42dd30

	bool error;
	int  errorId;
	TIXML_STRING errorDesc;
	int tabsize;
	TiXmlCursor errorLocation;
	bool useMicrosoftBOM;		// the UTF-8 BOM were found when read. Note this, and try to write.
};


/**
	A TiXmlHandle is a class that wraps a node pointer with null checks; this is
	an incredibly useful thing. Note that TiXmlHandle is not part of the TinyXml
	DOM structure. It is a separate utility class.
*/
class TiXmlHandle
{
public:
	/// Create a handle from any node (at any depth of the tree.) This can be a null pointer.
	TiXmlHandle( TiXmlNode* _node )					{ this->node = _node; }
	/// Copy constructor
	TiXmlHandle( const TiXmlHandle& ref )			{ this->node = ref.node; }
	TiXmlHandle operator=( const TiXmlHandle& ref ) { this->node = ref.node; return *this; }

	/// Return a handle to the first child node.
	TiXmlHandle FirstChild() const;                                 // 0x42dfc0
	/// Return a handle to the first child node with the given name.
	TiXmlHandle FirstChild( const char * value ) const;             // 0x42dff0
	/// Return a handle to the first child element.
	TiXmlHandle FirstChildElement() const;                          // 0x42e020

	/// Return the handle as a TiXmlNode. This may return null.
	TiXmlNode* Node() const			{ return node; }
	/// Return the handle as a TiXmlElement. This may return null.
	TiXmlElement* Element() const	{ return ( ( node && node->ToElement() ) ? node->ToElement() : 0 ); }
	/// Return the handle as a TiXmlText. This may return null.
	TiXmlText* Text() const			{ return ( ( node && node->ToText() ) ? node->ToText() : 0 ); }
	/// Return the handle as a TiXmlUnknown. This may return null;
	TiXmlUnknown* Unknown() const			{ return ( ( node && node->ToUnknown() ) ? node->ToUnknown() : 0 ); }

private:
	TiXmlNode* node;
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif

