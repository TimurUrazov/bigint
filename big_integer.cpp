#include "big_integer.h"
#include "ci-extra/big_integer_gmp.h"
#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <functional>
#include <iostream>

namespace constants {
    static const big_integer ZERO(0);
    static const big_integer ONE(1);
    static const big_integer TEN(10);
}

namespace binary_ {
    static const std::function<uint32_t(uint32_t const a, uint32_t const b)>
            and_ = [](uint32_t const a, uint32_t const b) { return a & b; },
            or_ = [](uint32_t const a, uint32_t const b) { return a | b; },
            xor_ = [](uint32_t const a, uint32_t const b) { return a ^ b; };
}

big_integer::big_integer() :
        number(1),
        sign (false) {}

big_integer::big_integer(int a) :
        number(1, static_cast<uint32_t>(std::abs(static_cast<int64_t>(a)))),
        sign (a < 0) {}

big_integer::big_integer(unsigned int a) :
        number(1, static_cast<uint32_t>(a)),
        sign (false) {}

big_integer::big_integer(long a) :
        big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(long long a) :
        number(0),
        sign (a < 0) {
    if (a != std::numeric_limits<long>::min()) {
        a = std::abs(a);
    }
    auto num = static_cast<uint64_t>(a);
    number.push_back(static_cast<uint32_t>(a) & UINT32_MAX);
    num >>= 32u;
    if (num > 0) {
        number.push_back(static_cast<uint32_t>(num));
    }
}

big_integer::big_integer(unsigned long a) :
        big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(unsigned long long a) :
        number(0),
        sign (false) {
    a = static_cast<uint64_t>(a);
    number.push_back(static_cast<uint32_t>(a & UINT32_MAX));
    a >>= 32u;
    if (a > 0) {
        number.push_back(static_cast<uint32_t>(a));
    }
}

big_integer::big_integer(std::string const& str) :
        big_integer() {
    size_t str_size = str.size();
    size_t i = 0;
    if (str_size == 0) {
        throw std::invalid_argument("Empty string");
    }
    if (str[i] == '-' || str[i] == '+') {
        if (str_size == 1) {
            std::string s = "Can not resolve ";
            s += str[i];
            throw std::invalid_argument(s);
        } else {
            i++;
        }
    }
    while (i < str_size && str[i] == '0') {
        i++;
    }
    if (i == str_size) {
        return;
    }
    for (size_t j = i; j < str_size; j++) {
        if (!isdigit(str[j])) {
            throw std::invalid_argument("Is not a number");
        }
        *this *= constants::TEN;
        *this += big_integer(str[j] - '0');
    }
    sign = (str[0] == '-');
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    if (sign ^ rhs.sign) {
        *this = sign ? rhs - (-*this) : *this - (-rhs);
        return *this;
    }
    size_t a_size = number.size();
    size_t b_size = rhs.number.size();
    size_t sz = std::max(a_size, b_size);
    size_t i = 0;
    uint64_t num;
    bool carry = false;
    sign = rhs.sign;
    number.resize(sz);
    for (; i < sz; ++i) {
        num = (carry ? 1 : 0);
        if (i < a_size) {
            num += number[i];
        }
        if (i < b_size) {
            num += rhs.number[i];
        }
        carry = num > UINT32_MAX;
        number[i] = static_cast<uint32_t>(num) & UINT32_MAX;
    }
    if (carry) {
        number.push_back(1);
    }
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    if (sign && rhs.sign) {
        *this = (-rhs) - (-*this);
        return *this;
    }
    if (sign ^ rhs.sign) {
        *this = sign ? -(-*this + rhs) : -rhs + *this;
        return *this;
    }
    if (*this < rhs) {
        *this = -(rhs - *this);
        return *this;
    }
    size_t i = 0;
    uint32_t carry = 0;
    number.resize(number.size());
    for (; i < rhs.number.size(); i++) {
        int64_t a = number[i];
        int64_t b = rhs.number[i];
        int64_t r = a - b;
        number[i] = number[i] - rhs.number[i] - carry;
        carry = r >= carry ? r >= 0 ? 0 : 1 : 1;
    }
    while (i < number.size()) {
        int64_t r = number[i];
        number[i] = number[i] - carry >= 0 ? number[i] - carry : UINT32_MAX;
        carry = r < carry ? 1 : 0;
        i++;
    }
    delete_zero();
    return *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    if (*this == constants::ZERO || rhs == constants::ZERO) {
        *this = 0;
        return *this;
    }
    size_t a_size = number.size();
    size_t b_size = rhs.number.size();
    sign = sign ^ rhs.sign;
    number.resize(a_size + b_size);
    uint64_t carry = 0;
    std::vector<uint32_t> cur(a_size + b_size);
    for (size_t i = 0; i != a_size; i++) {
        for (size_t shift = 0; shift != b_size; ++shift) {
            uint64_t num = static_cast<uint64_t>(number[i]) * static_cast<uint64_t>(rhs.number[shift]) + static_cast<uint64_t>(cur[i + shift]) + carry;
            cur[i + shift] = static_cast<uint32_t>(num & UINT32_MAX);
            carry = num >> 32u;
        }
        cur[i + b_size] += static_cast<uint32_t>(carry);
        carry = 0;
    }
    number = cur;
    delete_zero();
    return *this;
}

