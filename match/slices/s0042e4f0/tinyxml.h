// TinyXML 2.5.x (non-STL build, TiXmlString) as linked into mount&blade.exe: tinystr.cpp, tinyxml.cpp,
// tinyxmlparser.cpp occupy 0x42d820..0x431xxx. Layouts verified by the constructors in slices
// s0042e4f0 / s0042ee90 / s0042f840. Only members those slices call or define carry a
// real body or an address comment; the rest are declared to fix the vtable slots.
#ifndef MB_TINYXML_H
#define MB_TINYXML_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <new>
// The original calls the array operators (??_U 0x535482 / ??_V 0x53570d) even for POD arrays; cl 13.10 here
// lowers `new T[n]` / `delete[] p` of POD types to the scalar operators (??2 / ??3). Spell the calls out.
#define TX_NEW_ARRAY(T, n) ((T*)operator new[](sizeof(T) * (n)))
#define TX_DELETE_ARRAY(p) operator delete[](p)

class TiXmlDocument;
class TiXmlElement;
class TiXmlComment;
class TiXmlUnknown;
class TiXmlText;
class TiXmlDeclaration;
class TiXmlAttribute;
class TiXmlParsingData;

// ---- tinystr.h: reference-counting-free string, rep = { size, capacity, str[] } (data at +8) ----
class TiXmlString {
public:
    typedef size_t size_type;
    static const size_type npos;                                  // 0x54b8f4
    TiXmlString() : rep_(&nullrep_) {}
    TiXmlString(const char* copy);                                // 0x42ed00 (emitted out of line)
    ~TiXmlString() { quit(); }                                    // 0x42d820 where not inlined
    TiXmlString& operator=(const char* copy) { return assign(copy, (size_type)strlen(copy)); }
    TiXmlString& operator=(const TiXmlString& copy) { return assign(copy.start(), copy.length()); }
    TiXmlString& operator+=(const char* suffix) { return append(suffix, (size_type)strlen(suffix)); }
    TiXmlString& operator+=(char single) { return append(&single, 1); }
    TiXmlString& operator+=(const TiXmlString& suffix) { return append(suffix.data(), suffix.length()); }
    const char* c_str() const { return rep_->str; }
    const char* data() const { return rep_->str; }
    size_type length() const { return rep_->size; }
    size_type size() const { return rep_->size; }
    bool empty() const { return rep_->size == 0; }
    size_type capacity() const { return rep_->capacity; }
    const char& at(size_type index) const { return rep_->str[index]; }
    char& operator[](size_type index) const { return rep_->str[index]; }
    size_type find(char lookup) const { return find(lookup, 0); }
    size_type find(char tofind, size_type offset) const
    {
        if (offset >= length()) return npos;
        for (const char* p = c_str() + offset; *p != '\0'; ++p) {
            if (*p == tofind) return static_cast<size_type>(p - c_str());
        }
        return npos;
    }
    void reserve(size_type cap);                                  // 0x42d840
    TiXmlString& assign(const char* str, size_type len);          // 0x42d8b0
    TiXmlString& append(const char* str, size_type len);          // 0x42d950
private:
    void init(size_type sz) { init(sz, sz); }
    void init(size_type sz, size_type cap)
    {
        if (cap) {
            const size_type bytesNeeded = sizeof(Rep) + cap;
            const size_type intsNeeded = (bytesNeeded + sizeof(int) - 1) / sizeof(int);
            rep_ = reinterpret_cast<Rep*>(TX_NEW_ARRAY(int, intsNeeded));
            rep_->str[rep_->size = sz] = '\0';
            rep_->capacity = cap;
        } else
            rep_ = &nullrep_;
    }
    char* start() const { return rep_->str; }
    void quit()
    {
        if (rep_ != &nullrep_) TX_DELETE_ARRAY(reinterpret_cast<int*>(rep_));
    }
    struct Rep {
        size_type size, capacity;
        char str[1];
    };
    Rep* rep_;
    static Rep nullrep_;                                          // 0x5e23d0
};

class TiXmlOutStream : public TiXmlString {
public:
    TiXmlOutStream& operator<<(const TiXmlString& in) { *this += in; return *this; }
    TiXmlOutStream& operator<<(const char* in) { *this += in; return *this; }
};
#define TIXML_STRING TiXmlString
#define TIXML_OSTREAM TiXmlOutStream

