// Slice s00430860: TinyXML 2.5.x tinyxmlparser.cpp -- TiXmlElement::Parse, TiXmlComment::Parse,
// TiXmlAttribute::Parse, TiXmlText::Parse (CDATA aware).
#include <new>      // declares operator delete[] (cl then frees arrays through ??_V, as the original does)
#include "tinyxml.h"

// Not part of this slice: defined first so cl sees its register use (it keeps ECX), as in the original TU.
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


// @ 0x430860
const char* TiXmlElement::Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding)
{
    p = SkipWhiteSpace(p, encoding);
    TiXmlDocument* document = GetDocument();

    if (!p || !*p) {
        if (document) document->SetError(TIXML_ERROR_PARSING_ELEMENT, 0, 0, encoding);
        return 0;
    }
    if (data) {
        data->Stamp(p, encoding);
        location = data->Cursor();
    }
    if (*p != '<') {
        if (document) document->SetError(TIXML_ERROR_PARSING_ELEMENT, p, data, encoding);
        return 0;
    }
    p = SkipWhiteSpace(p + 1, encoding);

    const char* pErr = p;
    p = ReadName(p, &value, encoding);
    if (!p || !*p) {
        if (document) document->SetError(TIXML_ERROR_FAILED_TO_READ_ELEMENT_NAME, pErr, data, encoding);
        return 0;
    }

    TIXML_STRING endTag("</");
    endTag += value;
    endTag += ">";

    while (p && *p) {
        pErr = p;
        p = SkipWhiteSpace(p, encoding);
        if (!p || !*p) {
            if (document) document->SetError(TIXML_ERROR_READING_ATTRIBUTES, pErr, data, encoding);
            return 0;
        }
        if (*p == '/') {
            ++p;
            if (*p != '>') {
                if (document) document->SetError(TIXML_ERROR_PARSING_EMPTY, p, data, encoding);
                return 0;
            }
            return (p + 1);
        } else if (*p == '>') {
            ++p;
            p = ReadValue(p, data, encoding);
            if (!p || !*p)
                return 0;
            if (StringEqual(p, endTag.c_str(), false, encoding)) {
                p += endTag.length();
                return p;
            } else {
                if (document) document->SetError(TIXML_ERROR_READING_END_TAG, p, data, encoding);
                return 0;
            }
        } else {
            TiXmlAttribute* attrib = new TiXmlAttribute();
            if (!attrib) {
                if (document) document->SetError(TIXML_ERROR_OUT_OF_MEMORY, pErr, data, encoding);
                return 0;
            }
            attrib->SetDocument(document);
            const char* pErr = p;
            p = attrib->Parse(p, data, encoding);
            if (!p || !*p) {
                if (document) document->SetError(TIXML_ERROR_PARSING_ELEMENT, pErr, data, encoding);
                delete attrib;
                return 0;
            }
            TiXmlAttribute* node = attributeSet.Find(attrib->name);
            if (node) {
                node->SetValue(attrib->Value());
                delete attrib;
                return 0;
            }
            attributeSet.Add(attrib);
        }
    }
    return p;
}

// @ 0x430c60
const char* TiXmlComment::Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding)
{
    TiXmlDocument* document = GetDocument();
    value = "";
    p = SkipWhiteSpace(p, encoding);
    if (data) {
        data->Stamp(p, encoding);
        location = data->Cursor();
    }
    const char* startTag = "<!--";
    const char* endTag = "-->";
    if (!StringEqual(p, startTag, false, encoding)) {
        document->SetError(TIXML_ERROR_PARSING_COMMENT, p, data, encoding);
        return 0;
    }
    p += strlen(startTag);
    p = ReadText(p, &value, false, endTag, false, encoding);
    return p;
}

// @ 0x430d30
const char* TiXmlAttribute::Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding)
{
    p = SkipWhiteSpace(p, encoding);
    if (!p || !*p) return 0;

    int tabsize = 4;
    if (document)
        tabsize = document->TabSize();

    if (data) {
        data->Stamp(p, encoding);
        location = data->Cursor();
    }
    const char* pErr = p;
    p = ReadName(p, &name, encoding);
    if (!p || !*p) {
        if (document) document->SetError(TIXML_ERROR_READING_ATTRIBUTES, pErr, data, encoding);
        return 0;
    }
    p = SkipWhiteSpace(p, encoding);
    if (!p || !*p || *p != '=') {
        if (document) document->SetError(TIXML_ERROR_READING_ATTRIBUTES, p, data, encoding);
        return 0;
    }
    ++p;
    p = SkipWhiteSpace(p, encoding);
    if (!p || !*p) {
        if (document) document->SetError(TIXML_ERROR_READING_ATTRIBUTES, p, data, encoding);
        return 0;
    }
    const char* end;
    if (*p == '\'') {
        ++p;
        end = "\'";
        p = ReadText(p, &value, false, end, false, encoding);
    } else if (*p == '"') {
        ++p;
        end = "\"";
        p = ReadText(p, &value, false, end, false, encoding);
    } else {
        value = "";
        while (p && *p
               && !IsWhiteSpace(*p) && *p != '\n' && *p != '\r'
               && *p != '/' && *p != '>') {
            value += *p;
            ++p;
        }
    }
    return p;
}

// @ 0x430eb0
const char* TiXmlText::Parse(const char* p, TiXmlParsingData* data, TiXmlEncoding encoding)
{
    value = "";
    TiXmlDocument* document = GetDocument();
    if (data) {
        data->Stamp(p, encoding);
        location = data->Cursor();
    }
    const char* const startTag = "<![CDATA[";
    const char* const endTag = "]]>";

    if (cdata || StringEqual(p, startTag, false, encoding)) {
        cdata = true;
        if (!StringEqual(p, startTag, false, encoding)) {
            document->SetError(TIXML_ERROR_PARSING_CDATA, p, data, encoding);
            return 0;
        }
        p += strlen(startTag);
        while (p && *p
               && !StringEqual(p, endTag, false, encoding)) {
            value += *p;
            ++p;
        }
        TIXML_STRING dummy;
        p = ReadText(p, &dummy, false, endTag, false, encoding);
        return p;
    } else {
        bool ignoreWhite = true;
        const char* end = "<";
        p = ReadText(p, &value, ignoreWhite, end, false, encoding);
        if (p)
            return p - 1;
        return 0;
    }
}
