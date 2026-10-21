// Slice s0042f840: TinyXML 2.5.x tinyxmlparser.cpp -- parsing helpers (cursor stamping, white space, names,
// entities), TiXmlDocument::SetError, TiXmlUnknown::Parse, TiXmlText::Blank, TiXmlNode::Identify.
#include "../s0042e4f0/tinyxml.h"

// error strings (TiXmlBase::errorString, 0x5a9070) and entity table (0x5a90b0)
struct Entity {
    const char* str;
    unsigned int strLength;
    char chr;
};
enum { NUM_ENTITY = 5, MAX_ENTITY_LENGTH = 6 };
extern const Entity entity[NUM_ENTITY];                                  // 0x5a90b0
extern const char* const errorString_[TIXML_ERROR_STRING_COUNT];         // 0x5a9070 (== TiXmlBase::errorString)

int TiXmlBase::IsAlpha(unsigned char anyByte, TiXmlEncoding /*encoding*/)
{
    if (anyByte < 127)
        return isalpha(anyByte);
    else
        return 1;
}

// @ 0x42f840
void TiXmlParsingData::Stamp(const char* now, TiXmlEncoding encoding)
{
    if (tabsize < 1) return;
    int row = cursor.row;
    int col = cursor.col;
    const char* p = stamp;
    while (p < now) {
        const unsigned char* pU = (const unsigned char*)p;
        switch (*pU) {
        case 0:
            return;
        case '\r':
            ++row;
            col = 0;
            ++p;
            if (*p == '\n') {
                ++p;
            }
            break;
        case '\n':
            ++row;
            col = 0;
            ++p;
            if (*p == '\r') {
                ++p;
            }
            break;
        case '\t':
            ++p;
            col = (col / tabsize + 1) * tabsize;
            break;
        case TIXML_UTF_LEAD_0:
            if (encoding == TIXML_ENCODING_UTF8) {
                if (*(p + 1) && *(p + 2)) {
                    if (*(pU + 1) == TIXML_UTF_LEAD_1 && *(pU + 2) == TIXML_UTF_LEAD_2)
                        p += 3;
                    else if (*(pU + 1) == 0xbfU && *(pU + 2) == 0xbeU)
                        p += 3;
                    else if (*(pU + 1) == 0xbfU && *(pU + 2) == 0xbfU)
                        p += 3;
                    else {
                        p += 3;
                        ++col;
                    }
                }
            } else {
                ++p;
                ++col;
            }
            break;
        default:
            if (encoding == TIXML_ENCODING_UTF8) {
                int step = TiXmlBase::utf8ByteTable[*((unsigned char*)p)];
                if (step == 0) step = 1;
                p += step;
                ++col;
            } else {
                ++p;
                ++col;
            }
            break;
        }
    }
    cursor.row = row;
    cursor.col = col;
    stamp = p;
}

// @ 0x42fa40
bool TiXmlBase::IsWhiteSpace(char c)
{
    return (isspace((unsigned char)c) || c == '\n' || c == '\r');
}

// @ 0x42fa70
const char* TiXmlBase::SkipWhiteSpace(const char* p, TiXmlEncoding encoding)
{
    if (!p || !*p) {
        return 0;
    }
    if (encoding == TIXML_ENCODING_UTF8) {
        while (*p) {
            const unsigned char* pU = (const unsigned char*)p;
            if (*(pU + 0) == TIXML_UTF_LEAD_0 && *(pU + 1) == TIXML_UTF_LEAD_1 && *(pU + 2) == TIXML_UTF_LEAD_2) {
                p += 3;
                continue;
            } else if (*(pU + 0) == TIXML_UTF_LEAD_0 && *(pU + 1) == 0xbfU && *(pU + 2) == 0xbeU) {
                p += 3;
                continue;
            } else if (*(pU + 0) == TIXML_UTF_LEAD_0 && *(pU + 1) == 0xbfU && *(pU + 2) == 0xbfU) {
                p += 3;
                continue;
            }
            if (IsWhiteSpace(*p) || *p == '\n' || *p == '\r')
                ++p;
            else
                break;
        }
    } else {
        while (*p && IsWhiteSpace(*p) || *p == '\n' || *p == '\r')
            ++p;
    }
    return p;
}

// @ 0x42fb30
const char* TiXmlBase::ReadName(const char* p, TIXML_STRING* name, TiXmlEncoding encoding)
{
    *name = "";
    if (p && *p && (IsAlpha((unsigned char)*p, encoding) || *p == '_')) {
        while (p && *p && (IsAlphaNum((unsigned char)*p, encoding) || *p == '_' || *p == '-' || *p == '.' || *p == ':')) {
            (*name) += *p;
            ++p;
        }
        return p;
    }
    return 0;
}

// @ 0x42fbc0
bool TiXmlBase::StringEqual(const char* p, const char* tag, bool ignoreCase, TiXmlEncoding encoding)
{
    if (!p || !*p) {
        return false;
    }
    const char* q = p;
    if (ignoreCase) {
        while (*q && *tag && ToLower(*q, encoding) == ToLower(*tag, encoding)) {
            ++q;
            ++tag;
        }
        if (*tag == 0) return true;
    } else {
        while (*q && *tag && *q == *tag) {
            ++q;
            ++tag;
        }
        if (*tag == 0) return true;
    }
    return false;
}

