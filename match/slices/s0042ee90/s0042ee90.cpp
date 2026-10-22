// Slice s0042ee90: TinyXML 2.5.x tinyxml.cpp -- node destructors/constructors, TiXmlElement attribute access,
// document LoadFile(filename), Clone() of each node kind, UTF-8 helpers.
#include "../s0042e4f0/tinyxml.h"

// The identical GetDocument() bodies of TiXmlNode (0x42d9d0, another slice) are defined earlier in the
// same translation unit, which is why SetAttribute below carries the loop inline.
TiXmlDocument* TiXmlNode::GetDocument()
{
    TiXmlNode* node;
    for (node = this; node; node = node->parent) {
        if (node->ToDocument()) return node->ToDocument();
    }
    return 0;
}

TiXmlAttributeSet::~TiXmlAttributeSet() {}

TiXmlComment::TiXmlComment() : TiXmlNode(TiXmlNode::COMMENT) {}
TiXmlText::TiXmlText(const char* initValue) : TiXmlNode(TiXmlNode::TEXT)
{
    SetValue(initValue);
    cdata = false;
}

// @ 0x42ee90
TiXmlText::~TiXmlText() {}

// @ 0x42eec0
// TiXmlDeclaration::TiXmlDeclaration() -- inline in the header; out-of-line copy

// @ 0x42ef20 (the destructor proper; defining it here also emits the deleting form 0x42ef00)
TiXmlDeclaration::~TiXmlDeclaration() {}

// @ 0x42ef70
// TiXmlUnknown::TiXmlUnknown()

// @ 0x42efb0
TiXmlUnknown::~TiXmlUnknown() {}

// @ 0x42efe0
// The original's deleting destructor keeps ~TiXmlString (0x42d820) out of line, which inline_depth(1) reproduces.
#pragma inline_depth(1)
TiXmlDocument::~TiXmlDocument() {}
#pragma inline_depth()

// @ 0x42e170 (the destructor proper, out of line in s0042dcb0; defining it here emits the deleting form 0x42f010)
TiXmlNode::~TiXmlNode()
{
    TiXmlNode* node = firstChild;
    TiXmlNode* temp = 0;
    while (node) {
        temp = node;
        node = node->next;
        delete temp;
    }
}

// @ 0x42f030
TiXmlElement::TiXmlElement(const char* _value) : TiXmlNode(TiXmlNode::ELEMENT)
{
    firstChild = lastChild = 0;
    value = _value;
}

// @ 0x42f0b0
const char* TiXmlElement::Attribute(const char* name) const
{
    TIXML_STRING _name(name);
    TiXmlAttribute* node = attributeSet.Find(_name);
    if (node) return node->Value();
    return 0;
}

// @ 0x42f110
int TiXmlElement::QueryIntAttribute(const char* name, int* ival) const
{
    TIXML_STRING _name(name);
    TiXmlAttribute* node = attributeSet.Find(_name);
    if (!node) return TIXML_NO_ATTRIBUTE;
    return node->QueryIntValue(ival);
}

// @ 0x42f190
int TiXmlElement::QueryDoubleAttribute(const char* name, double* dval) const
{
    TIXML_STRING _name(name);
    TiXmlAttribute* node = attributeSet.Find(_name);
    if (!node) return TIXML_NO_ATTRIBUTE;
    return node->QueryDoubleValue(dval);
}

// @ 0x42f210
void TiXmlElement::SetAttribute(const char* cname, const char* cvalue)
{
    TIXML_STRING _name(cname);
    TIXML_STRING _value(cvalue);
    TiXmlAttribute* node = attributeSet.Find(_name);
    if (node) {
        node->SetValue(cvalue);
        return;
    }
    TiXmlAttribute* attrib = new TiXmlAttribute(cname, cvalue);
    if (attrib) {
        attributeSet.Add(attrib);
    } else {
        TiXmlDocument* document = GetDocument();
        if (document) document->SetError(TIXML_ERROR_OUT_OF_MEMORY, 0, 0, TIXML_ENCODING_UNKNOWN);
    }
}

