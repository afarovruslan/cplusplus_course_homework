#pragma once
#include <compare>
#include <iostream>
#include <memory>

template <size_t N>
class StackStorage {
 private:
  char array_[N];
  std::size_t reserved_ = 0;

 public:
  StackStorage& operator=(const StackStorage&) = delete;
  StackStorage() = default;
  StackStorage(const StackStorage& other) = delete;
  char* get_array() { return array_ + reserved_; }
  void reserve(size_t n) { reserved_ += n; }
};

template <typename T, size_t N>
class StackAllocator {
 private:
  void* storage_;
  void** ptr_to_storage_;
  std::size_t size_;

 public:
  using value_type = T;
  using pointer = T*;
  using size_type = size_t;

  template <typename U>
  struct rebind {  // NOLINT
    using other = StackAllocator<U, N>;
  };
  template <size_t U>
  StackAllocator(StackStorage<U>& stack_storage)
      : storage_(stack_storage.get_array()),
        ptr_to_storage_(&storage_),
        size_(N) {
    stack_storage.reserve(N);
  }
  template <typename U>
  StackAllocator(const StackAllocator<U, N>& allocator)
      : storage_(allocator.get_storage()),
        ptr_to_storage_(allocator.get_ptr_to_storage()),
        size_(allocator.get_size()) {}
  T* allocate(size_t n);
  void deallocate(pointer ptr, size_type n);
  [[nodiscard]] void* get_storage() const { return storage_; }
  [[nodiscard]] void** get_ptr_to_storage() const { return ptr_to_storage_; };
  [[nodiscard]] size_t get_size() const { return size_; }
};

template <typename T, size_t N>
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

template <typename T, size_t N>
void StackAllocator<T, N>::deallocate(pointer ptr, size_type n) {
  std::ignore = ptr;
  std::ignore = n;
}

template <typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  template <bool IsConst>
  class CommonIterator;

  using value_type = T;
  using allocator_type = Allocator;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename std::allocator_traits<Allocator>::pointer;
  using const_pointer =
      typename std::allocator_traits<Allocator>::const_pointer;
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List();
  List(size_type count, const T& value,
       const Allocator& allocator = Allocator());
  explicit List(const Allocator& allocator);
  explicit List(size_type count, const Allocator& allocator = Allocator());
  List(const List& other);
  List(const List& other, const Allocator& allocator);
  List(List&& other) noexcept;
  List(List&& other, const Allocator& allocator);

  allocator_type get_allocator() const { return alloc_; }
  List& operator=(const List& other);
  List& operator=(List&& other) noexcept;
  [[nodiscard]] size_type size() const { return size_; }
  [[nodiscard]] bool empty() const { return size_ == 0; }
  void push_back(const T& value) { insert(end(), value); }
  void push_back(T&& value) { insert(end(), std::move_if_noexcept(value)); }
  void pop_back() { erase(--end()); }
  void push_front(const T& value) { insert(begin(), value); }
  void push_front(T&& value) { insert(begin(), std::move_if_noexcept(value)); }
  void pop_front() { erase(begin()); }
  iterator begin() { return iterator{begin_node_}; }
  iterator end() { return iterator{&end_node_}; }
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
  const_iterator cbegin() const { return const_iterator{begin_node_}; }
  const_iterator cend() const { return const_iterator{&end_node_}; }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const { return rbegin(); }
  const_reverse_iterator crend() const { return rend(); }
  iterator insert(const_iterator pos, const T& value) {
    return emplace(pos, value);
  }
  iterator insert(const_iterator pos, T&& value) {
    return emplace(pos, std::move_if_noexcept(value));
  }
  iterator erase(const_iterator pos);
  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args);

  ~List() { free_memory(size_); }

 private:
  struct BaseNode;
  struct Node;
  using node_allocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using alloc_traits = std::allocator_traits<node_allocator>;

  [[no_unique_address]] node_allocator alloc_;
  size_type size_;
  BaseNode end_node_;
  Node* begin_node_;

  void fill(const List& other);
  void fill(List&& other);
  void free_memory(size_type count_allocated);
  void swap(List& other);
};

template <typename T, typename Allocator>
template <bool IsConst>
class List<T, Allocator>::CommonIterator {
 public:
  using value_type = std::conditional_t<IsConst, const T, T>;
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = int;
  using reference = value_type&;
  using pointer = value_type*;
  using base_node_type = std::conditional_t<IsConst, const BaseNode, BaseNode>;
  using node_type = std::conditional_t<IsConst, const Node, Node>;