enum TiXmlEncoding {
    TIXML_ENCODING_UNKNOWN,
    TIXML_ENCODING_UTF8,
    TIXML_ENCODING_LEGACY
};

struct TiXmlCursor {
    TiXmlCursor() { Clear(); }
    void Clear() { row = col = -1; }
    int row;
    int col;
};

enum { TIXML_SUCCESS, TIXML_NO_ATTRIBUTE, TIXML_WRONG_TYPE };

// error ids (tinyxml 2.5)
enum {
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

#define TIXML_UTF_LEAD_0 0xefU
#define TIXML_UTF_LEAD_1 0xbbU
#define TIXML_UTF_LEAD_2 0xbfU

class TiXmlBase {
public:
    TiXmlBase() : userData(0) {}
    virtual ~TiXmlBase() {}                                       // slot 0
    virtual void Print(FILE* cfile, int depth) const = 0;         // slot 1
    static const int utf8ByteTable[256];                          // 0x54bd88
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding) = 0; // slot 2
    static const char* const errorString[TIXML_ERROR_STRING_COUNT]; // 0x5a9070
    // slot 3 (declared in the node/attribute classes below through the same signature)
    virtual void StreamOut(TIXML_OSTREAM*) const = 0;

protected:
    static void PutString(const TIXML_STRING& str, TIXML_STRING* out);   // 0x42dad0
    static void PutString(const TIXML_STRING& str, TIXML_OSTREAM* stream)
    {
        TIXML_STRING buffer;
        PutString(str, &buffer);
        (*stream) << buffer;
    }
    static const char* SkipWhiteSpace(const char*, TiXmlEncoding encoding);      // 0x42fa70
    static bool IsWhiteSpace(char c);        // 0x42fa40; inline in the original, defined in the .cpp that needs it
    static const char* ReadName(const char* p, TIXML_STRING* name, TiXmlEncoding encoding); // 0x42fb30
    static bool StringEqual(const char* p, const char* endTag, bool ignoreCase, TiXmlEncoding encoding); // 0x42fbc0
    static const char* GetEntity(const char* in, char* value, int* length, TiXmlEncoding encoding);      // 0x42fe00
    static void ConvertUTF32ToUTF8(unsigned long input, char* output, int* length);                      // 0x42f720
    static int IsAlpha(unsigned char anyByte, TiXmlEncoding /*encoding*/);  // 0x42f820; inline in the original
    static int IsAlphaNum(unsigned char anyByte, TiXmlEncoding /*encoding*/)
    {
        if (anyByte < 127) return isalnum(anyByte);
        else return 1;
    }
    static int ToLower(int v, TiXmlEncoding encoding)
    {
        if (encoding == TIXML_ENCODING_UTF8) {
            if (v < 128) return tolower(v);
            return v;
        } else {
            return tolower(v);
        }
    }

    TiXmlCursor location;
    void* userData;
};

class TiXmlParsingData {
public:
    void Stamp(const char* now, TiXmlEncoding encoding);          // 0x42f840
    const TiXmlCursor& Cursor() { return cursor; }
    TiXmlCursor cursor;
    const char* stamp;
    int tabsize;
};

