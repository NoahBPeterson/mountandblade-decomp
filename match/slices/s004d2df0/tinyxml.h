// tinyxml.h: the engine embeds TinyXML 2.4/2.5 (non-STL build: its own TiXmlString and TiXmlOutStream, no
// TiXmlEncoding). Layouts and vtable slots confirmed by the functions at 0x42d820..0x42dc80 (s0042d360) and
// 0x42dcb0..0x42e450 (s0042dcb0). Copied into each slice directory that uses it.
#ifndef MB_TINYXML_H
#define MB_TINYXML_H
#include <stdio.h>
#include <string.h>

class TiXmlString {
public:
    typedef unsigned size_type;
    TiXmlString() : rep_(&nullrep_) {}
    ~TiXmlString() { quit(); }                              // 0x42d820 is its out-of-line copy (quit)
    TiXmlString& operator=(const char* copy);               // 0x42da20
    TiXmlString& operator+=(const char* suffix);            // 0x42da50
    TiXmlString& operator+=(char single) { return append(&single, 1); }
    TiXmlString& operator+=(const TiXmlString& suffix) { return append(suffix.data(), suffix.length()); }
    const char* c_str() const { return rep_->str; }
    const char* data() const { return rep_->str; }
    size_type length() const { return rep_->size; }
    bool empty() const { return rep_->size == 0; }
    size_type capacity() const { return rep_->capacity; }
    char& operator[](size_type index) const { return rep_->str[index]; }
    void reserve(size_type cap);                            // 0x42d840
    __declspec(noinline) TiXmlString& assign(const char* str, size_type len);    // 0x42d8b0
    __declspec(noinline) TiXmlString& append(const char* str, size_type len);    // 0x42d950 (tinystr.cpp: never inlined into tinyxml.cpp)
    void swap(TiXmlString& other) { Rep* r = rep_; rep_ = other.rep_; other.rep_ = r; }
private:
    void init(size_type sz) { init(sz, sz); }
    void set_size(size_type sz) { rep_->str[rep_->size = sz] = '\0'; }
    char* start() const { return rep_->str; }
    char* finish() const { return rep_->str + rep_->size; }
    struct Rep { size_type size, capacity; char str[1]; };
    void init(size_type sz, size_type cap)
    {
        if (cap) {
            const size_type bytesNeeded = sizeof(Rep) + cap;
            const size_type intsNeeded = (bytesNeeded + sizeof(int) - 1) / sizeof(int);
            rep_ = reinterpret_cast<Rep*>(new int[intsNeeded]);
            rep_->str[rep_->size = sz] = '\0';
            rep_->capacity = cap;
        } else {
            rep_ = &nullrep_;
        }
    }
    void quit();                                            // 0x42d820
    Rep* rep_;
    static Rep nullrep_;                                    // 0x5e23d0
};
inline bool operator==(const TiXmlString& a, const TiXmlString& b)
{
    return (a.length() == b.length()) && (strcmp(a.c_str(), b.c_str()) == 0);
}

// TIXML_OSTREAM of the non-STL build.
class TiXmlOutStream : public TiXmlString {
public:
    TiXmlOutStream& operator<<(const TiXmlString& in) { *this += in; return *this; }
    TiXmlOutStream& operator<<(const char* in) { *this += in; return *this; }
};
#define TIXML_OSTREAM TiXmlOutStream

struct TiXmlCursor {
    TiXmlCursor() { Clear(); }
    void Clear() { row = col = -1; }
    int row, col;
};

class TiXmlDocument; class TiXmlElement; class TiXmlComment; class TiXmlUnknown; class TiXmlText;
class TiXmlDeclaration; class TiXmlAttribute;

class TiXmlBase {
public:
    TiXmlBase() : userData(0) {}
    virtual ~TiXmlBase();                                   // 0 (deleting dtor 0x42da80, vtable 0x54b8f8)
    virtual void Print(FILE* cfile, int depth) const = 0;   // 1
    virtual const char* Parse(const char* p, void* data) = 0;   // 2
    virtual void StreamOut(TIXML_OSTREAM* out) const = 0;   // 3
    static void PutString(const TiXmlString& str, TiXmlString* out);   // 0x42dad0 (cdecl)
    TiXmlCursor location;           // +4
    void* userData;                 // +0xc
};

