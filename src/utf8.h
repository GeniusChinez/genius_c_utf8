#ifndef __GENIUS_C_UTF8__
#define __GENIUS_C_UTF8__

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace genius_c {
    struct InvalidUtf8 : public std::exception {
        InvalidUtf8(const char* msg) 
            : msg(msg) {}

        const char* what() const noexcept { 
            return msg; 
        }

        private:
            const char* msg;
    };

    /*
    ** @brief: Finds the length of a utf8 sequence based on the leading byte.
    ** @param byte: The leading byte in a utf8 sequence.
    ** @returns: The number of trail bytes in the utf8 sequence.
    ** @note: May return 0 if the given byte is not a valid utf8 lead byte.
    ** @note: This works for ascii as well.
    */
    template <typename ByteT>
    inline int getUtf8SequenceLength(ByteT byte) {
        if ((byte & 0x80) == 0) {
            return 1;
        }

        if ((byte & 0xe0) == 0xc0) {
            return 2;
        }

        if ((byte & 0xf0) == 0xe0) {
            return 3;
        }

        if ((byte & 0xf8) == 0xf0) {
            return 4;
        }

        if ((byte & 0xfc) == 0xf8) {
            return 5;
        }

        if ((byte & 0xfe) == 0xfc) {
            return 6;
        }

        return 0;
    }

    /*
    ** @brief: Verifies that the given byte is a leading byte in a valid utf8 
    **    sequence.
    ** @retval true: If the given byte is a leading byte in a valid utf8 
    **    sequence. It returns false otherwise.
    ** @note: This works for ascii as well.
    */
    template <typename ByteT>
    inline bool isValidUtf8LeadByte(ByteT byte) {
        return getUtf8SequenceLength(byte) != 0;
    }

    /*
    ** @brief: Verifies that the given byte is a trailing byte in a valid utf8 
    **    sequence.
    ** @retval true: If the given byte is a trailing byte in a valid utf8 
    **    sequence. It returns false otherwise.
    */
    template <typename ByteT>
    inline bool isValidUtf8TrailByte(ByteT byte) {
        return (byte & 0xc0) == 0x80;
    }

    /*
    ** @brief: Retrieves a single code point from the given utf8 bytestream.
    ** @param octetIterator: A reference to the current octet in a utf8 
    **    sequence.
    **
    ** @param iteratorEnd: A reference to the end of the bytestream.
    ** @returns: the code point parsed from the given bytestream.
    ** 
    ** @throws InvalidUtf8:
    **    If the number of bytes in the utf8 sequence are not as predicted from
    **       the leading byte.
    **    OR If a trailing byte is not a valid utf8 trailing byte (ie. not of 
    **       the form: 10xxxxxx [in binary]).
    **    OR If the leading byte is not a valid utf8 leading byte.
    */
    template <typename OctetIteratorT>
    uint32_t getUtf8Character(
        OctetIteratorT& octetIterator, 
        const OctetIteratorT& iteratorEnd
    ) {
        uint32_t value = 0;
        auto numberOfBytes = getUtf8SequenceLength(*octetIterator);

        if (std::distance(octetIterator, iteratorEnd) < numberOfBytes) {
            throw InvalidUtf8("utf8 sequence too short. Expected more bytes");
        }

        for (int x = 1; x < numberOfBytes; ++x) {
            if (not isValidUtf8TrailByte(octetIterator[x])) {
                throw InvalidUtf8("invalid trailing byte for utf8 sequence");
            }
        }

        switch (numberOfBytes) {
            case 0: {
                throw InvalidUtf8("invalid leading byte for utf8 sequence");
                break;
            }
            case 1: {
                value = *octetIterator++ & 0x7f;
                break;
            }
            case 2: {
                value  = static_cast<uint32_t>((*octetIterator++ & 0x1f)) << 6;
                value |= (*octetIterator++ & 0x3f);
                break;
            }
            case 3: {
                value  = (static_cast<uint32_t>(*octetIterator++ & 0xf))  << 12;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 6;
                value |= (*octetIterator++ & 0x3f);
                break;
            }
            case 4: {
                value  = (static_cast<uint32_t>(*octetIterator++ & 0x7))  << 18;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 12;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 6;
                value |= (*octetIterator++ & 0x3f);
                break;
            }
            case 5: {
                value  = (static_cast<uint32_t>(*octetIterator++ & 0x3))  << 24;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 18;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 12;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 6;
                value |= (*octetIterator++ & 0x3f);
                break;
            }
            case 6: {
                value  = (static_cast<uint32_t>(*octetIterator++ & 0x1))  << 30;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 24;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 18;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 12;
                value |= (static_cast<uint32_t>(*octetIterator++ & 0x3f)) << 6;
                value |= (*octetIterator++ & 0x3f);
                break;
            }
        }

        return value;
    }

    /*
    ** @brief: Appends a single code point to the given utf8 bytestream.
    ** @param octetIterator: A reference to the next iterator position at which 
    **    to insert the next octet.
    */
    template <typename OctetIteratorT>
    void put_utf8_char(
        OctetIteratorT& octetIterator, 
        uint32_t value
    ) {
        if (value < 0x80) {
            *octetIterator++ = value;
            return;
        }

        if (value < 0x800) {
            *octetIterator++ = 0xc0 | ((value >> 6) & 0x1f);
            *octetIterator++ = 0x80 | (value & 0x3f);
            return;
        }

        if (value < 0x10000) {
            *octetIterator++ = 0xe0 | ((value >> 12) & 0xf);
            *octetIterator++ = 0x80 | ((value >> 6) & 0x3f);
            *octetIterator++ = 0x80 | (value & 0x3f);
            return;
        }

        if (value < 0x200000) {
            *octetIterator++ = 0xf0 | ((value >> 18) & 0x7);
            *octetIterator++ = 0x80 | ((value >> 12) & 0x3f);
            *octetIterator++ = 0x80 | ((value >> 6) & 0x3f);
            *octetIterator++ = 0x80 | (value & 0x3f);
            return;
        }

        if (value < 0x4000000) {
            *octetIterator++ = 0xf8 | ((value >> 24) & 0x3);
            *octetIterator++ = 0x80 | ((value >> 18) & 0x3f);
            *octetIterator++ = 0x80 | ((value >> 12) & 0x3f);
            *octetIterator++ = 0x80 | ((value >> 6) & 0x3f);
            *octetIterator++ = 0x80 | (value & 0x3f);
            return;
        }

        *octetIterator++ = 0xfc | ((value >> 30) & 0x1);
        *octetIterator++ = 0x80 | ((value >> 24) & 0x3f);
        *octetIterator++ = 0x80 | ((value >> 18) & 0x3f);
        *octetIterator++ = 0x80 | ((value >> 12) & 0x3f);
        *octetIterator++ = 0x80 | ((value >> 6) & 0x3f);
        *octetIterator++ = 0x80 | (value & 0x3f);
    }

    /*
    ** @brief: Appends the given codePoint to the utf8 container.
    ** @param container: The destination container.
    ** @param codePoint: The codePoint to append to the container.
    ** @note: The type 'ContainerT' must be usable with 'std::back_inserter'.
    */
    template <typename ContainerT>
    void appendUtf8(ContainerT& container, uint32_t codePoint) {
        auto it = std::back_inserter(container);
        put_utf8_char(it, codePoint);
    }

    /*
    ** @brief: Converts the given "std::wstring" instance into a "std::string" 
    **    that is utf8 encoded.
    ** @param wstr: The "std::wstring" instance.
    **/
    std::string convertWStringToUtf8(const std::wstring& wstr) {
        std::string s;
        for (auto ch : wstr) {
            appendUtf8(s, static_cast<uint32_t>(ch));
        }
        return s;
    }

    /*
    ** @brief: Converts the given utf8-encoded bytestream instance into a 
    **    "std::wstring".
    ** @param wstr: The utf8-encoded bytestream.
    ** @note 'BytestreamT' must support 'std::begin(x)' and 'std::end(x)'
    **/
    template <typename BytestreamT>
    std::wstring convertUtf8ToWString(const BytestreamT& str) {
        std::wstring output;
        auto it = str.begin();

        while (it != str.end()) {
            output += static_cast<wchar_t>(getUtf8Character(it, str.end()));
        }

        return output;
    }

} // namespace genius_c

#endif // __GENIUS_C_UTF8__
