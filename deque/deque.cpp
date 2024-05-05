#include "deque.h"

int DivisionUp(int divisible, int divider) {
  return (divisible + divider - 1) / divider;
}

template <typename T>
T* Deque<T>::alloc(size_t size) {
  return reinterpret_cast<T*>(new char[size * sizeof(T)]);
}

template <typename T>
void Deque<T>::dealloc(T* arr) { delete[] reinterpret_cast<char*>(arr); }

template <typename T>
std::vector<T*> Deque<T>::reserve(size_t new_cap) {
  for (int i = 0; i < ptr_first_array_; ++i) {
    dealloc(vector_arrays_[i]);
  }
  for (int i = ptr_last_array_ + 1; i < static_cast<int>(capacity_); ++i) {
    dealloc(vector_arrays_[i]);
  }
  std::vector<T*> new_vector_arrays(new_cap);
  size_t free_space = new_cap - (ptr_last_array_ - ptr_first_array_ + 1);
  size_t free_space_before_main = free_space / 2;
  size_t free_space_after_main = free_space - free_space_before_main;
  for (size_t i = 0; i < new_cap; ++i) {
    if (i >= free_space_before_main and i < new_cap - free_space_after_main and
        size_ != 0) {
      new_vector_arrays[i] =
          vector_arrays_[ptr_first_array_ + i - free_space_before_main];
    } else {
      new_vector_arrays[i] = alloc(kSizeArray);
    }
  }
  ptr_first_array_ = static_cast<int>(free_space_before_main);
  ptr_last_array_ = static_cast<int>(new_cap - free_space_after_main - 1);
  capacity_ = new_cap;
  std::swap(new_vector_arrays, vector_arrays_);
  return new_vector_arrays;
}

template <typename T>
void Deque<T>::unreserve(std::vector<T*>& reserved_vector,
                         int old_ptr_first_arr) {
  std::swap(reserved_vector, vector_arrays_);
  for (int i = 0; i < ptr_first_array_; ++i) {
    dealloc(reserved_vector[i]);
  }
  for (int i = ptr_last_array_ + 1; i < static_cast<int>(capacity_); ++i) {
    dealloc(reserved_vector[i]);
  }
  ptr_first_array_ = old_ptr_first_arr;
  capacity_ = vector_arrays_.size();
  ptr_last_array_ = ptr_first_array_ + capacity_ - 1;
  for (int i = 0; i < ptr_first_array_; ++i) {
    vector_arrays_[i] = alloc(kSizeArray);
  }
  for (int i = ptr_last_array_ + 1; i < static_cast<int>(capacity_); ++i) {
    vector_arrays_[i] = alloc(kSizeArray);
  }
}

template <typename T>
void Deque<T>::increment_ptr(int& ptr_arr, int& ptr_elem) {
  ptr_arr += (++ptr_elem / static_cast<int>(kSizeArray));
  ptr_elem %= static_cast<int>(kSizeArray);
}

template <typename T>
void Deque<T>::decrement_ptr(int& ptr_arr, int& ptr_elem) {
  if (ptr_elem == 0) {
    --ptr_arr;
  }
  ptr_elem = (static_cast<int>(kSizeArray) + ptr_elem - 1) %
      static_cast<int>(kSizeArray);
}

template <typename T>
void Deque<T>::push(const T& value, std::pair<int&, int&> ptr,
                    void (*change_ptr)(int&, int&),
                    void (*undo_change_ptr)(int&, int&), bool reserved) {
  int& ptr_arr = ptr.first;
  int& ptr_elem = ptr.second;
  std::vector<T*> new_vector_arrays;
  int old_ptr_first_array = ptr_first_array_;
  if (reserved) {
    new_vector_arrays =
        reserve(std::max(2 * capacity_, static_cast<size_t>(3)));
  }
  try {
    change_ptr(ptr_arr, ptr_elem);
    new (vector_arrays_[ptr_arr] + ptr_elem) T(value);
    ++size_;
  } catch (...) {
    undo_change_ptr(ptr_arr, ptr_elem);
    if (reserved) {
      unreserve(new_vector_arrays, old_ptr_first_array);
    }
    throw;
  }
}

template <typename T>
void Deque<T>::free_memory() {
  for (auto iterator = begin(); iterator != end(); ++iterator) {
    iterator->~T();
  }
  for (size_t i = 0; i < capacity_; ++i) {
    dealloc(vector_arrays_[i]);
  }
}

template <typename T>
void Deque<T>::alloc_arrays() {
  for (size_t i = 0; i < capacity_; ++i) {
    vector_arrays_[i] = alloc(kSizeArray);
  }
}

template <typename T>
Deque<T>::Deque()
    : size_(0),
      capacity_(0),
      ptr_first_array_(0),
      ptr_last_array_(-1),
      ptr_first_elem_(0),
      ptr_last_elem_(static_cast<int>(kSizeArray - 1)) {}

template <typename T>
Deque<T>::Deque(int size) : Deque(size, T()) {}

