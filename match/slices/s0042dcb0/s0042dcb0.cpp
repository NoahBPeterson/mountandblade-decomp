// Slice s0042dcb0: TinyXML (the engine embeds the non-STL TinyXML 2.4/2.5): element/document/declaration/
// comment/unknown output, attribute and node constructors, attribute-set lookup and TiXmlHandle.
#include <stdio.h>
#include <string.h>
#include "tinyxml.h"

TiXmlString::Rep TiXmlString::nullrep_;       // 0x5e23d0

// @ 0x42d820 (visible here so that it inlines into the destructors; the out-of-line copy is s0042d360's)
inline void TiXmlString::quit()
{
    if (rep_ != &nullrep_)
        delete[] reinterpret_cast<int*>(rep_);
}

// @ 0x42da80 (likewise: TiXmlBase's empty destructor inlines into the derived ones)
TiXmlBase::~TiXmlBase() {}

// @ 0x42da20 (defined here too: it is inlined into this TU's callers; the out-of-line copy is s0042d360's)
TiXmlString& TiXmlString::operator=(const char* copy)
{
    return assign(copy, (size_type)strlen(copy));
}

// @ 0x42da50
TiXmlString& TiXmlString::operator+=(const char* suffix)
{
    return append(suffix, (size_type)strlen(suffix));
}

// @ 0x42d9a0 (defined here so that CopyTo inlines it, as in the original; the out-of-line copy is s0042d360's)
TiXmlNode* TiXmlNode::LinkEndChild(TiXmlNode* node)
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

// @ 0x42dcb0
// (the original range 0x42dcb0..0x42ce.. also covers padding; see nonmatching.txt: this is TiXmlElement::ClearThis)
void TiXmlElement::ClearThis()
{
    // TiXmlNode::Clear(), inlined
    TiXmlNode* node = firstChild;
    TiXmlNode* temp = 0;
    while (node) {
        temp = node;
        node = node->next;
        delete temp;
    }
    firstChild = 0;
    lastChild = 0;
    while (attributeSet.First()) {
        TiXmlAttribute* attr = attributeSet.First();
        attributeSet.Remove(attr);
        delete attr;
    }
}

// @ 0x42dd30
void TiXmlDocument::CopyTo(TiXmlDocument* target) const
{
    target->SetValue(value.c_str());
    target->userData = userData;
    target->error = error;
    target->errorDesc = errorDesc.c_str();
    const TiXmlNode* node;
    for (node = firstChild; node; node = node->next)
        target->LinkEndChild(node->Clone());
}

// @ 0x42ddd0
void TiXmlDocument::Print(FILE* cfile, int depth) const
{
    const TiXmlNode* node;
    for (node = FirstChild(); node; node = node->NextSibling()) {
        node->Print(cfile, depth);
        fprintf(cfile, "\n");
    }
}

// @ 0x42de10
void TiXmlDocument::StreamOut(TIXML_OSTREAM* out) const
{
    TiXmlNode* node;
    for (node = FirstChild(); node; node = node->NextSibling()) {
        node->StreamOut(out);
        if (node->ToElement())
            break;
    }
}

// @ 0x42de40
void TiXmlComment::Print(FILE* cfile, int depth) const
{
    for (int i = 0; i < depth; i++)
        fputs("    ", cfile);
    fprintf(cfile, "<!--%s-->", value.c_str());
}

// @ 0x42de80
void TiXmlDeclaration::Print(FILE* cfile, int) const
{
    fprintf(cfile, "<?xml ");
    if (!version.empty())
        fprintf(cfile, "version=\"%s\" ", version.c_str());
    if (!encoding.empty())
        fprintf(cfile, "encoding=\"%s\" ", encoding.c_str());
    if (!standalone.empty())
        fprintf(cfile, "standalone=\"%s\" ", standalone.c_str());
    fprintf(cfile, "?>");
}

// @ 0x42df00
void TiXmlUnknown::Print(FILE* cfile, int depth) const
{
    for (int i = 0; i < depth; i++)
        fprintf(cfile, "    ");
    fprintf(cfile, "<%s>", value.c_str());
}