class TiXmlNode : public TiXmlBase {
public:
    enum NodeType { DOCUMENT, ELEMENT, COMMENT, UNKNOWN, TEXT, DECLARATION, TYPECOUNT };
    virtual ~TiXmlNode();                                         // body 0x42e170 (deleting dtor 0x42f010)
    void SetValue(const char* _value) { value = _value; }
    const char* Value() const { return value.c_str(); }
    TiXmlDocument* GetDocument();                                 // 0x42d9d0
    const TiXmlDocument* GetDocument() const;                     // 0x42d9d0 (identical code)
    virtual const TiXmlDocument* ToDocument() const { return 0; }
    virtual TiXmlDocument* ToDocument() { return 0; }
    virtual const TiXmlElement* ToElement() const { return 0; }
    virtual TiXmlElement* ToElement() { return 0; }
    virtual const TiXmlComment* ToComment() const { return 0; }
    virtual TiXmlComment* ToComment() { return 0; }
    virtual const TiXmlUnknown* ToUnknown() const { return 0; }
    virtual TiXmlUnknown* ToUnknown() { return 0; }
    virtual const TiXmlText* ToText() const { return 0; }
    virtual TiXmlText* ToText() { return 0; }
    virtual const TiXmlDeclaration* ToDeclaration() const { return 0; }
    virtual TiXmlDeclaration* ToDeclaration() { return 0; }
    virtual TiXmlNode* Clone() const = 0;                         // slot 16
    TiXmlNode* NextSibling() const { return next; }
    TiXmlNode* LinkEndChild(TiXmlNode* node)
    {
        node->parent = this;
        node->prev = lastChild;
        node->next = 0;
        if (lastChild)
            lastChild->next = node;
        else
            firstChild = node;
        lastChild = node;
        return node;
    }
    void Clear()
    {
        TiXmlNode* node = firstChild;
        TiXmlNode* temp = 0;
        while (node) {
            temp = node;
            node = node->next;
            delete temp;
        }
        firstChild = 0;
        lastChild = 0;
    }
protected:
    TiXmlNode(NodeType _type) : TiXmlBase()
    {
        parent = 0;
        type = _type;
        firstChild = 0;
        lastChild = 0;
        prev = 0;
        next = 0;
    }
    void CopyTo(TiXmlNode* target) const
    {
        target->SetValue(value.c_str());
        target->userData = userData;
    }
    TiXmlNode* Identify(const char* start, TiXmlEncoding encoding); // 0x42ffc0

    TiXmlNode* parent;
    NodeType type;
    TiXmlNode* firstChild;
    TiXmlNode* lastChild;
    TIXML_STRING value;
    TiXmlNode* prev;
    TiXmlNode* next;
};

class TiXmlAttribute : public TiXmlBase {
public:
    TiXmlAttribute() : TiXmlBase() { document = 0; prev = next = 0; }
    TiXmlAttribute(const char* _name, const char* _value);        // 0x42e070
    const char* Name() const { return name.c_str(); }
    const char* Value() const { return value.c_str(); }
    const TiXmlAttribute* Next() const
    {
        if (next->value.empty() && next->name.empty()) return 0;
        return next;
    }
    int QueryIntValue(int* ival) const
    {
        if (sscanf(value.c_str(), "%d", ival) == 1) return TIXML_SUCCESS;
        return TIXML_WRONG_TYPE;
    }
    int QueryDoubleValue(double* dval) const
    {
        if (sscanf(value.c_str(), "%lf", dval) == 1) return TIXML_SUCCESS;
        return TIXML_WRONG_TYPE;
    }
    void SetValue(const char* _value) { value = _value; }
    virtual void Print(FILE* cfile, int depth) const;             // 0x42e6f0
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding); // 0x430d30
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42e7c0
    TiXmlDocument* document;
    TIXML_STRING name;
    TIXML_STRING value;
    TiXmlAttribute* prev;
    TiXmlAttribute* next;
};

class TiXmlAttributeSet {
public:
    TiXmlAttributeSet() { sentinel.next = &sentinel; sentinel.prev = &sentinel; }
    ~TiXmlAttributeSet();
    void Add(TiXmlAttribute* addMe)
    {
        addMe->next = &sentinel;
        addMe->prev = sentinel.prev;
        sentinel.prev->next = addMe;
        sentinel.prev = addMe;
    }
    const TiXmlAttribute* First() const { return (sentinel.next == &sentinel) ? 0 : sentinel.next; }
    TiXmlAttribute* Find(const TIXML_STRING& name) const;         // 0x42df40
    TiXmlAttribute sentinel;
};

class TiXmlElement : public TiXmlNode {
public:
    TiXmlElement(const char* in_value);                           // 0x42f030
    virtual ~TiXmlElement();                                      // 0x42f660
    const char* Attribute(const char* name) const;                // 0x42f0b0
    int QueryIntAttribute(const char* name, int* ival) const;     // 0x42f110
    int QueryDoubleAttribute(const char* name, double* dval) const; // 0x42f190
    void SetAttribute(const char* name, const char* value);       // 0x42f210
    virtual TiXmlNode* Clone() const;                             // 0x42f3b0
    virtual void Print(FILE* cfile, int depth) const;             // 0x42e1b0
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding); // 0x430860
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42e300
    virtual const TiXmlElement* ToElement() const { return this; }
    virtual TiXmlElement* ToElement() { return this; }
