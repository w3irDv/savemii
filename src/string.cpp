#include <string.hpp>

bool replace(std::string &str, const std::string &from, const std::string &to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string decodeXMLEscapeLine(std::string xmlString) {
    replace(xmlString, "&quot;", "\"");
    replace(xmlString, "&apos;", "'");
    replace(xmlString, "&lt;", "<");
    replace(xmlString, "&gt;", ">");
    replace(xmlString, "&amp;", "&");
    return xmlString;
}

std::string vformat(const char *format, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int len = std::vsnprintf(nullptr, 0, format, copy);
    va_end(copy);

    if (len >= 0) {
        std::string s(std::size_t(len) + 1, '\0');
        std::vsnprintf(&s[0], s.size(), format, args);
        s.resize(len);
        return s;
    }

    const auto err = errno;
    const auto ec = std::error_code(err, std::generic_category());
    throw std::system_error(ec);
}