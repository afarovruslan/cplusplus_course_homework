//
// Created by ruslan on 4/22/23.
//
#pragma once
#include <cassert>
#include <cmath>
#include <compare>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

template <typename T, typename Allocator = std::allocator<T>>
class List {
 public:
  struct BaseNode;
  struct Node;
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
  iterator insert(const_iterator pos, Node* new_node);
  iterator erase(const_iterator pos);
  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args);
  void swap(List& other);
  void fix_list();
  iterator get_dummy_iterator() const { return iterator(nullptr); };

  ~List() { free_memory(size_); }

 private:
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
  void set_default();
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
  BaseNode& operator=(const BaseNode&) = default;
  BaseNode(const BaseNode&) = default;
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
      : BaseNode(prev, next), value(std::forward<Args>(args)...) {}
  Node(const T& value, Node* prev, Node* next)
      : BaseNode(prev, next), value(value) {}
  Node(T&& value, Node* prev, Node* next)
      : BaseNode(prev, next), value(std::move(value)) {}
  Node(Node* prev, Node* next) : BaseNode(prev, next) {}
  Node(Node&& node) noexcept
      : BaseNode(node.prev, node.next), value(std::move(node.value)) {
    node.prev = nullptr;
    node.next = nullptr;
  }
  Node(const Node& node) : BaseNode(node.prev, node.next), value(node.value) {}
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
      end_node_(other.end_node_),
      begin_node_(other.begin_node_) {
  other.set_default();
  fix_list();
}

template <typename T, typename Allocator>
List<T, Allocator>::List(List&& other, const Allocator& allocator)
    : alloc_(allocator), size_(other.size_) {
  if (alloc_ != other.get_allocator()) {
    fill(std::move(other));
  } else {
    begin_node_ = other.begin_node_;
    end_node_ = other.end_node_;
  }
  other.set_default();
  fix_list();
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
  copy.set_default();
  return *this;
}

template <typename T, typename Allocator>
typename List<T, Allocator>::iterator List<T, Allocator>::insert(
    const_iterator pos, Node* new_node) {
  auto pos_ptr = const_cast<BaseNode*>(pos.get_ptr());
  pos_ptr->prev->next = new_node;
  pos_ptr->prev = new_node;
  ++size_;
  if (pos == begin()) {
    begin_node_ = new_node;
  }
  return iterator{new_node};
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
  set_default();
}

template <typename T, typename Allocator>
void List<T, Allocator>::set_default() {
  size_ = 0;
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
  return insert(pos, new_node);
}