  explicit CommonIterator(base_node_type* node) : node_(node) {}
  CommonIterator& operator++();
  CommonIterator operator++(int);
  CommonIterator& operator--();
  CommonIterator operator--(int);
  reference operator*() const { return static_cast<node_type*>(node_)->value; }
  pointer operator->() const { return &static_cast<node_type*>(node_)->value; }
  operator CommonIterator<true>() const { return CommonIterator<true>(node_); }
  bool operator==(const CommonIterator& other) const {
    return node_ == other.node_;
  }
  bool operator!=(const CommonIterator& other) const {
    return node_ != other.node_;
  }
  base_node_type* get_ptr() const { return node_; }

 private:
  base_node_type* node_;
};

template <typename T, typename Allocator>
struct List<T, Allocator>::BaseNode {
  Node* prev = nullptr;
  Node* next = nullptr;

  BaseNode(BaseNode&& other) noexcept;
  BaseNode() : prev(static_cast<Node*>(this)), next(static_cast<Node*>(this)) {}
  BaseNode(Node* prev, Node* next) : prev(prev), next(next) {}
  BaseNode& operator=(BaseNode&& other) noexcept;
};

template <typename T, typename Allocator>
List<T, Allocator>::BaseNode::BaseNode(BaseNode&& other) noexcept
    : prev(other.prev), next(other.next) {
  other.prev = nullptr;
  other.next = nullptr;
}

template <typename T, typename Allocator>
typename List<T, Allocator>::BaseNode& List<T, Allocator>::BaseNode::operator=(
    BaseNode&& other) noexcept {
  prev = other.prev;
  next = other.next;
  other.prev = nullptr;
  other.next = nullptr;
  return *this;
}

template <typename T, typename Allocator>
struct List<T, Allocator>::Node : BaseNode {
  T value;
  template <typename... Args>
  Node(Node* prev, Node* next, Args&&... args)
      : BaseNode(prev, next), value(std::forward<Args...>(args...)) {}
  Node(const T& value, Node* prev, Node* next)
      : BaseNode(prev, next), value(value) {}
  Node(T&& value, Node* prev, Node* next)
      : BaseNode(prev, next), value(std::move(value)) {}
  Node(Node* prev, Node* next) : BaseNode(prev, next) {}
};

