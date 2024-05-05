#include "string.h"

String::String(const char* c_string)
    : length_(strlen(c_string)),
      capacity_(length_),
      string_(new char[capacity_ + 1]) {
  std::copy(c_string, c_string + capacity_ + 1, string_);
}

String::String(size_t n, char symbol)
    : length_(n), capacity_(n), string_(new char[capacity_ + 1]) {
  std::fill(string_, string_ + n, symbol);
  string_[n] = '\0';
}

String::String() : length_(0), capacity_(0), string_(new char[1]) {
  string_[0] = '\0';
}

String::String(const String& string)
    : length_(string.length()),
      capacity_(length_),
      string_(new char[capacity_ + 1]) {
  std::copy(string.data(), string.data() + capacity_ + 1, data());
}

String::String(char symbol)
    : length_(1), capacity_(length_), string_(new char[capacity_ + 1]) {
  string_[0] = symbol;
  string_[1] = '\0';
}

String::~String() { delete[] string_; }

String& String::operator=(String string) {
  swap(string);
  return *this;
}

void String::push_back(char symbol) {
  if (capacity_ == length_) {
    reserve(std::max(2 * capacity_, static_cast<size_t>(1)));
  }
  string_[length_] = symbol;
  string_[++length_] = '\0';
}

String& String::operator+=(const String& string) {
  if (capacity_ < string.length_ + length_) {
    reserve(std::max(2 * capacity_, string.length_ + length_));
  }
  std::copy(string.string_, string.string_ + string.length_ + 1,
            string_ + length_);
  length_ += string.length_;
  return *this;
}

String& String::operator+=(char symbol) {
  push_back(symbol);
  return *this;
}

size_t String::find(const String& substring) const {
  for (size_t i = 0; i + substring.length_ <= length_; ++i) {
    if (std::strncmp(string_ + i, substring.string_, substring.length_) == 0) {
      return i;
    }
  }
  return length();
}

size_t String::rfind(const String& substring) const {
  if (length_ != 0) {
    for (size_t i = length_ - 1; i + 1 >= substring.length_; --i) {
      if (std::strncmp(string_ + i - substring.length_ + 1, substring.string_,
                       substring.length_) == 0) {
        return i - substring.length_ + 1;
      }
    }
  }
  return length();
}

String String::substr(size_t start, size_t count) const {
  String substr(count, '\0');
  std::copy(string_ + start, string_ + start + count, substr.string_);
  return substr;
}

void String::clear() {
  length_ = 0;
  string_[0] = '\0';
}

void String::shrink_to_fit() {
  String new_string(*this);
  if (capacity_ > length_) {
    swap(new_string);
  }
}

void String::swap(String& string) {
  std::swap(string_, string.string_);
  std::swap(length_, string.length_);
  std::swap(capacity_, string.capacity_);
}

void String::reserve(size_t new_size) {
  capacity_ = new_size;
  char* new_memory = new char[capacity_ + 1];
  std::copy(string_, string_ + length_ + 1, new_memory);
  delete[] string_;
  string_ = new_memory;
}

bool operator==(const String& string1, const String& string2) {
  if (string1.length() == string2.length()) {
    return strncmp(string1.data(), string2.data(), string1.length()) == 0;
  }
  return false;
}

bool operator!=(const String& string1, const String& string2) {
  return !(string1 == string2);
}

bool operator<(const String& string1, const String& string2) {
  size_t ind = 0;
  while (ind < std::min(string1.length(), string2.length()) and
         string1[ind] == string2[ind]) {
    ++ind;
  }
  if (ind == std::min(string1.length(), string2.length())) {
    return string1.length() < string2.length();
  }
  return string1[ind] < string2[ind];
}

bool operator>(const String& string1, const String& string2) {
  return string2 < string1;
}

bool operator<=(const String& string1, const String& string2) {
  return !(string1 > string2);
}

bool operator>=(const String& string1, const String& string2) {
  return !(string1 < string2);
}

String operator+(String string1, const String& string2) {
  string1 += string2;
  return string1;
}

std::ostream& operator<<(std::ostream& out, const String& string) {
  for (size_t i = 0; i < string.length(); ++i) {
    out << string[i];
  }
  return out;
}

std::istream& operator>>(std::istream& flow_in, String& string) {
  string.clear();
  char symbol;
  flow_in.get(symbol);
  while (!flow_in.eof() and std::isspace(symbol) != 0) {
    flow_in.get(symbol);
  }
  while (!flow_in.eof() and std::isspace(symbol) == 0) {
    string.push_back(symbol);
    flow_in.get(symbol);
  }
  return flow_in;
}