#include <utils/StringUtils.h>

#define MAXWIDTH 60

bool StringUtils::replace(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string StringUtils::decodeXMLEscapeLine(std::string xmlString) {
    replace(xmlString, "&quot;", "\"");
    replace(xmlString, "&apos;", "'");
    replace(xmlString, "&lt;", "<");
    replace(xmlString, "&gt;", ">");
    replace(xmlString, "&amp;", "&");
    return xmlString;
}

void StringUtils::splitStringWithNewLines(const std::string &input, std::string &output) {
    for (unsigned i = 0; i < input.length(); i += MAXWIDTH) {
        output = output + input.substr(i, MAXWIDTH) + "\n";
    }
}