// @ 0x42fc70
void TiXmlDocument::SetError(int err, const char* pError, TiXmlParsingData* data, TiXmlEncoding encoding)
{
    if (error) return;
    error = true;
    errorId = err;
    errorDesc = errorString_[errorId];
    errorLocation.Clear();
    if (pError && data) {
        data->Stamp(pError, encoding);
        errorLocation = data->Cursor();
    }
}

// @ 0x42fce0
const char* TiXmlUnknown::Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding)
{
    TiXmlDocument* document = GetDocument();
    p = SkipWhiteSpace(p, encoding);
    if (data) {
        data->Stamp(p, encoding);
        location = data->Cursor();
    }
    if (!p || !*p || *p != '<') {
        if (document) document->SetError(TIXML_ERROR_PARSING_UNKNOWN, p, data, encoding);
        return 0;
    }
    ++p;
    value = "";
    while (p && *p && *p != '>') {
        value += *p;
        ++p;
    }
    if (!p) {
        if (document) document->SetError(TIXML_ERROR_PARSING_UNKNOWN, 0, 0, encoding);
    }
    if (*p == '>')
        return p + 1;
    return p;
}

// @ 0x42fdb0
bool TiXmlText::Blank() const
{
    for (unsigned i = 0; i < value.length(); i++)
        if (!IsWhiteSpace(value[i])) return false;
    return true;
}

// @ 0x42fe00
const char* TiXmlBase::GetEntity(const char* p, char* value, int* length, TiXmlEncoding encoding)
{
    int i;
    *length = 0;
    if (*(p + 1) && *(p + 1) == '#' && *(p + 2)) {
        unsigned long ucs = 0;
        unsigned delta = 0;
        unsigned mult = 1;
        if (*(p + 2) == 'x') {
            if (!*(p + 3)) return 0;
            const char* q = strchr(p + 3, ';');
            if (!q || !*q) return 0;
            delta = q - p;
            --q;
            while (*q != 'x') {
                if (*q >= '0' && *q <= '9')
                    ucs += mult * (*q - '0');
                else if (*q >= 'a' && *q <= 'f')
                    ucs += mult * (*q - 'a' + 10);
                else if (*q >= 'A' && *q <= 'F')
                    ucs += mult * (*q - 'A' + 10);
                else
                    return 0;
                mult *= 16;
                --q;
            }
        } else {
            const char* q = strchr(p + 2, ';');
            if (!q || !*q) return 0;
            delta = q - p;
            --q;
            while (*q != '#') {
                if (*q >= '0' && *q <= '9')
                    ucs += mult * (*q - '0');
                else
                    return 0;
                mult *= 10;
                --q;
            }
        }
        if (encoding == TIXML_ENCODING_UTF8) {
            ConvertUTF32ToUTF8(ucs, value, length);
        } else {
            *value = (char)ucs;
            *length = 1;
        }
        return p + delta + 1;
    }
    for (i = 0; i < NUM_ENTITY; ++i) {
        if (strncmp(entity[i].str, p, entity[i].strLength) == 0) {
            *value = entity[i].chr;
            *length = 1;
            return (p + entity[i].strLength);
        }
    }
    *value = *p;
    return p + 1;
}

// @ 0x42ffc0
TiXmlNode* TiXmlNode::Identify(const char* p, TiXmlEncoding encoding)
{
    TiXmlNode* returnNode = 0;
    p = SkipWhiteSpace(p, encoding);
    if (!p || !*p || *p != '<') {
        return 0;
    }
    TiXmlDocument* doc = GetDocument();
    p = SkipWhiteSpace(p, encoding);
    if (!p || !*p) {
        return 0;
    }
    const char* xmlHeader = { "<?xml" };
    const char* commentHeader = { "<!--" };
    const char* dtdHeader = { "<!" };
    const char* cdataHeader = { "<![CDATA[" };
    if (StringEqual(p, xmlHeader, true, encoding)) {
        returnNode = new TiXmlDeclaration();
    } else if (StringEqual(p, commentHeader, false, encoding)) {
        returnNode = new TiXmlComment();
    } else if (StringEqual(p, cdataHeader, false, encoding)) {
        TiXmlText* text = new TiXmlText("");
        text->SetCDATA(true);
        returnNode = text;
    } else if (StringEqual(p, dtdHeader, false, encoding)) {
        returnNode = new TiXmlUnknown();
    } else if (IsAlpha(*(p + 1), encoding) || *(p + 1) == '_') {
        returnNode = new TiXmlElement("");
    } else {
        returnNode = new TiXmlUnknown();
    }
    if (returnNode) {
        returnNode->parent = this;
    } else {
        if (doc) doc->SetError(TIXML_ERROR_OUT_OF_MEMORY, 0, 0, TIXML_ENCODING_UNKNOWN);
    }
    return returnNode;
}