class TiXmlNode : public TiXmlBase {
public:
    enum NodeType { DOCUMENT, ELEMENT, COMMENT, UNKNOWN, TEXT, DECLARATION, TYPECOUNT };
    TiXmlNode(NodeType _type);                              // 0x42e130
    virtual ~TiXmlNode();                                   // 0x42e170 (body)
    const char* Value() const { return value.c_str(); }
    void SetValue(const char* _value) { value = _value; }
    TiXmlNode* FirstChild() const { return firstChild; }
    TiXmlNode* FirstChild(const char* value) const;         // 0x42dc10
    TiXmlNode* NextSibling() const { return next; }
    TiXmlElement* NextSiblingElement() const;               // 0x42dc80
    TiXmlElement* FirstChildElement() const
    {
        const TiXmlNode* node;
        for (node = FirstChild(); node; node = node->NextSibling()) {
            if (node->ToElement())
                return const_cast<TiXmlElement*>(node->ToElement());
        }
        return 0;
    }
    void Clear();
    TiXmlNode* LinkEndChild(TiXmlNode* addThis);            // 0x42d9a0
    const TiXmlDocument* GetDocument() const;               // 0x42d9d0
    virtual TiXmlDocument* ToDocument() { return 0; }
    virtual const TiXmlDocument* ToDocument() const { return 0; }   // MSVC lays overloads out in reverse: this one first
    virtual const TiXmlElement* ToElement() const { return 0; }   // MSVC lays overloads out in reverse: this one first
    virtual TiXmlElement* ToElement() { return 0; }
    virtual TiXmlComment* ToComment() { return 0; }
    virtual const TiXmlComment* ToComment() const { return 0; }   // MSVC lays overloads out in reverse: this one first
    virtual TiXmlUnknown* ToUnknown() { return 0; }
    virtual const TiXmlUnknown* ToUnknown() const { return 0; }   // MSVC lays overloads out in reverse: this one first
    virtual TiXmlText* ToText() { return 0; }
    virtual const TiXmlText* ToText() const { return 0; }   // MSVC lays overloads out in reverse: this one first
    virtual TiXmlDeclaration* ToDeclaration() { return 0; }
    virtual const TiXmlDeclaration* ToDeclaration() const { return 0; }   // MSVC lays overloads out in reverse: this one first
    virtual TiXmlNode* Clone() const = 0;                            // 16 (0x40)

    TiXmlNode* parent;              // +0x10
    NodeType type;                  // +0x14
    TiXmlNode* firstChild;          // +0x18
    TiXmlNode* lastChild;           // +0x1c
    TiXmlString value;              // +0x20
    TiXmlNode* prev;                // +0x24
    TiXmlNode* next;                // +0x28
};

class TiXmlAttribute : public TiXmlBase {
public:
    TiXmlAttribute(const char* _name, const char* _value);  // 0x42e070
    void SetValue(const char* _value);                      // 0x42daa0
    const TiXmlAttribute* Next() const
    {
        // The sentinel has neither a value nor a name.
        if (next->value.empty() && next->name.empty())
            return 0;
        return next;
    }
    virtual void Print(FILE* cfile, int depth) const;
    virtual const char* Parse(const char* p, void* data);
    virtual void StreamOut(TIXML_OSTREAM* out) const;
    TiXmlDocument* document;        // +0x10
    TiXmlString name;               // +0x14
    TiXmlString value;              // +0x18
    TiXmlAttribute* prev;           // +0x1c
    TiXmlAttribute* next;           // +0x20
};