protected:
    void ClearThis();                                             // 0x42dcb0
    void CopyTo(TiXmlElement* target) const;                      // 0x42f300
protected:
    TiXmlAttributeSet attributeSet;
};

class TiXmlComment : public TiXmlNode {
public:
    TiXmlComment();                                               // 0x42edc0 (inline in the original)
    virtual ~TiXmlComment();                                      // 0x42ee00
    virtual TiXmlNode* Clone() const;                             // 0x42f470
    virtual void Print(FILE* cfile, int depth) const;             // 0x42de40
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding); // 0x430c60
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42e960
    virtual const TiXmlComment* ToComment() const { return this; }
    virtual TiXmlComment* ToComment() { return this; }
};

class TiXmlText : public TiXmlNode {
public:
    TiXmlText(const char* initValue);                             // 0x42ee30 (inline in the original)
    virtual ~TiXmlText();                                         // 0x42ee90
    virtual void Print(FILE* cfile, int depth) const;             // 0x42e9a0
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding); // 0x430eb0
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42ea60
    bool Blank() const;                                           // 0x42fdb0
    void SetCDATA(bool _cdata) { cdata = _cdata; }
    virtual TiXmlNode* Clone() const;                             // 0x42f500
    virtual const TiXmlText* ToText() const { return this; }
    virtual TiXmlText* ToText() { return this; }
    bool cdata;
};

class TiXmlDeclaration : public TiXmlNode {
public:
    TiXmlDeclaration() : TiXmlNode(TiXmlNode::DECLARATION) {}     // 0x42eec0 (inline)
    __declspec(noinline) virtual ~TiXmlDeclaration();                                  // 0x42ef20 (deleting dtor 0x42ef00)
    virtual TiXmlNode* Clone() const;                             // 0x42f560
    virtual void Print(FILE* cfile, int depth) const;             // 0x42de80 .. printed through 0x431050
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding);
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42eaf0
    virtual const TiXmlDeclaration* ToDeclaration() const { return this; }
    virtual TiXmlDeclaration* ToDeclaration() { return this; }
protected:
    void CopyTo(TiXmlDeclaration* target) const;                  // 0x42ec50
private:
    TIXML_STRING version;
    TIXML_STRING encoding;
    TIXML_STRING standalone;
};

class TiXmlUnknown : public TiXmlNode {
public:
    TiXmlUnknown() : TiXmlNode(TiXmlNode::UNKNOWN) {}             // 0x42ef70 (inline)
    virtual ~TiXmlUnknown();                                      // 0x42efb0
    virtual TiXmlNode* Clone() const;                             // 0x42f5d0
    virtual void Print(FILE* cfile, int depth) const;             // 0x42df00
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding); // 0x42fce0
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42ecc0
    virtual const TiXmlUnknown* ToUnknown() const { return this; }
    virtual TiXmlUnknown* ToUnknown() { return this; }
};

class TiXmlDocument : public TiXmlNode {
public:
    TiXmlDocument();                                              // 0x42e3e0
    virtual ~TiXmlDocument();                                     // 0x42efe0
    bool LoadFile(TiXmlEncoding encoding = TIXML_ENCODING_UNKNOWN);                       // 0x42f6c0
    bool LoadFile(const char* filename, TiXmlEncoding encoding = TIXML_ENCODING_UNKNOWN); // 0x42f3f0
    bool LoadFile(FILE*, TiXmlEncoding encoding = TIXML_ENCODING_UNKNOWN);                // 0x42e4f0
    virtual TiXmlNode* Clone() const;                             // 0x42e6b0
    virtual void Print(FILE* cfile, int depth) const;             // 0x42ddd0
    virtual const char* Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding); // 0x4305b0
    virtual void StreamOut(TIXML_OSTREAM* out) const;             // 0x42de10
    virtual const TiXmlDocument* ToDocument() const { return this; }
    virtual TiXmlDocument* ToDocument() { return this; }
    bool Error() const { return error; }
    void SetError(int err, const char* errorLocation, TiXmlParsingData* prevData, TiXmlEncoding encoding); // 0x42fc70
protected:
    void CopyTo(TiXmlDocument* target) const;                     // 0x42dd30
private:
    bool error;
    int errorId;
    TIXML_STRING errorDesc;
    int tabsize;
    TiXmlCursor errorLocation;
    bool useMicrosoftBOM;
};

#endif