// @ 0x42f300
void TiXmlElement::CopyTo(TiXmlElement* target) const
{
    TiXmlNode::CopyTo(target);
    const TiXmlAttribute* attribute = 0;
    for (attribute = attributeSet.First(); attribute; attribute = attribute->Next()) {
        target->SetAttribute(attribute->Name(), attribute->Value());
    }
    TiXmlNode* node = 0;
    for (node = firstChild; node; node = node->NextSibling()) {
        target->LinkEndChild(node->Clone());
    }
}

// @ 0x42f3b0
TiXmlNode* TiXmlElement::Clone() const
{
    TiXmlElement* clone = new TiXmlElement(Value());
    if (!clone) return 0;
    CopyTo(clone);
    return clone;
}

// @ 0x42f3f0
bool TiXmlDocument::LoadFile(const char* filename, TiXmlEncoding encoding)
{
    value = filename;
    FILE* file = fopen(value.c_str(), "rb");
    if (file) {
        bool result = LoadFile(file, encoding);
        fclose(file);
        return result;
    } else {
        SetError(TIXML_ERROR_OPENING_FILE, 0, 0, TIXML_ENCODING_UNKNOWN);
        return false;
    }
}

// @ 0x42f470
TiXmlNode* TiXmlComment::Clone() const
{
    TiXmlComment* clone = new TiXmlComment();
    if (!clone) return 0;
    CopyTo(clone);
    return clone;
}

// @ 0x42f500
TiXmlNode* TiXmlText::Clone() const
{
    TiXmlText* clone = 0;
    clone = new TiXmlText("");
    if (!clone) return 0;
    CopyTo(clone);
    clone->cdata = cdata;
    return clone;
}

// @ 0x42f560
TiXmlNode* TiXmlDeclaration::Clone() const
{
    TiXmlDeclaration* clone = new TiXmlDeclaration();
    if (!clone) return 0;
    CopyTo(clone);
    return clone;
}

// @ 0x42f5d0
TiXmlNode* TiXmlUnknown::Clone() const
{
    TiXmlUnknown* clone = new TiXmlUnknown();
    if (!clone) return 0;
    CopyTo(clone);
    return clone;
}

// @ 0x42f660
TiXmlElement::~TiXmlElement()
{
    ClearThis();
}

// @ 0x42f6c0
bool TiXmlDocument::LoadFile(TiXmlEncoding encoding)
{
    char* buffer = TX_NEW_ARRAY(char, value.length() + 1);
    if (buffer) strcpy(buffer, value.c_str());
    if (buffer && LoadFile(buffer, encoding)) {
        TX_DELETE_ARRAY(buffer);
        return true;
    }
    TX_DELETE_ARRAY(buffer);
    return false;
}

// @ 0x42f720
void TiXmlBase::ConvertUTF32ToUTF8(unsigned long input, char* output, int* length)
{
    const unsigned long BYTE_MASK = 0xBF;
    const unsigned long BYTE_MARK = 0x80;
    const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    if (input < 0x80)
        *length = 1;
    else if (input < 0x800)
        *length = 2;
    else if (input < 0x10000)
        *length = 3;
    else if (input < 0x200000)
        *length = 4;
    else {
        *length = 0;
        return;
    }
    output += *length;
    switch (*length) {
    case 4:
        --output;
        *output = (char)((input | BYTE_MARK) & BYTE_MASK);
        input >>= 6;
    case 3:
        --output;
        *output = (char)((input | BYTE_MARK) & BYTE_MASK);
        input >>= 6;
    case 2:
        --output;
        *output = (char)((input | BYTE_MARK) & BYTE_MASK);
        input >>= 6;
    case 1:
        --output;
        *output = (char)(input | FIRST_BYTE_MARK[*length]);
    }
}

// @ 0x42f820
int TiXmlBase::IsAlpha(unsigned char anyByte, TiXmlEncoding /*encoding*/)
{
    if (anyByte < 127)
        return isalpha(anyByte);
    else
        return 1;
}