// The sentinel of the circular attribute list is itself a TiXmlAttribute (0x24 bytes), embedded in the element.
class TiXmlAttributeSet {
public:
    TiXmlAttributeSet();
    ~TiXmlAttributeSet();
    void Add(TiXmlAttribute* attribute);                    // 0x42da00 (sentinel.next / sentinel.prev links)
    void Remove(TiXmlAttribute* removeMe)
    {
        TiXmlAttribute* node;
        for (node = sentinel.next; node != &sentinel; node = node->next) {
            if (node == removeMe) {
                node->prev->next = node->next;
                node->next->prev = node->prev;
                node->next = 0;
                node->prev = 0;
                return;
            }
        }
    }
    TiXmlAttribute* Find(const TiXmlString& name) const;    // 0x42df40
    TiXmlAttribute* First() const { return (sentinel.next == &sentinel) ? 0 : sentinel.next; }
    TiXmlAttribute sentinel;
};

class TiXmlElement : public TiXmlNode {
public:
    TiXmlElement(const char* value);
    virtual ~TiXmlElement();
    virtual void Print(FILE* cfile, int depth) const;       // 0x42e1b0
    virtual const char* Parse(const char* p, void* data);
    virtual void StreamOut(TIXML_OSTREAM* out) const;       // 0x42e300
    virtual TiXmlNode* Clone() const;
    const char* Attribute(const char* name) const;          // 0x42f0b0
    int QueryDoubleAttribute(const char* name, double* dval) const;   // 0x42f190
    void ClearThis();                                       // 0x42dcb0
    void Clear();
    TiXmlAttributeSet attributeSet; // +0x2c
};

class TiXmlComment : public TiXmlNode {
public:
    TiXmlComment() : TiXmlNode(TiXmlNode::COMMENT) {}
    virtual void Print(FILE* cfile, int depth) const;       // 0x42de40
    virtual const char* Parse(const char* p, void* data);
    virtual void StreamOut(TIXML_OSTREAM* out) const;
    virtual TiXmlNode* Clone() const;
};

class TiXmlUnknown : public TiXmlNode {
public:
    TiXmlUnknown() : TiXmlNode(TiXmlNode::UNKNOWN) {}
    virtual void Print(FILE* cfile, int depth) const;       // 0x42df00
    virtual const char* Parse(const char* p, void* data);
    virtual void StreamOut(TIXML_OSTREAM* out) const;
    virtual TiXmlNode* Clone() const;
};

class TiXmlDeclaration : public TiXmlNode {
public:
    TiXmlDeclaration() : TiXmlNode(TiXmlNode::DECLARATION) {}
    virtual void Print(FILE* cfile, int depth) const;       // 0x42de80
    virtual const char* Parse(const char* p, void* data);
    virtual void StreamOut(TIXML_OSTREAM* out) const;
    virtual TiXmlNode* Clone() const;
    TiXmlString version;            // +0x2c
    TiXmlString encoding;           // +0x30
    TiXmlString standalone;         // +0x34
};

class TiXmlDocument : public TiXmlNode {
public:
    TiXmlDocument();                                        // 0x42e3e0
    TiXmlDocument(const char* documentName);                // 0x42e450
    virtual void Print(FILE* cfile, int depth) const;       // 0x42ddd0
    virtual const char* Parse(const char* p, void* data);
    virtual void StreamOut(TIXML_OSTREAM* out) const;       // 0x42de10
    virtual TiXmlNode* Clone() const;
    void CopyTo(TiXmlDocument* target) const;               // 0x42dd30
    void ClearError() { error = false; errorId = 0; errorDesc = ""; errorLocation.row = errorLocation.col = 0; }
    bool error;                     // +0x2c
    int errorId;                    // +0x30
    TiXmlString errorDesc;          // +0x34
    int tabsize;                    // +0x38
    TiXmlCursor errorLocation;      // +0x3c
    bool useMicrosoftBOM;           // +0x44
};

class TiXmlHandle {
public:
    TiXmlHandle(TiXmlNode* node) { this->node = node; }
    TiXmlHandle FirstChild() const;                         // 0x42dfc0
    TiXmlHandle FirstChild(const char* value) const;        // 0x42dff0
    TiXmlHandle FirstChildElement() const;                  // 0x42e020
    TiXmlNode* node;
};
#endif
