// Slice s0042e4f0: TinyXML 2.5.x tinyxml.cpp -- document file loading, node/attribute printing and
// stream output, comment/text ctors.
#include "tinyxml.h"

// Not part of this slice (0x42e070 / 0x42e... elsewhere); defined first, as in tinyxml.cpp, so that
// ~TiXmlAttribute is compiled before its callers (this is what lets ~TiXmlString inline into 0x42ed70).
TiXmlAttribute::TiXmlAttribute(const char* _name, const char* _value)
{
    name = _name;
    value = _value;
    document = 0;
    prev = next = 0;
}
TiXmlAttributeSet::~TiXmlAttributeSet() {}

// @ 0x42e4f0
bool TiXmlDocument::LoadFile(FILE* file, TiXmlEncoding encoding)
{
    if (!file) {
        SetError(TIXML_ERROR_OPENING_FILE, 0, 0, TIXML_ENCODING_UNKNOWN);
        return false;
    }
    Clear();
    location.Clear();
    long length = 0;
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length == 0) {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, 0, 0, TIXML_ENCODING_UNKNOWN);
        return false;
    }
    TIXML_STRING data;
    data.reserve(length);
    char* buf = TX_NEW_ARRAY(char, length + 1);
    buf[0] = 0;
    if (fread(buf, length, 1, file) != 1) {
        SetError(TIXML_ERROR_OPENING_FILE, 0, 0, TIXML_ENCODING_UNKNOWN);
        return false;
    }
    const char* lastPos = buf;
    const char* p = buf;
    buf[length] = 0;
    while (*p) {
        if (*p == 0xa) {
            data.append(lastPos, (p - lastPos + 1));
            ++p;
            lastPos = p;
        } else if (*p == 0xd) {
            if ((p - lastPos) > 0) {
                data.append(lastPos, p - lastPos);
            }
            data += (char)0xa;
            if (*(p + 1) == 0xa) {
                p += 2;
                lastPos = p;
            } else {
                ++p;
                lastPos = p;
            }
        } else {
            ++p;
        }
    }
    if (p - lastPos) {
        data.append(lastPos, p - lastPos);
    }
    TX_DELETE_ARRAY(buf);
    Parse(data.c_str(), 0, encoding);
    if (Error())
        return false;
    else
        return true;
}

// @ 0x42e6b0
TiXmlNode* TiXmlDocument::Clone() const
{
    TiXmlDocument* clone = new TiXmlDocument();
    if (!clone) return 0;
    CopyTo(clone);
    return clone;
}

// @ 0x42e6f0
void TiXmlAttribute::Print(FILE* cfile, int /*depth*/) const
{
    TIXML_STRING n, v;
    PutString(name, &n);
    PutString(value, &v);
    if (value.find('\"') == TIXML_STRING::npos)
        fprintf(cfile, "%s=\"%s\"", n.c_str(), v.c_str());
    else
        fprintf(cfile, "%s='%s'", n.c_str(), v.c_str());
}

// @ 0x42e7c0
void TiXmlAttribute::StreamOut(TIXML_OSTREAM* stream) const
{
    if (value.find('\"') != TIXML_STRING::npos) {
        PutString(name, stream);
        (*stream) << "=" << "'";
        PutString(value, stream);
        (*stream) << "'";
    } else {
        PutString(name, stream);
        (*stream) << "=" << "\"";
        PutString(value, stream);
        (*stream) << "\"";
    }
}

// @ 0x42e960
void TiXmlComment::StreamOut(TIXML_OSTREAM* stream) const
{
    (*stream) << "<!--";
    (*stream) << value;
    (*stream) << "-->";
}

// @ 0x42e9a0
void TiXmlText::Print(FILE* cfile, int depth) const
{
    if (cdata) {
        int i;
        fprintf(cfile, "\n");
        for (i = 0; i < depth; i++) {
            fprintf(cfile, "    ");
        }
        fprintf(cfile, "<![CDATA[");
        fprintf(cfile, "%s", value.c_str());
        fprintf(cfile, "]]>\n");
    } else {
        TIXML_STRING buffer;
        PutString(value, &buffer);
        fprintf(cfile, "%s", buffer.c_str());
    }
}

// @ 0x42ea60
void TiXmlText::StreamOut(TIXML_OSTREAM* stream) const
{
    if (cdata) {
        (*stream) << "<![CDATA[" << value << "]]>";
    } else {
        PutString(value, stream);
    }
}

// @ 0x42eaf0
void TiXmlDeclaration::StreamOut(TIXML_OSTREAM* stream) const
{
    (*stream) << "<?xml ";
    if (!version.empty()) {
        (*stream) << "version=\"";
        PutString(version, stream);
        (*stream) << "\" ";
    }
    if (!encoding.empty()) {
        (*stream) << "encoding=\"";
        PutString(encoding, stream);
        (*stream) << "\" ";
    }
    if (!standalone.empty()) {
        (*stream) << "standalone=\"";
        PutString(standalone, stream);
        (*stream) << "\" ";
    }
    (*stream) << "?>";
}

// @ 0x42ec50
void TiXmlDeclaration::CopyTo(TiXmlDeclaration* target) const
{
    TiXmlNode::CopyTo(target);
    target->version = version;
    target->encoding = encoding;
    target->standalone = standalone;
}

// @ 0x42ecc0
void TiXmlUnknown::StreamOut(TIXML_OSTREAM* stream) const
{
    (*stream) << "<" << value << ">";
}

// @ 0x42ed00
TiXmlString::TiXmlString(const char* copy)
{
    init(static_cast<size_type>(strlen(copy)));
    memcpy(start(), copy, length());
}

// 0x42ed70: ~TiXmlAttribute is implicit; its scalar deleting form (??_GTiXmlAttribute) is emitted because the
// TiXmlAttribute constructor above builds the vtable.

// @ 0x42edc0
TiXmlComment::TiXmlComment() : TiXmlNode(TiXmlNode::COMMENT) {}

// @ 0x42ee00
TiXmlComment::~TiXmlComment() {}

// @ 0x42ee30
TiXmlText::TiXmlText(const char* initValue) : TiXmlNode(TiXmlNode::TEXT)
{
    SetValue(initValue);
    cdata = false;
}
