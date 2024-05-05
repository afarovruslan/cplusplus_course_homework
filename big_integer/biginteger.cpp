#include "biginteger.h"

BigInteger::BigInteger(int number) : is_positive_(number >= 0) {
  number = abs(number);
  if (number != 0) {
    while (number > 0) {
      digits_.push_back(number % kBase);
      number /= kBase;
    }
  } else {
    digits_.push_back(0);
  }
}

BigInteger::BigInteger(const std::string& string_number)
    : is_positive_(string_number.length() != 0 and string_number[0] != '-') {
  std::string new_digit;
  int lower_bound = 0;
  if (string_number[0] == '-' or string_number[0] == '+') {
    ++lower_bound;
  }
  for (int i = static_cast<int>(string_number.length()) - 1; i >= lower_bound;
       i -= getMaxLengthDigit()) {
    new_digit.clear();
    for (int j = std::max(lower_bound, i - getMaxLengthDigit() + 1); j <= i;
         ++j) {
      new_digit.push_back(string_number[j]);
    }
    digits_.push_back(std::stoi(new_digit));
  }
}

BigInteger::BigInteger() : is_positive_(true) {}

BigInteger& BigInteger::operator+=(const BigInteger& big_integer) {
  if (signum() == big_integer.signum()) {
    size_t ind = 0;
    unsigned int sum_digits = 0;
    size_t start_size = std::max(digits().size(), big_integer.digits().size());
    while (sum_digits != 0 or ind < start_size) {
      if (ind < big_integer.digits().size()) {
        sum_digits += big_integer.digits()[ind];
      }
      if (ind < digits().size()) {
        sum_digits += digits()[ind];
        digits_[ind] = static_cast<int>(sum_digits % kBase);
      } else {
        digits_.push_back(static_cast<int>(sum_digits % kBase));
      }
      sum_digits /= kBase;
      ++ind;
    }
  } else {
    *this -= -big_integer;
  }
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& big_integer) {
  if (big_integer.signum() != signum()) {
    *this += -big_integer;
  } else {
    if (big_integer.signum() < 0) {
      if (big_integer <= *this) {
        *this = ModuloSubtraction(big_integer, *this);
        changeSignum();
      } else {
        *this = ModuloSubtraction(*this, big_integer);
      }
    } else {
      if (big_integer <= *this) {
        *this = ModuloSubtraction(*this, big_integer);
      } else {
        *this = ModuloSubtraction(big_integer, *this);
        changeSignum();
      }
    }
  }
  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& big_integer) {
  BigInteger answer;
  answer.digits().resize(big_integer.digits().size() + digits().size());
  long long next_digit = 0;
  long long new_sum;
  long long now_sum = 0;
  for (size_t i = 0; i < digits().size() + big_integer.digits().size(); ++i) {
    for (size_t j = 0; j <= i; ++j) {
      if (j < digits().size() and i - j < big_integer.digits().size()) {
        new_sum = static_cast<long long>(digits()[j]) *
            static_cast<long long>(big_integer.digits()[i - j]);
        now_sum += new_sum;
        if (now_sum >= getBase()) {
          next_digit += now_sum / getBase();
          now_sum %= getBase();
        }
      }
    }
    answer.digits()[i] = static_cast<int>(now_sum);
    now_sum = next_digit;
    next_digit = 0;
  }
  answer.is_positive_ = signum() * big_integer.signum() > 0;
  *this = answer;
  removeLeadingZeros();
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& big_integer) {
  BigInteger answer{0};
  BigInteger positive_copy = big_integer;
  positive_copy.is_positive_ = true;
  if (!IsLessModulo(*this, big_integer)) {
    BigInteger reduced;
    int ind_now_digit;
    GetMinMore(reduced, *this, big_integer, ind_now_digit);
    answer.digits().resize(ind_now_digit + 2);
    int new_digit;
    while (ind_now_digit >= -1) {
      new_digit = BinSearchDivision(reduced, positive_copy);
      answer.digits()[ind_now_digit + 1] = new_digit;
      reduced -= positive_copy * new_digit;
      if (ind_now_digit >= 0) {
        reduced.addNewDigit(digits()[ind_now_digit]);
      }
      --ind_now_digit;
    }
    answer.is_positive_ = signum() * big_integer.signum() > 0;
  }
  *this = answer;
  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& big_integer) {
  int signum_ans = signum();
  is_positive_ = true;
  BigInteger copy = big_integer;
  copy.is_positive_ = true;
  *this -= copy * (*this / copy);
  is_positive_ = signum_ans > 0;
  return *this;
}

