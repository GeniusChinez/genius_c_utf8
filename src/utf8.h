#ifndef __GENIUS_C_UTF8__
#define __GENIUS_C_UTF8__

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace genius_c {
    struct invalid_utf8 : public std::exception {
        invalid_utf8(const char* msg) 
            : msg(msg) {}

        const char* what() const { 
            return msg; 
        }

        private:
            const char* msg;
    };

    /*
    ** @brief Finds the length of a utf8 sequence based on the leading byte.
    ** @param byte The leading byte in a utf8 sequence.
    ** @return The number of trail bytes in the utf8 sequence.
    ** @note May return 0 if the given byte is not a valid utf8 lead byte.
    ** @note This works for ascii as well.
    */
    template <typename byte_type>
    inline int get_utf8_sequence_length(byte_type byte) {
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
    ** @brief Verifies that the given byte is a leading byte in a valid utf8 
    ** ... sequence.
    ** @returns true If the given byte is a leading byte in a valid utf8 
    ** ... sequence. It returns false otherwise.
    ** @note This works for ascii as well.
    */
    template <typename byte_type>
    inline bool is_valid_utf8_lead_byte(byte_type byte) {
        return get_utf8_sequence_length(byte) != 0;
    }

    /*
    ** @brief Verifies that the given byte is a trailing byte in a valid utf8 
    ** ... sequence.
    ** @returns true If the given byte is a trailing byte in a valid utf8 
    ** ... sequence. It returns false otherwise.
    */
    template <typename byte_type>
    inline bool is_valid_utf8_trail_byte(byte_type byte) {
        return (byte & 0xc0) == 0x80;
    }

    /*
    ** @brief Retrieves a single code point from the given utf8 bytestream.
    ** @param oct_it A reference to the current octet in a utf8 sequence.
    ** @param it_end A reference to the end of the bytestream.
    ** @returns the code point parsed from the given bytestream.
    ** 
    ** @throw invalid_utf8
    ** ... If the number of bytes in the utf8 sequence are not as predicted from
    ** ....... the leading byte.
    ** ... OR If a trailing byte is not a valid utf8 trailing byte (ie. not of 
    ** ....... the form: 10xxxxxx [in binary]).
    ** ... OR If the leading byte is not a valid utf8 leading byte.
    */
    template <typename octet_iterator>
    uint32_t get_utf8_char(
        octet_iterator& oct_it, 
        const octet_iterator& it_end
    ) {
        uint32_t value = 0;
        auto number_of_bytes = get_utf8_sequence_length(*oct_it);

        if (std::distance(oct_it, it_end) < number_of_bytes) {
            throw invalid_utf8("utf8 sequence too short. Expected more bytes");
        }

        for (int x = 1; x < number_of_bytes; ++x) {
            if (not is_valid_utf8_trail_byte(oct_it[x])) {
                throw invalid_utf8("invalid trailing byte for utf8 sequence");
            }
        }

        switch (number_of_bytes) {
            case 0: {
                throw invalid_utf8("invalid leading byte for utf8 sequence");
                break;
            }
            case 1: {
                value = *oct_it++ & 0x7f;
                break;
            }
            case 2: {
                value  = static_cast<uint32_t>((*oct_it++ & 0x1f)) << 6;
                value |= (*oct_it++ & 0x3f);
                break;
            }
            case 3: {
                value  = (static_cast<uint32_t>(*oct_it++ & 0xf))  << 12;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 6;
                value |= (*oct_it++ & 0x3f);
                break;
            }
            case 4: {
                value  = (static_cast<uint32_t>(*oct_it++ & 0x7))  << 18;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 12;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 6;
                value |= (*oct_it++ & 0x3f);
                break;
            }
            case 5: {
                value  = (static_cast<uint32_t>(*oct_it++ & 0x3))  << 24;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 18;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 12;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 6;
                value |= (*oct_it++ & 0x3f);
                break;
            }
            case 6: {
                value  = (static_cast<uint32_t>(*oct_it++ & 0x1))  << 30;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 24;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 18;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 12;
                value |= (static_cast<uint32_t>(*oct_it++ & 0x3f)) << 6;
                value |= (*oct_it++ & 0x3f);
                break;
            }
        }

        return value;
    }

    /*
    ** @brief Appends a single code point to the given utf8 bytestream.
    ** @param oct_it A reference to the next iterator position at which to 
    ** ... insert the next octet.
    */
    template <typename octet_iterator>
    void put_utf8_char(
        octet_iterator& oct_it, 
        uint32_t value
    ) {
        if (value < 0x80) {
            *oct_it++ = value;
            return;
        }

        if (value < 0x800) {
            *oct_it++ = 0xc0 | ((value >> 6) & 0x1f);
            *oct_it++ = 0x80 | (value & 0x3f);
            return;
        }

        if (value < 0x10000) {
            *oct_it++ = 0xe0 | ((value >> 12) & 0xf);
            *oct_it++ = 0x80 | ((value >> 6) & 0x3f);
            *oct_it++ = 0x80 | (value & 0x3f);
            return;
        }

        if (value < 0x200000) {
            *oct_it++ = 0xf0 | ((value >> 18) & 0x7);
            *oct_it++ = 0x80 | ((value >> 12) & 0x3f);
            *oct_it++ = 0x80 | ((value >> 6) & 0x3f);
            *oct_it++ = 0x80 | (value & 0x3f);
            return;
        }

        if (value < 0x4000000) {
            *oct_it++ = 0xf8 | ((value >> 24) & 0x3);
            *oct_it++ = 0x80 | ((value >> 18) & 0x3f);
            *oct_it++ = 0x80 | ((value >> 12) & 0x3f);
            *oct_it++ = 0x80 | ((value >> 6) & 0x3f);
            *oct_it++ = 0x80 | (value & 0x3f);
            return;
        }

        *oct_it++ = 0xfc | ((value >> 30) & 0x1);
        *oct_it++ = 0x80 | ((value >> 24) & 0x3f);
        *oct_it++ = 0x80 | ((value >> 18) & 0x3f);
        *oct_it++ = 0x80 | ((value >> 12) & 0x3f);
        *oct_it++ = 0x80 | ((value >> 6) & 0x3f);
        *oct_it++ = 0x80 | (value & 0x3f);
    }

    /*
    ** @brief Appends the given codepoint to the utf8 container.
    ** @param container The destination container.
    ** @param codepoint The codepoint to append to the container.
    ** @note The type 'container_type' must be usable with 'std::back_inserter'.
    */
    template <typename container_type>
    void append_utf8(container_type& container, uint32_t codepoint) {
        auto it = std::back_inserter(container);
        put_utf8_char(it, codepoint);
    }

    /*
    ** @brief Converts the given "std::wstring" instance into a "std::string" 
    ** ... that is utf8 encoded.
    ** @param wstr The "std::wstring" instance.
    **/
    std::string wstring_to_utf8(const std::wstring& wstr) {
        std::string s;
        for (auto ch : wstr) {
            append_utf8(s, static_cast<uint32_t>(ch));
        }
        return s;
    }

    /*
    ** @brief Converts the given utf8-encoded bytestream instance into a 
    ** ... "std::wstring".
    ** @param wstr The utf8-encoded bytestream.
    ** @note 'bytestream_type' must support 'std::begin(x)' and 'std::end(x)'
    **/
    template <typename bytestream_type>
    std::wstring utf8_to_wstring(const bytestream_type& str) {
        std::wstring output;
        auto it = str.begin();

        while (it != str.end()) {
            output += static_cast<wchar_t>(get_utf8_char(it, str.end()));
        }

        return output;
    }

} // namespace genius_c::utf8

#endif // __GENIUS_C_UTF8__