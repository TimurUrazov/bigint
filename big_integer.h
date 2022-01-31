#pragma once

#include <iosfwd>
#include <vector>
#include <string>
#include <functional>

struct big_integer {
public:
    big_integer();
    big_integer(int a);
    big_integer(long long a);
    big_integer(unsigned long long a);
    big_integer(unsigned int a);
    big_integer(long a);
    big_integer(unsigned long a);
    explicit big_integer(std::string const& str);
    big_integer(big_integer const& other) = default;
    ~big_integer() = default;

    big_integer& operator=(big_integer const& other) = default;

    big_integer& operator+=(big_integer const& rhs);
    big_integer& operator-=(big_integer const& rhs);
    big_integer& operator*=(big_integer const& rhs);
    big_integer& operator/=(big_integer const& rhs);
    big_integer& operator%=(big_integer const& rhs);

    big_integer& operator&=(const big_integer& rhs);
    big_integer& operator|=(const big_integer& rhs);
    big_integer& operator^=(const big_integer& rhs);

    big_integer& operator<<=(int rhs);
    big_integer& operator>>=(int rhs);

    big_integer operator+() const;
    big_integer operator-() const;
    big_integer operator~() const;

    big_integer& operator++();
    big_integer operator++(int);

    big_integer& operator--();
    big_integer operator--(int);

    friend bool operator==(big_integer const& a, big_integer const& b);
    friend bool operator!=(big_integer const& a, big_integer const& b);
    friend bool operator<(big_integer const& a, big_integer const& b);
    friend bool operator>(big_integer const& a, big_integer const& b);
    friend bool operator<=(big_integer const& a, big_integer const& b);
    friend bool operator>=(big_integer const& a, big_integer const& b);

    friend std::string to_string(big_integer const& a);

private:
    big_integer binary(big_integer b, std::function<uint32_t(const uint32_t, const uint32_t)> const& function);
    big_integer is_negate(size_t sz);

    big_integer divide_long_short(uint32_t short_num);
    std::pair<big_integer&, big_integer> div_mod(big_integer const& rhs);
    void swap(big_integer& other);
    void delete_zero();

    std::vector<uint32_t> number;
    bool sign;
};

big_integer operator+(big_integer a, const big_integer& b);
big_integer operator-(big_integer a, const big_integer& b);
big_integer operator*(big_integer a, const big_integer& b);
big_integer operator/(big_integer a, const big_integer& b);
big_integer operator%(big_integer a, const big_integer& b);

big_integer operator&(big_integer a, const big_integer& b);
big_integer operator|(big_integer a, const big_integer& b);
big_integer operator^(big_integer a, const big_integer& b);

big_integer operator<<(big_integer a, uint32_t b);
big_integer operator>>(big_integer a, uint32_t b);

bool operator==(big_integer const& a, big_integer const& b);
bool operator!=(big_integer const& a, big_integer const& b);
bool operator<(big_integer const& a, big_integer const& b);
bool operator>(big_integer const& a, big_integer const& b);
bool operator<=(big_integer const& a, big_integer const& b);
bool operator>=(big_integer const& a, big_integer const& b);

std::string to_string(big_integer const& a);
std::ostream& operator<<(std::ostream& s, big_integer const& a);