BigInteger BigInteger::operator-() const {
  BigInteger copy = *this;
  copy.changeSignum();
  return copy;
}

BigInteger& BigInteger::operator++() {
  *this += 1;
  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger copy = *this;
  ++*this;
  return copy;
}

BigInteger& BigInteger::operator--() {
  *this -= 1;
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger copy = *this;
  --*this;
  return copy;
}

BigInteger::operator bool() const { return *this != 0; }

std::string BigInteger::toString() const {
  std::string bi_string;
  if (!is_positive_ and !(digits().size() == 1 and digits()[0] == 0)) {
    bi_string += '-';
  }
  std::string str_digit;
  for (size_t i = digits_.size(); i > 0; --i) {
    str_digit = std::to_string(digits_[i - 1]);
    if (i != digits_.size()) {
      AddZeros(str_digit, getMaxLengthDigit());
    }
    bi_string += str_digit;
  }
  return bi_string;
}

int BigInteger::signum() const { return is_positive_ ? 1 : -1; }
const std::vector<int>& BigInteger::digits() const { return digits_; }
std::vector<int>& BigInteger::digits() { return digits_; }
int BigInteger::getBase() { return kBase; }
void BigInteger::changeSignum() { is_positive_ = !is_positive_; }

int BigInteger::getMaxLengthDigit() {
  return static_cast<int>(std::to_string(kBase).length()) - 1;
}

void BigInteger::removeLeadingZeros() {
  while (digits().size() != 1 and digits()[digits().size() - 1] == 0) {
    digits().pop_back();
  }
}

void BigInteger::addNewDigit(int digit) {
  std::vector<int> new_digits(digits().size() + 1, 0);
  new_digits[0] = digit;
  for (size_t i = 0; i < digits().size(); ++i) {
    new_digits[i + 1] = digits()[i];
  }
  digits() = new_digits;
  removeLeadingZeros();
}

BigInteger operator+(BigInteger big_integer1, const BigInteger& big_integer2) {
  big_integer1 += big_integer2;
  return big_integer1;
}

BigInteger operator-(BigInteger big_integer1, const BigInteger& big_integer2) {
  big_integer1 -= big_integer2;
  return big_integer1;
}

BigInteger operator*(BigInteger big_integer1, const BigInteger& big_integer2) {
  big_integer1 *= big_integer2;
  return big_integer1;
}

BigInteger operator/(BigInteger big_integer1, const BigInteger& big_integer2) {
  big_integer1 /= big_integer2;
  return big_integer1;
}

BigInteger operator%(BigInteger big_integer1, const BigInteger& big_integer2) {
  big_integer1 %= big_integer2;
  return big_integer1;
}

bool operator==(const BigInteger& big_integer1,
                const BigInteger& big_integer2) {
  return big_integer1.signum() == big_integer2.signum() and
      big_integer1.digits() == big_integer2.digits();
}

bool operator!=(const BigInteger& big_integer1,
                const BigInteger& big_integer2) {
  return !(big_integer1 == big_integer2);
}

bool operator<(const BigInteger& big_integer1, const BigInteger& big_integer2) {
  if (big_integer1.signum() != big_integer2.signum()) {
    return big_integer1.signum() < big_integer2.signum();
  }
  if (big_integer1.digits().size() != big_integer2.digits().size()) {
    return (big_integer1.signum() < 0 and
        big_integer1.digits().size() > big_integer2.digits().size()) or
        (big_integer1.signum() > 0 and
            big_integer1.digits().size() < big_integer2.digits().size());
  }
  for (int i = static_cast<int>(big_integer1.digits().size()) - 1; i >= 0;
       --i) {
    if (big_integer1.digits()[i] != big_integer2.digits()[i]) {
      return (big_integer1.signum() > 0 and
          big_integer1.digits()[i] < big_integer2.digits()[i]) or
          (big_integer2.signum() < 0 and
              big_integer1.digits()[i] > big_integer2.digits()[i]);
    }
  }
  return false;
}