big_integer big_integer::divide_long_short(uint32_t short_num) {
    uint32_t carry = 0;
    uint64_t tmp;
    for (size_t i = number.size(); i != 0; --i) {
        tmp = (static_cast<uint64_t>(carry) << 32u) + number[i - 1];
        number[i - 1] = static_cast<uint32_t>(tmp / static_cast<uint64_t>(short_num)) & UINT32_MAX;
        carry = static_cast<uint32_t>(tmp % static_cast<uint64_t>(short_num));
    }
    delete_zero();
    return carry;
}

std::pair<big_integer&, big_integer> big_integer::div_mod(big_integer const& rhs) {
    if (*this == constants::ZERO || (*this < rhs && !sign) || (*this > rhs && sign)) {
        big_integer copy(*this);
        *this = 0;
        return {*this, copy};
    }
    if (sign ^ rhs.sign) {
        const bool s = sign;
        std::pair<big_integer& , big_integer> bi = div_mod(-rhs);
        if (s && bi.second != constants::ZERO) {
            bi.second.sign = true;
        }
        *this = -bi.first;
        return {*this, bi.second};
    }
    if (rhs.number.size() == 1) {
        sign = false;
        big_integer mod = this->divide_long_short(rhs.number.front());
        return {*this, mod};
    }
    big_integer bi(rhs);
    bool rem_sign = sign && bi.sign;
    sign = false;
    bi.sign = false;
    auto f = static_cast<uint32_t>((static_cast<uint64_t>(1) << 32u) / (bi.number.back() + 1));
    *this *= f;
    bi *= f;
    size_t km = number.size() - 1;
    size_t delta = number.size() - bi.number.size();
    size_t i = 0, j = delta - 1;
    std::vector<uint32_t> current;
    for (; i != delta && *this != 0; ++i, --j) {
        big_integer dif = (bi << 32u * static_cast<uint32_t>(j + 1));
        if (*this >= dif) {
            current.push_back(1);
            *this -= dif;
        }
        uint64_t tmp = (km - i < number.size() ? (static_cast<uint64_t>(number[km - i]) << 32u) : 0) +
                       (km - i - 1 < number.size() ? static_cast<uint64_t>(number[km - i - 1]) : 0);
        if (tmp == 0) {
            current.push_back(0);
            continue;
        }
        uint64_t quotient = tmp / bi.number.back();
        big_integer qt = quotient > UINT32_MAX ? UINT32_MAX : quotient & UINT32_MAX;;
        dif = (qt * bi) << (32u * static_cast<uint32_t>(j));
        if (*this < dif) {
            qt--;
            dif -= (bi << (32u * static_cast<uint32_t>(j)));
        }
        *this -= dif;
        current.push_back(qt.number.front());
    }
    big_integer mod = *this / f;
    mod.sign = rem_sign;
    while (i++ < delta) {
        current.push_back(0);
    }
    std::reverse(current.begin(), current.end());
    number = current;
    delete_zero();
    return {*this, mod};
}

big_integer& big_integer::operator/=(big_integer const& rhs) {
    return div_mod(rhs).first;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    return *this = div_mod(rhs).second;
}