template <typename T>
Deque<T>::Deque(int size, const T& value)
    : size_(static_cast<size_t>(size)),
      capacity_(DivisionUp(size_, kSizeArray)),
      ptr_first_array_(0),
      ptr_last_array_(capacity_ - 1),
      ptr_first_elem_(0),
      ptr_last_elem_((size - 1) % static_cast<int>(kSizeArray)),
      vector_arrays_(capacity_) {
  alloc_arrays();
  iterator creating_it = begin();
  try {
    for (; creating_it != end(); ++creating_it) {
      new (creating_it.operator->()) T(value);
    }
  } catch (...) {
    for (iterator destroying_it = begin(); destroying_it != creating_it;
         ++destroying_it) {
      destroying_it->~T();
    }
    for (size_t i = 0; i < capacity_; ++i) {
      dealloc(vector_arrays_[i]);
    }
    throw;
  }
}

template <typename T>
Deque<T>::Deque(const Deque<T>& deque)
    : size_(deque.size_),
      capacity_(deque.capacity_),
      ptr_first_array_(deque.ptr_first_array_),
      ptr_last_array_(deque.ptr_last_array_),
      ptr_first_elem_(deque.ptr_first_elem_),
      ptr_last_elem_(deque.ptr_last_elem_),
      vector_arrays_(capacity_) {
  alloc_arrays();
  iterator creating_it = begin();
  try {
    for (const_iterator deque_it = deque.begin(); creating_it != end();
         ++creating_it, ++deque_it) {
      new (creating_it.operator->()) T(*deque_it);
    }
  } catch (...) {
    for (iterator destroying_it = begin(); destroying_it != creating_it;
         ++destroying_it) {
      destroying_it->~T();
    }
    for (size_t i = 0; i < capacity_; ++i) {
      dealloc(vector_arrays_[i]);
    }
    throw;
  }
}

template <typename T>
Deque<T>& Deque<T>::operator=(const Deque<T>& deque) {
  free_memory();
  size_ = deque.size_;
  capacity_ = deque.capacity_;
  ptr_first_array_ = deque.ptr_first_array_;
  ptr_last_array_ = deque.ptr_last_array_;
  ptr_first_array_ = deque.ptr_first_elem_;
  ptr_last_elem_ = deque.ptr_last_elem_;
  vector_arrays_.reserve(capacity_);
  alloc_arrays();
  iterator internal_it = begin();
  for (auto deque_it = deque.begin();
       internal_it != end() or deque_it != deque.end();
       ++internal_it, ++deque_it) {
    new (internal_it.operator->()) T(*deque_it);
  }
  return *this;
}

template <typename T>
size_t Deque<T>::size() const { return size_; }

template <typename T>
T& Deque<T>::operator[](size_t index) { return *(begin() + index); }

template <typename T>
const T& Deque<T>::operator[](size_t index) const {
  return *(begin() + index);
}

template <typename T>
T& Deque<T>::at(size_t index) {
  if (index >= size_) {
    throw std::out_of_range("Следи за тем, куда обращаешься, чувак");
  }
  return (*this)[index];
}

template <typename T>
const T& Deque<T>::at(size_t index) const {
  return const_cast<Deque<T>&>(*this).at(index);
}

template <typename T>
void Deque<T>::push_back(const T& value) {
  bool reserved = ptr_last_elem_ + 1 == static_cast<int>(kSizeArray) and
      ptr_last_array_ + 1 == static_cast<int>(capacity_);
  push(value, {ptr_last_array_, ptr_last_elem_}, &Deque::increment_ptr,
       &Deque::decrement_ptr, reserved);
}

template <typename T>
void Deque<T>::pop_back() {
  --size_;
  (vector_arrays_[ptr_last_array_] + ptr_last_elem_)->~T();
  decrement_ptr(ptr_last_array_, ptr_last_elem_);
}

template <typename T>
void Deque<T>::push_front(const T& value) {
  bool reserved = ptr_first_array_ == 0 and ptr_first_elem_ == 0;
  push(value, {ptr_first_array_, ptr_first_elem_}, &Deque::decrement_ptr,
       &Deque::increment_ptr, reserved);
}

template <typename T>
void Deque<T>::pop_front() {
  --size_;
  (vector_arrays_[ptr_first_array_] + ptr_first_elem_)->~T();
  increment_ptr(ptr_first_array_, ptr_first_elem_);
}

template <typename T>
typename Deque<T>::iterator Deque<T>::begin() {
  return {&vector_arrays_, ptr_first_array_, ptr_first_elem_};
}