bool operator>(const BigInteger& big_integer1, const BigInteger& big_integer2) {
  return big_integer2 < big_integer1;
}

bool operator<=(const BigInteger& big_integer1,
                const BigInteger& big_integer2) {
  return !(big_integer1 > big_integer2);
}

bool operator>=(const BigInteger& big_integer1,
                const BigInteger& big_integer2) {
  return !(big_integer1 < big_integer2);
}

std::istream& operator>>(std::istream& flow_in, BigInteger& big_integer) {
  std::string string_big_int;
  flow_in >> string_big_int;
  BigInteger new_bi{string_big_int};
  big_integer = new_bi;
  return flow_in;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& big_integer) {
  out << big_integer.toString();
  return out;
}

BigInteger operator""_bi(unsigned long long number) {
  return {static_cast<int>(number)};
}

BigInteger operator""_bi(const char* c_string, size_t length) {
  std::string string;
  for (size_t i = 0; i < length; ++i) {
    string.push_back(c_string[i]);
  }
  return BigInteger{string};
}

BigInteger ModuloSubtraction(BigInteger bigger_int,
                             const BigInteger& smaller_int) {
  int sum_digits = 0;
  size_t ind = 0;
  while (ind < smaller_int.digits().size() or sum_digits != 0) {
    sum_digits += bigger_int.digits()[ind];
    if (ind < smaller_int.digits().size()) {
      sum_digits -= smaller_int.digits()[ind];
    }
    if (sum_digits < 0) {
      sum_digits += BigInteger::getBase();
      bigger_int.digits()[ind] = sum_digits;
      sum_digits = -1;
    } else {
      bigger_int.digits()[ind] = sum_digits;
      sum_digits = 0;
    }
    ++ind;
  }
  bigger_int.removeLeadingZeros();
  return bigger_int;
}

int BinSearchDivision(const BigInteger& divisor, const BigInteger& divisible) {
  int left = -1;
  int right = BigInteger::getBase();
  int middle;
  while (right - left > 1) {
    middle = (left + right) / 2;
    if (!IsLessModulo(divisor, middle * divisible)) {
      left = middle;
    } else {
      right = middle;
    }
  }
  return left;
}

bool IsLessModulo(const BigInteger& big_integer1,
                  const BigInteger& big_integer2) {
  if (big_integer1.digits().size() >= big_integer2.digits().size()) {
    for (size_t i = big_integer1.digits().size(); i != 0; --i) {
      if (i - 1 >= big_integer2.digits().size() or
          big_integer1.digits()[i - 1] > big_integer2.digits()[i - 1]) {
        return false;
      }
      if (big_integer2.digits()[i - 1] > big_integer1.digits()[i - 1]) {
        return true;
      }
    }
    return false;
  }
  return true;
}

void AddZeros(std::string& string, size_t required_length) {
  std::string zeros;
  for (size_t j = string.length(); j < required_length; ++j) {
    zeros += '0';
  }
  string = zeros + string;
}

void GetMinMore(
    BigInteger& result,
    const BigInteger& big_integer,  // get min number from the beginning of
                                    // big_integer more than smaller_integer
    const BigInteger& smaller_int, int& ind_now_digit) {
  result.digits().resize(smaller_int.digits().size());
  int diff = static_cast<int>(big_integer.digits().size() -
      smaller_int.digits().size());
  std::copy(big_integer.digits().begin() + diff, big_integer.digits().end(),
            result.digits().begin());
  ind_now_digit = diff - 1;
  if (IsLessModulo(result, smaller_int)) {
    result.addNewDigit(big_integer.digits()[ind_now_digit]);
    --ind_now_digit;
  }
}

Rational::Rational(const BigInteger& numerator)
    : numerator_(numerator), denominator_(BigInteger{1}) {}

Rational::Rational(int number)
    : numerator_(BigInteger{number}), denominator_(BigInteger{1}) {}

BigInteger Gcd(BigInteger big_integer1, BigInteger big_integer2) {
  while (big_integer1 != 0 and big_integer2 != 0) {
    if (big_integer1 > big_integer2) {
      big_integer1 %= big_integer2;
    } else {
      big_integer2 %= big_integer1;
    }
  }
  return big_integer1 + big_integer2;
}

Rational& Rational::operator+=(const Rational& fraction) {
  numerator_ =
      numerator_ * fraction.denominator_ + denominator_ * fraction.numerator_;
  denominator_ *= fraction.denominator_;
  correctFractional();
  return *this;
}

