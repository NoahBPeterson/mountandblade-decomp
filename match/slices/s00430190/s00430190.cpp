// Slice s00430190: TinyXML 2.5.x tinyxmlparser.cpp -- TiXmlNode::ReadValue, TiXmlBase::GetChar / ReadText,
// TiXmlDocument::Parse.
#include <new>      // declares operator delete[] (cl then frees arrays through ??_V, as the original does)
#include "tinyxml.h"

// @ 0x430190
const char* TiXmlNode::ReadValue(const char* p, TiXmlParsingData* prevData, TiXmlEncoding encoding)
{
    TiXmlDocument* document = GetDocument();
    const char* pWithWhiteSpace = p;
    p = SkipWhiteSpace(p, encoding);

    while (p && *p) {
        if (*p != '<') {
            TiXmlText* textNode = new TiXmlText("");
            if (!textNode) {
                if (document) document->SetError(TIXML_ERROR_OUT_OF_MEMORY, 0, 0, encoding);
                return 0;
            }
            if (TiXmlBase::condenseWhiteSpace) {
                p = textNode->Parse(p, prevData, encoding);
            } else {
                p = textNode->Parse(pWithWhiteSpace, prevData, encoding);
            }
            if (!textNode->Blank())
                LinkEndChild(textNode);
            else
                delete textNode;
        } else {
            if (StringEqual(p, "</", false, encoding)) {
                return p;
            } else {
                TiXmlNode* node = Identify(p, encoding);
                if (node) {
                    p = node->Parse(p, prevData, encoding);
                    LinkEndChild(node);
                } else {
                    return 0;
                }
            }
        }
        pWithWhiteSpace = p;
        p = SkipWhiteSpace(p, encoding);
    }
    if (!p) {
        if (document) document->SetError(TIXML_ERROR_READING_ELEMENT_VALUE, 0, 0, encoding);
    }
    return p;
}

// @ 0x430360
const char* TiXmlBase::GetChar(const char* p, char* _value, int* length, TiXmlEncoding encoding)
{
    if (encoding == TIXML_ENCODING_UTF8) {
        *length = utf8ByteTable[*((unsigned char*)p)];
    } else {
        *length = 1;
    }
    if (*length == 1) {
        if (*p == '&')
            return GetEntity(p, _value, length, encoding);
        *_value = *p;
        return p + 1;
    } else if (*length) {
        for (int i = 0; p[i] && i < *length; ++i) {
            _value[i] = p[i];
        }
        return p + (*length);
    } else {
        return 0;
    }
}

// @ 0x4303f0
const char* TiXmlBase::ReadText(const char* p, TIXML_STRING* text, bool trimWhiteSpace, const char* endTag, bool caseInsensitive, TiXmlEncoding encoding)
{
    *text = "";
    if (!trimWhiteSpace || !condenseWhiteSpace) {
        while (p && *p && !StringEqual(p, endTag, caseInsensitive, encoding)) {
            int len;
            char cArr[4] = { 0, 0, 0, 0 };
            p = GetChar(p, cArr, &len, encoding);
            text->append(cArr, len);
        }
    } else {
        bool whitespace = false;
        p = SkipWhiteSpace(p, encoding);
        while (p && *p && !StringEqual(p, endTag, caseInsensitive, encoding)) {
            if (*p == '\r' || *p == '\n') {
                whitespace = true;
                ++p;
            } else if (IsWhiteSpace(*p)) {
                whitespace = true;
                ++p;
            } else {
                if (whitespace) {
                    (*text) += ' ';
                    whitespace = false;
                }
                int len;
                char cArr[4] = { 0, 0, 0, 0 };
                p = GetChar(p, cArr, &len, encoding);
                if (len == 1)
                    (*text) += cArr[0];
                else
                    text->append(cArr, len);
            }
        }
    }
    return p + strlen(endTag);
}

// @ 0x4305b0
const char* TiXmlDocument::Parse(const char* p, TiXmlParsingData* prevData, TiXmlEncoding encoding)
{
    ClearError();
    if (!p || !*p) {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, 0, 0, TIXML_ENCODING_UNKNOWN);
        return 0;
    }
    location.Clear();
    if (prevData) {
        location.row = prevData->cursor.row;
        location.col = prevData->cursor.col;
    } else {
        location.row = 0;
        location.col = 0;
    }
    TiXmlParsingData data(p, TabSize(), location.row, location.col);
    location = data.Cursor();

    if (encoding == TIXML_ENCODING_UNKNOWN) {
        const unsigned char* pU = (const unsigned char*)p;
        if (*(pU + 0) && *(pU + 0) == TIXML_UTF_LEAD_0
            && *(pU + 1) && *(pU + 1) == TIXML_UTF_LEAD_1
            && *(pU + 2) && *(pU + 2) == TIXML_UTF_LEAD_2) {
            encoding = TIXML_ENCODING_UTF8;
            useMicrosoftBOM = true;
        }
    }

    p = SkipWhiteSpace(p, encoding);
    if (!p) {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, 0, 0, TIXML_ENCODING_UNKNOWN);
        return 0;
    }

    while (p && *p) {
        TiXmlNode* node = Identify(p, encoding);
        if (node) {
            p = node->Parse(p, &data, encoding);
            LinkEndChild(node);
        } else {
            break;
        }
        if (encoding == TIXML_ENCODING_UNKNOWN && node->ToDeclaration()) {
            TiXmlDeclaration* dec = node->ToDeclaration();
            const char* enc = dec->Encoding();
            if (*enc == 0)
                encoding = TIXML_ENCODING_UTF8;
            else if (StringEqual(enc, "UTF-8", true, TIXML_ENCODING_UNKNOWN))
                encoding = TIXML_ENCODING_UTF8;
            else if (StringEqual(enc, "UTF8", true, TIXML_ENCODING_UNKNOWN))
                encoding = TIXML_ENCODING_UTF8;
            else
                encoding = TIXML_ENCODING_LEGACY;
        }
        p = SkipWhiteSpace(p, encoding);
    }

    if (!firstChild) {
        SetError(TIXML_ERROR_DOCUMENT_EMPTY, 0, 0, encoding);
        return 0;
    }
    return p;
}