template <typename T>
typename Deque<T>::iterator Deque<T>::end() {
  int copy_ptr_arr = ptr_last_array_;
  int copy_ptr_elem = ptr_last_elem_;
  increment_ptr(copy_ptr_arr, copy_ptr_elem);
  return iterator{&vector_arrays_, copy_ptr_arr, copy_ptr_elem};
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::cbegin() const {
  return {&vector_arrays_, ptr_first_array_, ptr_first_elem_};
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::cend() const {
  int copy_ptr_arr = ptr_last_array_;
  int copy_ptr_elem = ptr_last_elem_;
  increment_ptr(copy_ptr_arr, copy_ptr_elem);
  return const_iterator{&vector_arrays_, copy_ptr_arr, copy_ptr_elem};
}

template <typename T>
typename Deque<T>::const_iterator Deque<T>::begin() const { return cbegin(); }

template <typename T>
typename Deque<T>::const_iterator Deque<T>::end() const {
  return cbegin();
}

template <typename T>
typename Deque<T>::reverse_iterator Deque<T>::rbegin() {
  return reverse_iterator(end());
}

template <typename T>
typename Deque<T>::reverse_iterator Deque<T>::rend() {
  return reverse_iterator(begin());
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::crbegin() const {
  return const_reverse_iterator(end());
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::crend() const {
  return const_reverse_iterator(begin());
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::rbegin() const {
  return crbegin();
}

template <typename T>
typename Deque<T>::const_reverse_iterator Deque<T>::rend() const {
  return crend();
}

template <typename T>
void Deque<T>::insert(iterator insert_it, const T& value) {
  if (insert_it == end()) {
    push_back(value);
  } else {
    push_back(*(end() - 1));
    iterator iterator = end() - 2;
    while (iterator != insert_it) {
      *iterator = *(iterator - 1);
      --iterator;
    }
    *iterator = value;
  }
}

template <typename T>
void Deque<T>::erase(iterator erase_it) {
  while (erase_it != begin()) {
    *erase_it = *(erase_it - 1);
    --erase_it;
  }
  pop_front();
}

template <typename T>
Deque<T>::~Deque() { free_memory(); }

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>
Deque<T>::CommonIterator<IsConst>::operator+(difference_type n) const {
  CommonIterator<IsConst> copy = *this;
  copy += n;
  return copy;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>
Deque<T>::CommonIterator<IsConst>::operator-(difference_type n) const {
  return *this + (-n);
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>::reference
Deque<T>::CommonIterator<IsConst>::operator*() const {
  return (*ptr_to_vector_)[ptr_array_][ptr_elem_];
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>::pointer
Deque<T>::CommonIterator<IsConst>::operator->() const {
  return (*ptr_to_vector_)[ptr_array_] + ptr_elem_;
}

template <typename T>
template <bool IsConst>
Deque<T>::CommonIterator<IsConst>::CommonIterator(
    ptr_to_vector_type ptr_to_vector, int ptr_array, int ptr_elem)
    : ptr_to_vector_(ptr_to_vector),
      ptr_array_(ptr_array),
      ptr_elem_(ptr_elem) {}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>&
Deque<T>::CommonIterator<IsConst>::operator++() {
  increment_ptr(ptr_array_, ptr_elem_);
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>
Deque<T>::CommonIterator<IsConst>::operator++(int) {
  Deque<T>::CommonIterator<IsConst> copy = *this;
  increment_ptr(ptr_array_, ptr_elem_);
  return copy;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>&
Deque<T>::CommonIterator<IsConst>::operator--() {
  decrement_ptr(ptr_array_, ptr_elem_);
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>
Deque<T>::CommonIterator<IsConst>::operator--(int) {
  Deque<T>::CommonIterator<IsConst> copy = *this;
  decrement_ptr(ptr_array_, ptr_elem_);
  return copy;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>&
Deque<T>::CommonIterator<IsConst>::operator+=(difference_type n) {
  int numb_it = static_cast<int>(*this);
  numb_it += n;
  ptr_array_ = static_cast<int>(numb_it / kSizeArray);
  ptr_elem_ = static_cast<int>(numb_it % kSizeArray);
  return *this;
}

template <typename T>
template <bool IsConst>
typename Deque<T>::template CommonIterator<IsConst>&
Deque<T>::CommonIterator<IsConst>::operator-=(difference_type n) {
  return *this += -n;
}

template <typename T>
template <bool IsConst>
std::strong_ordering Deque<T>::CommonIterator<IsConst>::operator<=>(
    const CommonIterator& iterator) const {
  int current_iterator_numb = static_cast<int>(*this);
  int iterator_numb = static_cast<int>(iterator);
  if (current_iterator_numb > iterator_numb) {
    return std::strong_ordering::greater;
  }
  if (current_iterator_numb == iterator_numb) {
    return std::strong_ordering::equal;
  }
  return std::strong_ordering::less;
}

template <typename T>
template <bool IsConst>
Deque<T>::CommonIterator<IsConst>::operator int() const {
  return ptr_array_ * kSizeArray + ptr_elem_;
}

template <typename T>
template <bool IsConst>
Deque<T>::CommonIterator<IsConst>::operator CommonIterator<true>() const {
  return *this;
}

template <typename T>
template <bool IsConst>
Deque<T>::CommonIterator<IsConst>::difference_type
Deque<T>::CommonIterator<IsConst>::operator-(const CommonIterator& iterator) const {
  return static_cast<int>(*this) - static_cast<int>(iterator);
}

template <typename T>
template <bool IsConst>
bool Deque<T>::CommonIterator<IsConst>::operator==(const CommonIterator<IsConst> &iterator) const {
  return static_cast<int>(*this) == static_cast<int>(iterator);
}