Rational& Rational::operator-=(const Rational& fraction) {
  numerator_.changeSignum();
  *this += fraction;
  numerator_.changeSignum();
  return *this;
}

Rational& Rational::operator*=(const Rational& fraction) {
  numerator_ *= fraction.numerator_;
  denominator_ *= fraction.denominator_;
  correctFractional();
  return *this;
}

Rational& Rational::operator/=(const Rational& fraction) {
  numerator_ *= fraction.denominator_;
  denominator_ *= fraction.numerator_;
  correctFractional();
  return *this;
}

Rational Rational::operator-() {
  Rational copy = *this;
  copy.numerator_.changeSignum();
  return copy;
}

void Rational::correctFractional() {
  if (denominator_.signum() == -1) {
    denominator_.changeSignum();
    numerator_.changeSignum();
  }
  BigInteger gcd_num_denom;
  if (numerator_.signum() == -1) {
    numerator_.changeSignum();
    gcd_num_denom = Gcd(numerator_, denominator_);
    numerator_.changeSignum();
  } else {
    gcd_num_denom = Gcd(numerator_, denominator_);
  }
  if (gcd_num_denom != 1) {
    numerator_ /= gcd_num_denom;
    denominator_ /= gcd_num_denom;
  }
}

std::string Rational::toString() const {
  std::string answer = numerator_.toString();
  return denominator_ == 1 ? answer : answer + '/' + denominator_.toString();
}

std::string Rational::asDecimal(size_t precision = 0) const {
  BigInteger integer_part = numerator_ / denominator_;
  BigInteger float_part = numerator_ - denominator_ * integer_part;
  std::string answer;
  if (integer_part == 0 and numerator_ < 0 and precision != 0) {
    answer += "-";
  }
  answer += integer_part.toString();
  if (precision == 0) {
    return answer;
  }
  answer.push_back('.');
  size_t start_length = answer.length();
  std::string new_digits;
  while (answer.length() < precision + start_length) {
    float_part *= BigInteger::getBase();
    new_digits = std::to_string(BinSearchDivision(float_part, denominator_));
    AddZeros(new_digits, BigInteger::getMaxLengthDigit());
    if (answer.length() + new_digits.length() < precision + start_length) {
      answer += new_digits;
    } else {
      for (size_t i = 0; answer.length() < precision + start_length; ++i) {
        answer.push_back(new_digits[i]);
      }
    }
    float_part %= denominator_;
  }
  return answer;
}

Rational::operator double() const {
  return std::stod(asDecimal(kMaximumPrecision));
}

const BigInteger& Rational::getNumerator() const { return numerator_; }
const BigInteger& Rational::getDenominator() const { return denominator_; }

Rational operator+(Rational fraction1, const Rational& fraction2) {
  fraction1 += fraction2;
  return fraction1;
}

Rational operator-(Rational fraction1, const Rational& fraction2) {
  fraction1 -= fraction2;
  return fraction1;
}

Rational operator*(Rational fraction1, const Rational& fraction2) {
  fraction1 *= fraction2;
  return fraction1;
}

Rational operator/(Rational fraction1, const Rational& fraction2) {
  fraction1 /= fraction2;
  return fraction1;
}

bool operator==(const Rational& fraction1, const Rational& fraction2) {
  return fraction1.getNumerator() == fraction2.getNumerator() and
      fraction1.getDenominator() == fraction2.getDenominator();
}

bool operator!=(const Rational& fraction1, const Rational& fraction2) {
  return !(fraction1 == fraction2);
}

bool operator<(const Rational& fraction1, const Rational& fraction2) {
  return fraction1.getNumerator() * fraction2.getDenominator() <
      fraction2.getNumerator() * fraction1.getDenominator();
}

bool operator>(const Rational& fraction1, const Rational& fraction2) {
  return fraction2 < fraction1;
}

bool operator<=(const Rational& fraction1, const Rational& fraction2) {
  return !(fraction1 > fraction2);
}

bool operator>=(const Rational& fraction1, const Rational& fraction2) {
  return !(fraction1 < fraction2);
}

std::ostream& operator<<(std::ostream& out, const Rational& fraction) {
  out << fraction.toString();
  return out;
}