// @ 0x42df40
TiXmlAttribute* TiXmlAttributeSet::Find(const TiXmlString& name) const
{
    for (TiXmlAttribute* node = sentinel.next; node != &sentinel; node = node->next) {
        if (node->name == name)
            return node;
    }
    return 0;
}

// @ 0x42dfc0
TiXmlHandle TiXmlHandle::FirstChild() const
{
    if (node) {
        TiXmlNode* child = node->FirstChild();
        if (child)
            return TiXmlHandle(child);
    }
    return TiXmlHandle(0);
}

// @ 0x42dff0
TiXmlHandle TiXmlHandle::FirstChild(const char* value) const
{
    if (node) {
        TiXmlNode* child = node->FirstChild(value);
        if (child)
            return TiXmlHandle(child);
    }
    return TiXmlHandle(0);
}

// @ 0x42e020
TiXmlHandle TiXmlHandle::FirstChildElement() const
{
    if (node) {
        TiXmlElement* child = node->FirstChildElement();
        if (child)
            return TiXmlHandle(child);
    }
    return TiXmlHandle(0);
}

// @ 0x42e070
TiXmlAttribute::TiXmlAttribute(const char* _name, const char* _value)
{
    name = _name;
    value = _value;
    document = 0;
    prev = next = 0;
}

// @ 0x42e0f0
// TiXmlAttribute has no user-declared destructor: the implicit one is the function at this address (cl emits it
// out of line when it is called explicitly; the deleting destructor in the vtable calls it).
void destroy_attribute(TiXmlAttribute* a) { a->TiXmlAttribute::~TiXmlAttribute(); }

// @ 0x42e130
TiXmlNode::TiXmlNode(NodeType _type) : TiXmlBase()
{
    parent = 0;
    type = _type;
    firstChild = 0;
    lastChild = 0;
    prev = 0;
    next = 0;
}

// @ 0x42e170
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

// @ 0x42e1b0
void TiXmlElement::Print(FILE* cfile, int depth) const
{
    int i;
    for (i = 0; i < depth; i++)
        fprintf(cfile, "    ");
    fprintf(cfile, "<%s", value.c_str());
    const TiXmlAttribute* attrib;
    for (attrib = attributeSet.First(); attrib; attrib = attrib->Next()) {
        fprintf(cfile, " ");
        attrib->Print(cfile, depth);
    }
    const TiXmlNode* node;
    if (!firstChild) {
        fprintf(cfile, " />");
    } else if (firstChild == lastChild && static_cast<const TiXmlNode*>(firstChild)->ToText()) {
        fprintf(cfile, ">");
        firstChild->Print(cfile, depth + 1);
        fprintf(cfile, "</%s>", value.c_str());
    } else {
        fprintf(cfile, ">");
        for (node = firstChild; node; node = node->NextSibling()) {
            if (!node->ToText())
                fprintf(cfile, "\n");
            node->Print(cfile, depth + 1);
        }
        fprintf(cfile, "\n");
        for (i = 0; i < depth; ++i)
            fprintf(cfile, "    ");
        fprintf(cfile, "</%s>", value.c_str());
    }
}

// @ 0x42e300
void TiXmlElement::StreamOut(TIXML_OSTREAM* stream) const
{
    (*stream) << "<" << value;
    const TiXmlAttribute* attrib;
    for (attrib = attributeSet.First(); attrib; attrib = attrib->Next()) {
        (*stream) << " ";
        attrib->StreamOut(stream);
    }
    const TiXmlNode* node;
    if (firstChild) {
        (*stream) << ">";
        for (node = firstChild; node; node = node->NextSibling())
            node->StreamOut(stream);
        (*stream) << "</" << value << ">";
    } else {
        (*stream) << " />";
    }
}

// @ 0x42e3e0
TiXmlDocument::TiXmlDocument() : TiXmlNode(TiXmlNode::DOCUMENT)
{
    tabsize = 4;
    useMicrosoftBOM = false;
    ClearError();
}

// @ 0x42e450
TiXmlDocument::TiXmlDocument(const char* documentName) : TiXmlNode(TiXmlNode::DOCUMENT)
{
    tabsize = 4;
    useMicrosoftBOM = false;
    value = documentName;
    ClearError();
}