template <typename T, typename Allocator>
void List<T, Allocator>::swap(List& other) {
  if (alloc_traits::propagate_on_container_swap::value) {
    std::swap(alloc_, other.alloc_);
  }
  std::swap(size_, other.size_);
  std::swap(end_node_, other.end_node_);
  std::swap(begin_node_, other.begin_node_);
  fix_list();
  other.fix_list();
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

template <typename T, typename Allocator>
void List<T, Allocator>::fix_list() {
  if (size_ == 0) {
    begin_node_ = static_cast<Node*>(&end_node_);
    end_node_.prev = static_cast<Node*>(&end_node_);
  }
  end_node_.prev->next = static_cast<Node*>(&end_node_);
  begin_node_->prev = static_cast<Node*>(&end_node_);
}

template <typename Key, typename Value, typename Hash = std::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
 public:
  template <bool IsConst>
  class CommonIterator;

  using node_type = std::pair<const Key, Value>;
  using alloc_traits = std::allocator_traits<Allocator>;
  using key_type = Key;
  using mapped_type = Value;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using hasher = Hash;
  using key_equal = Equal;
  using allocator_type = Allocator;
  using reference = node_type&;
  using pointer = typename alloc_traits::pointer;
  using const_pointer = typename alloc_traits::const_pointer;
  using iterator = CommonIterator<false>;
  using const_iterator = CommonIterator<true>;

  UnorderedMap();
  explicit UnorderedMap(size_type bucket_count, const Hash& hash = Hash(),
                        const Equal& equal = Equal(),
                        const Allocator& alloc = Allocator());
  UnorderedMap(size_type bucket_count, const Allocator& alloc)
      : UnorderedMap(bucket_count, Hash(), Equal(), alloc) {}
  UnorderedMap(size_type bucket_count, const Hash& hash, const Allocator& alloc)
      : UnorderedMap(bucket_count, hash, Equal(), alloc) {}
  explicit UnorderedMap(const Allocator& alloc);
  UnorderedMap(const UnorderedMap& other);
  UnorderedMap(const UnorderedMap& other, const Allocator& alloc);
  UnorderedMap(UnorderedMap&& other);
  UnorderedMap(UnorderedMap&& other, const Allocator& alloc);

  UnorderedMap& operator=(const UnorderedMap& other);
  UnorderedMap& operator=(UnorderedMap&& other);
  allocator_type get_allocator() const { return alloc_; }
  Value& operator[](const Key& key) { return get_or_insert_value(key); }
  Value& operator[](Key&& key) { return get_or_insert_value(std::move(key)); }
  Value& at(const Key& key);
  const Value& at(const Key& key) const {
    return const_cast<UnorderedMap&>(*this).at(key);
  }
  [[nodiscard]] size_type size() const { return size_; }
  iterator begin() { return iterator(elements_.begin()); }
  const_iterator begin() const { return cbegin(); }
  const_iterator cbegin() const { return const_iterator(elements_.begin()); }
  iterator end() { return iterator(elements_.end()); }
  const_iterator end() const { return cend(); }
  const_iterator cend() const { return const_iterator(elements_.end()); }
  std::pair<iterator, bool> insert(const node_type& node) {
    return emplace(node);
  }
  template <typename P>
  std::pair<iterator, bool> insert(P&& node) {
    return emplace(std::forward<P>(node));
  }
  template <typename InputIt>
  void insert(InputIt first, InputIt last);
  template <typename... Args>
  auto emplace(Args&&... args);
  iterator erase(const_iterator pos);
  iterator erase(const_iterator first, const_iterator last);
  iterator find(const Key& key);
  const_iterator find(const Key& key) const {
    return const_cast<UnorderedMap&>(*this).find(key);
  }
  [[nodiscard]] size_type bucket_count() const { return buckets_.size(); }
  void reserve(size_type count);
  [[nodiscard]] float load_factor() const {
    return static_cast<float>(size_) / bucket_count();
  }
  [[nodiscard]] float max_load_factor() const { return max_load_factor_; }
  void max_load_factor(float new_max_load_factor);
  void rehash(size_type count);
  void swap(UnorderedMap& other);

  ~UnorderedMap();

 private:
  using hash_type = std::size_t;
  using list_value_type = std::pair<node_type*, hash_type>;
  using list_alloc_type =
      typename alloc_traits::template rebind_alloc<list_value_type>;
  using list_iterator_type =
      typename List<list_value_type, list_alloc_type>::iterator;
  using arr_alloc_type =
      typename alloc_traits::template rebind_alloc<list_iterator_type>;

  constexpr static const size_type kBeginBucketCount = 1;
  constexpr static const float kDefaultMaxLoadFactor = 1;
  [[no_unique_address]] hasher hash_;
  [[no_unique_address]] key_equal equal_;
  [[no_unique_address]] allocator_type alloc_;
  size_type size_;
  List<list_value_type, list_alloc_type> elements_;
  std::vector<list_iterator_type, arr_alloc_type> buckets_;
  float max_load_factor_ = kDefaultMaxLoadFactor;

  template <typename U>
  hash_type get_hash(U&& value) {
    return hash_(std::forward<U>(value)) % bucket_count();
  }
  template <typename U>
  Value& get_or_insert_value(U&& key);
  void rehash_if_necessary();
  void set_default();
};

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap()
    : UnorderedMap(kBeginBucketCount) {}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(
    size_type bucket_count, const Hash& hash, const Equal& equal,
    const Allocator& alloc)
    : hash_(hash),
      equal_(equal),
      alloc_(alloc),
      size_(0),
      elements_(alloc),
      buckets_(bucket_count, elements_.get_dummy_iterator(), alloc) {}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(
    const Allocator& alloc)
    : UnorderedMap(kBeginBucketCount, alloc) {}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(
    const UnorderedMap& other)
    : UnorderedMap(other, alloc_traits::select_on_container_copy_construction(
                              other.get_allocator())) {}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(
    const UnorderedMap& other, const Allocator& alloc)
    : hash_(other.hash_),
      equal_(other.equal_),
      alloc_(alloc),
      size_(0),
      elements_(alloc_),
      buckets_(kBeginBucketCount, elements_.get_dummy_iterator(), alloc_) {
  insert(other.begin(), other.end());
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(
    UnorderedMap&& other)
    : hash_(std::move(other.hash_)),
      equal_(std::move(other.equal_)),
      alloc_(std::move(other.alloc_)),
      size_(other.size_),
      elements_(std::move(other.elements_)),
      buckets_(std::move(other.buckets_)),
      max_load_factor_(other.max_load_factor_) {}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::UnorderedMap(
    UnorderedMap&& other, const Allocator& alloc)
    : hash_(std::move(other.hash_)),
      equal_(std::move(other.equal_)),
      alloc_(alloc),
      size_(0),
      elements_(alloc_),
      buckets_(alloc_),
      max_load_factor_(other.max_load_factor_) {
  elements_ = std::move(other.elements_);
  size_ = other.size_;
  buckets_ = std::move(other.buckets_);
  other.set_default();
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>&
UnorderedMap<Key, Value, Hash, Equal, Allocator>::operator=(
    const UnorderedMap& other) {
  Allocator alloc_copy = alloc_;
  if (alloc_traits::propagate_on_container_copy_assignment::value) {
    alloc_copy = other.alloc_;
  }
  UnorderedMap<Key, Value, Hash, Equal, Allocator> copy(other, alloc_copy);
  swap(copy);
  return *this;
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>&
UnorderedMap<Key, Value, Hash, Equal, Allocator>::operator=(
    UnorderedMap&& other) {
  Allocator alloc_copy = alloc_;
  if (alloc_traits::propagate_on_container_move_assignment::value) {
    alloc_copy = other.alloc_;
  }
  UnorderedMap<Key, Value, Hash, Equal, Allocator> copy(std::move(other),
                                                        alloc_copy);
  swap(copy);
  return *this;
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::max_load_factor(
    float new_max_load_factor) {
  float old_max_load_factor = max_load_factor_;
  try {
    max_load_factor_ = new_max_load_factor;
    rehash_if_necessary();
  } catch (...) {
    max_load_factor_ = old_max_load_factor;
    throw;
  }
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::reserve(
    size_type count) {
  if (max_load_factor_ * bucket_count() < static_cast<float>(count)) {
    rehash(std::ceil(static_cast<float>(count) / max_load_factor_));
  }
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
template <typename U>
Value& UnorderedMap<Key, Value, Hash, Equal, Allocator>::get_or_insert_value(
    U&& key) {
  auto iter = find(key);
  if (iter != end()) {
    return iter->second;
  }
  auto [iter_emplace, is_emplace] = emplace(std::forward<U>(key), Value());
  return iter_emplace->second;
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
Value& UnorderedMap<Key, Value, Hash, Equal, Allocator>::at(const Key& key) {
  auto iter = find(key);
  if (iter != end()) {
    return iter->second;
  }
  throw std::out_of_range("key error!");
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator
UnorderedMap<Key, Value, Hash, Equal, Allocator>::find(const Key& key) {
  hash_type hash = get_hash(key);
  auto iter = buckets_[hash];
  while (iter != elements_.get_dummy_iterator() and hash == iter->second) {
    if (equal_(key, iter->first->first)) {
      return iterator(iter);
    }
    --iter;
  }
  return iterator(elements_.end());
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
template <typename... Args>
auto UnorderedMap<Key, Value, Hash, Equal, Allocator>::emplace(Args&&... args) {
  node_type* new_node = alloc_traits::allocate(alloc_, 1);
  try {
    alloc_traits::construct(alloc_, new_node, std::forward<Args>(args)...);
  } catch (...) {
    alloc_traits::deallocate(alloc_, new_node, 1);
    throw;
  }
  auto iter = find(new_node->first);
  if (iter == end()) {
    hash_type hash;
    try {
      ++size_;
      rehash_if_necessary();
      hash = get_hash(new_node->first);
    } catch (...) {
      --size_;
      alloc_traits::destroy(alloc_, new_node);
      alloc_traits::deallocate(alloc_, new_node, 1);
      throw;
    }
    auto insert_it =
        elements_.insert(elements_.end(), std::make_pair(new_node, hash));
    buckets_[hash] = insert_it;
    return std::make_pair(iterator(insert_it), true);
  }
  alloc_traits::deallocate(alloc_, new_node, 1);
  return std::make_pair(iter, false);
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::rehash_if_necessary() {
  if (load_factor() > max_load_factor()) {
    rehash(2 * bucket_count());
  }
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::set_default() {
  size_ = 0;
  max_load_factor(kDefaultMaxLoadFactor);
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
UnorderedMap<Key, Value, Hash, Equal, Allocator>::~UnorderedMap() {
  for (auto iter = elements_.begin(); iter != elements_.end(); ++iter) {
    alloc_traits::destroy(alloc_, iter->first);
    alloc_traits::deallocate(alloc_, iter->first, 1);
  }
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
template <typename InputIt>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::insert(InputIt first,
                                                              InputIt last) {
  while (first != last) {
    insert(std::forward<decltype(*first)>(*first));
    ++first;
  }
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator
UnorderedMap<Key, Value, Hash, Equal, Allocator>::erase(const_iterator pos) {
  list_iterator_type iter = pos.get_list_iterator();
  hash_type hash = iter->second;
  if (buckets_[hash] == iter) {
    ++iter;
    if (iter != elements_.get_dummy_iterator() and hash == iter->second) {
      buckets_[hash] = iter;
    } else {
      buckets_[hash] = elements_.get_dummy_iterator();
    }
    --iter;
  }
  auto ptr = const_cast<pointer>(pos.operator->());
  auto erase_it = iterator(elements_.erase(iter));
  alloc_traits::destroy(alloc_, ptr);
  alloc_traits::deallocate(alloc_, ptr, 1);
  --size_;
  return erase_it;
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
typename UnorderedMap<Key, Value, Hash, Equal, Allocator>::iterator
UnorderedMap<Key, Value, Hash, Equal, Allocator>::erase(const_iterator first,
                                                        const_iterator last) {
  while (first != last) {
    erase(first++);
  }
  return reinterpret_cast<iterator&>(--first);
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::rehash(size_type count) {
  decltype(elements_) new_elements(alloc_);
  decltype(buckets_) new_buckets(count, elements_.get_dummy_iterator(), alloc_);
  for (auto iter = elements_.begin(); iter != elements_.end(); ++iter) {
    hash_type new_hash = hash_(iter->first->first) % count;
    auto insert_iter = new_elements.insert(
        new_buckets[new_hash] == elements_.get_dummy_iterator()
            ? new_elements.end()
            : new_buckets[new_hash],
        std::make_pair(iter->first, new_hash));
    new_buckets[new_hash] = insert_iter;
  }
  buckets_.swap(new_buckets);
  elements_.swap(new_elements);
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
void UnorderedMap<Key, Value, Hash, Equal, Allocator>::swap(
    UnorderedMap& other) {
  if (alloc_traits::propagate_on_container_swap::value) {
    std::swap(other.alloc_, alloc_);
  }
  std::swap(equal_, other.equal_);
  std::swap(hash_, other.hash_);
  buckets_.swap(other.buckets_);
  elements_.swap(other.elements_);
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
template <bool IsConst>
class UnorderedMap<Key, Value, Hash, Equal, Allocator>::CommonIterator {
 public:
  using value_type = std::conditional_t<IsConst, const node_type, node_type>;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = int;
  using reference = value_type&;
  using pointer = value_type*;

  explicit CommonIterator(
      typename List<list_value_type, list_alloc_type>::const_iterator iterator)
      : iterator_(reinterpret_cast<list_iterator_type&>(iterator)) {}
  operator CommonIterator<true>() const {
    return CommonIterator<true>(iterator_);
  }
  CommonIterator& operator++();
  CommonIterator& operator--();
  CommonIterator operator++(int) { return CommonIterator(iterator_++); }
  reference operator*() const { return *(iterator_->first); }
  pointer operator->() const { return iterator_->first; }
  bool operator==(const CommonIterator& other) const {
    return iterator_ == other.iterator_;
  }
  bool operator!=(const CommonIterator& other) const {
    return iterator_ != other.iterator_;
  }
  list_iterator_type get_list_iterator() const { return iterator_; }

 private:
  list_iterator_type iterator_;
};

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
template <bool IsConst>
typename UnorderedMap<Key, Value, Hash, Equal,
                      Allocator>::template CommonIterator<IsConst>&
UnorderedMap<Key, Value, Hash, Equal,
             Allocator>::CommonIterator<IsConst>::operator++() {
  ++iterator_;
  return *this;
}

template <typename Key, typename Value, typename Hash, typename Equal,
          typename Allocator>
template <bool IsConst>
typename UnorderedMap<Key, Value, Hash, Equal,
                      Allocator>::template CommonIterator<IsConst>&
UnorderedMap<Key, Value, Hash, Equal,
             Allocator>::CommonIterator<IsConst>::operator--() {
  --iterator_;
  return *this;
}