big_integer big_integer::binary(big_integer b, std::function<uint32_t(const uint32_t, const uint32_t)> const& function) {
    size_t sz = std::max(number.size(), b.number.size());
    is_negate(sz);
    b.is_negate(sz);
    sign = function(sign, b.sign);
    for (size_t i = 0; i < sz; i++) {
        number[i] = function(number[i], b.number[i]);
    }
    delete_zero();
    if (sign) {

        for (size_t i = 0; i < sz; i++) {
            number[i] = ~number[i];
        }
        operator--();
    }
    return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
    this->binary(rhs, binary_::and_);
    return *this;
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
    this->binary(rhs, binary_::or_);
    return *this;
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
    this->binary(rhs, binary_::xor_);
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    if (rhs == 0) {
        return *this;
    }
    auto shift = static_cast<size_t>(rhs / 32u);
    uint32_t inner = rhs % 32u;
    size_t sz = number.size() + shift + 1;
    std::vector<uint32_t> res(sz);
    res[shift] = static_cast<uint32_t>(static_cast<uint64_t>(number[0]) << inner);
    for (size_t i = shift + 1; i < sz; ++i) {
        uint64_t x = i - shift < number.size() ? static_cast<uint64_t>(number[i - shift]) << inner : 0;
        uint64_t y = i - shift - 1 < number.size() ? static_cast<uint64_t>(number[i - shift - 1]) >> (32u - inner) : 0;
        res[i] = static_cast<uint32_t>((x + y) & UINT32_MAX);
    }
    number = res;
    delete_zero();
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    if (rhs == 0) {
        return *this;
    }
    auto shift = static_cast<size_t>(rhs / 32u);
    uint32_t inner = (rhs % 32u);
    size_t sz = shift < number.size() ? number.size() - shift: 0;
    bool tmp_sign = sign;
    sign = false;
    std::vector<uint32_t> res(sz);
    for (size_t i = 0; i < sz; ++i) {
        uint64_t x = i + shift < number.size() ? static_cast<uint64_t>(number[i + shift]) >> inner : 0;
        uint64_t y = i + shift + 1 < number.size() ? static_cast<uint64_t>(number[i + shift + 1]) << (32u - inner) : 0;
        res[i] = static_cast<uint32_t>((x + y) & UINT32_MAX);
    }
    number = res;
    delete_zero();
    if (tmp_sign) {
        ++*this;
    }
    sign = tmp_sign;
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    big_integer copy(*this);
    if (copy != constants::ZERO) {
        copy.sign = !copy.sign;
    }
    return copy;
}

big_integer big_integer::operator~() const {
    return -(*this + constants::ONE);
}

void big_integer::delete_zero() {
    while (!number.empty() && number.back() == 0) {
        number.pop_back();
    }
    if (number.empty()) {
        sign = false;
        number.push_back(0);
    }
}

big_integer operator+(big_integer a, big_integer const& b) {
    a += b;
    return a;
}

big_integer operator-(big_integer a, const big_integer& b) {
    a -= b;
    return a;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, const big_integer& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator>>(big_integer a, uint32_t b) {
    return a >>= b;
}

big_integer operator<<(big_integer a, uint32_t b) {
    return a <<= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return a.sign == b.sign && a.number == b.number;
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.sign != b.sign) {
        return a.sign;
    }
    if (a.number.size() != b.number.size()) {
        return a.number.size() > b.number.size() ? a.sign : !a.sign;
    }
    for (size_t i = a.number.size(); i != 0; --i) {
        if (a.number[i - 1] != b.number[i - 1]) {
            return a.number[i - 1] < b.number[i - 1] ? !a.sign : a.sign;
        }
    }
    return false;
}

bool operator>(big_integer const& a, big_integer const& b) {
    return b < a;
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

big_integer big_integer::is_negate(size_t sz) {
    size_t i = 0;
    if (sign) {
        (*this)++;
        for (;i < number.size(); i++){
            number[i] = ~number[i];
        }
    }
    for (;i < sz; i++) {
        number.push_back(sign ? UINT32_MAX : 0);
    }
    return *this;
}

big_integer operator&(big_integer a, const big_integer& b) {
    return a &= b;
}

big_integer operator|(big_integer a, const big_integer& b) {
    return a |= b;
}

big_integer operator^(big_integer a, const big_integer& b) {
    return a ^= b;
}

std::string to_string(big_integer const& a) {
    if (a == constants::ZERO) {
        return "0";
    }
    std::string str;
    big_integer copy(a);
    while (copy != 0) {
        uint32_t digit = (copy % constants::TEN).number.front();
        str += static_cast<char>(digit + '0');
        copy /= constants::TEN;
    }
    if (a.sign) {
        str += "-";
    }
    std::reverse(str.begin(), str.end());
    return str;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}

big_integer& big_integer::operator++() {
    *this += constants::ONE;
    return *this;
}

big_integer big_integer::operator++(int) {
    big_integer copy(*this);
    *this += constants::ONE;
    return copy;
}

big_integer& big_integer::operator--() {
    *this -= constants::ONE;
    return *this;
}

big_integer big_integer::operator--(int) {
    big_integer copy(*this);
    *this -= constants::ONE;
    return copy;
}

void big_integer::swap(big_integer& other) {
    std::swap(sign, other.sign);
    std::swap(number, other.number);
}
