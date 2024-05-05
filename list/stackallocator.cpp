#include "stackallocator.h"

template <std::size_t N>
char* StackStorage<N>::get_array() { return array_ + reserved_; }

template <std::size_t N>
void StackStorage<N>::reserve(int n) { reserved_ += n; }

template <typename T, std::size_t N>
template <std::size_t U>
StackAllocator<T, N>::StackAllocator(StackStorage<U> &stack_storage)
    : storage_(stack_storage.get_array()),
      ptr_to_storage_(&storage_),
      size_(N) {
  stack_storage.reserve(N);
}

template <typename T, std::size_t N>
template <typename U>
StackAllocator<T, N>::StackAllocator(const StackAllocator<U, N> &allocator)
    : storage_(allocator.get_storage()),
      ptr_to_storage_(allocator.get_ptr_to_storage()),
      size_(allocator.get_size()) {}

template <typename T, std::size_t N>
T* StackAllocator<T, N>::allocate(size_type n) {
  if (std::align(alignof(T), n * sizeof(T), *ptr_to_storage_, size_) !=
      nullptr) {
    T* result = reinterpret_cast<T*>(*ptr_to_storage_);
    *ptr_to_storage_ =
        reinterpret_cast<char*>(*ptr_to_storage_) + sizeof(T) * n;
    size_ -= n * sizeof(T);
    return result;
  }
  throw std::bad_alloc();
}

template <typename T, std::size_t N>
void StackAllocator<T, N>::deallocate(pointer ptr, size_type n) {
  std::ignore = ptr;
  std::ignore = n;
}

template <typename T, std::size_t N>
void* StackAllocator<T, N>::get_storage() const {
  return storage_;
}

template <typename T, std::size_t N>
void** StackAllocator<T, N>::get_ptr_to_storage() const {
  return ptr_to_storage_;
}

template <typename T, std::size_t N>
std::size_t StackAllocator<T, N>::get_size() const {
  return size_;
}