template <typename T, typename Allocator>
List<T, Allocator>::List()
    : size_(0), begin_node_(static_cast<Node*>(&end_node_)) {}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_type count, const T& value,
                         const Allocator& allocator)
    : List(allocator) {
  size_type count_allocated = 0;
  try {
    for (; count_allocated < count; ++count_allocated) {
      push_back(value);
    }
    size_ = count;
  } catch (...) {
    free_memory(count_allocated);
    throw;
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const Allocator& allocator)
    : alloc_(allocator),
      size_(0),
      begin_node_(static_cast<Node*>(&end_node_)) {}

template <typename T, typename Allocator>
List<T, Allocator>::List(size_type count, const Allocator& allocator) try
    : List(allocator) {
  for (size_type count_allocated = 0; count_allocated < count;
       ++count_allocated) {
    emplace(end());
  }
  size_ = count;
} catch (...) {
  throw;
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List& other)
    : alloc_(alloc_traits::select_on_container_copy_construction(
          other.get_allocator())),
      size_(other.size_),
      begin_node_(static_cast<Node*>(&end_node_)) {
  fill(other);
}

template <typename T, typename Allocator>
List<T, Allocator>::List(const List& other, const Allocator& allocator)
    : alloc_(allocator),
      size_(other.size_),
      begin_node_(static_cast<Node*>(&end_node_)) {
  fill(other);
}

template <typename T, typename Allocator>
void List<T, Allocator>::fill(const List& other) {
  size_type count_allocated = 0;
  try {
    for (auto other_iterator = other.begin(); other_iterator != other.end();
         ++count_allocated, ++other_iterator) {
      push_back(*other_iterator);
    }
    size_ = other.size_;
  } catch (...) {
    free_memory(count_allocated);
    throw;
  }
}

template <typename T, typename Allocator>
void List<T, Allocator>::fill(List&& other) {
  size_type count_allocated = 0;
  try {
    for (auto other_iterator = other.begin(); other_iterator != other.end();
         ++count_allocated, ++other_iterator) {
      push_back(std::move(*other_iterator));
    }
    size_ = other.size_;
  } catch (...) {
    free_memory(count_allocated);
    throw;
  }
}

template <typename T, typename Allocator>
List<T, Allocator>::List(List&& other) noexcept
    : alloc_(std::move(other.alloc_)),
      size_(other.size_),
      begin_node_(other.begin_node_),
      end_node_(std::move(other.end_node_)) {}

template <typename T, typename Allocator>
List<T, Allocator>::List(List&& other, const Allocator& allocator)
    : alloc_(allocator), size_(other.size_) {
  if (alloc_ != other.get_allocator()) {
    fill(std::move(other));
  } else {
    begin_node_ = other.begin_node_;
    end_node_ = std::move(other.end_node_);
  }
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(const List& other) {
  Allocator alloc_copy = alloc_;
  if (alloc_traits::propagate_on_container_copy_assignment::value) {
    alloc_copy = other.alloc_;
  }
  List<T, Allocator> copy(other, alloc_copy);
  swap(copy);
  return *this;
}

template <typename T, typename Allocator>
List<T, Allocator>& List<T, Allocator>::operator=(List&& other) noexcept {
  Allocator alloc_copy = alloc_;
  if (alloc_traits::propagate_on_container_move_assignment::value) {
    alloc_copy = std::move(other.alloc_);
  }
  List<T, Allocator> copy(std::move(other), alloc_copy);
  swap(copy);
  return *this;
}

template <typename T, typename Allocator>
void List<T, Allocator>::free_memory(size_type count_allocated) {
  Node* now_node = begin_node_;
  Node* new_node;
  for (; count_allocated > 0; --count_allocated) {
    new_node = now_node->next;
    alloc_traits::destroy(alloc_, now_node);
    alloc_traits::deallocate(alloc_, now_node, 1);
    now_node = new_node;
  }
  end_node_.prev = static_cast<Node*>(&end_node_);
  end_node_.next = static_cast<Node*>(&end_node_);
  begin_node_ = static_cast<Node*>(&end_node_);
}

template <typename T, typename Allocator>
template <typename... Args>
typename List<T, Allocator>::iterator List<T, Allocator>::emplace(
    const_iterator pos, Args&&... args) {
  auto* pos_ptr = const_cast<BaseNode*>(pos.get_ptr());
  Node* new_node = alloc_traits::allocate(alloc_, 1);
  try {
    alloc_traits::construct(alloc_, new_node, pos_ptr->prev,
                            pos_ptr->prev->next, std::forward<Args>(args)...);
  } catch (...) {
    alloc_traits::deallocate(alloc_, new_node, 1);
    throw;
  }
  pos_ptr->prev->next = new_node;
  pos_ptr->prev = new_node;
  ++size_;
  if (pos == begin()) {
    begin_node_ = new_node;
  }
  return iterator{new_node};
}

template <typename T, typename Allocator>
void List<T, Allocator>::swap(List& other) {
  std::swap(alloc_, other.alloc_);
  std::swap(size_, other.size_);
  std::swap(end_node_, other.end_node_);
  std::swap(begin_node_, other.begin_node_);
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>&
List<T, Allocator>::CommonIterator<IsConst>::operator++() {
  node_ = node_->next;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>
List<T, Allocator>::CommonIterator<IsConst>::operator++(int) {
  CommonIterator<IsConst> copy = *this;
  ++*this;
  return copy;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>&
List<T, Allocator>::CommonIterator<IsConst>::operator--() {
  node_ = node_->prev;
  return *this;
}

template <typename T, typename Allocator>
template <bool IsConst>
typename List<T, Allocator>::template CommonIterator<IsConst>
List<T, Allocator>::CommonIterator<IsConst>::operator--(int) {
  CommonIterator<IsConst> copy = *this;
  --*this;
  return copy;
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::erase(
    const_iterator pos) {
  Node* erase_node = static_cast<Node*>(pos.get_ptr()->next->prev);
  if (pos == begin()) {
    begin_node_ = erase_node->next;
  }
  erase_node->next->prev = erase_node->prev;
  erase_node->prev->next = erase_node->next;
  alloc_traits::destroy(alloc_, erase_node);
  alloc_traits::deallocate(alloc_, erase_node, 1);
  --size_;
  return iterator(erase_node);
}