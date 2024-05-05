#pragma once
#include <memory>
#include <type_traits>

class BaseControlBlock {
 public:
  mutable std::size_t count;
  mutable std::size_t weak_count;

  BaseControlBlock() : BaseControlBlock(1, 0) {}
  BaseControlBlock(std::size_t count, std::size_t weak_count);
  virtual ~BaseControlBlock() = default;
  virtual void* get_ptr() const = 0;
  virtual void use_deleter(void* ptr) = 0;
  virtual void destroy() = 0;
};

template <typename T>
class SmartPointer {
 protected:
  template <typename Deleter, typename Alloc>
  class ControlBlockRegular : public BaseControlBlock {
   public:
    T* ptr;
    [[no_unique_address]] Deleter deleter;
    [[no_unique_address]] Alloc alloc;

    template <typename Y>
    ControlBlockRegular(Y* ptr, Deleter deleter, Alloc alloc);
    void* get_ptr() const override { return ptr; }
    void use_deleter(void* ptr) override { deleter(reinterpret_cast<T*>(ptr)); }
    void destroy() override;
  };

  template <typename Alloc>
  class ControlBlockMakeShared : public BaseControlBlock {
   public:
    mutable T object;
    [[no_unique_address]] Alloc alloc;

    template <typename... Args>
    explicit ControlBlockMakeShared(const Alloc& alloc, Args&&... args);
    void* get_ptr() const override { return &object; }
    void use_deleter(void* ptr) override;
    void destroy() override;
  };

  BaseControlBlock* control_block_;

  void increment_field(std::size_t(BaseControlBlock::*field)) const;
  void decrement_field(std::size_t(BaseControlBlock::*field)) const;
  void increment_count() const { increment_field(&BaseControlBlock::count); }
  void increment_weak_count() const;
  void decrement_count() const { decrement_field(&BaseControlBlock::count); }
  void decrement_weak_count() const;

 public:
  template <typename ControlBlock>
  SmartPointer(ControlBlock* control_block) : control_block_(control_block) {}
  SmartPointer(std::nullptr_t) : SmartPointer() {}
  SmartPointer() : control_block_(nullptr) {}

  std::size_t use_count() const;
  template <typename Pointer>
  void swap(Pointer& pointer);
};

template <typename T>
class SharedPtr;

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args);  // NOLINT

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args);  // NOLINT

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis {
 private:
  WeakPtr<T> weak_ptr_;

 public:
  SharedPtr<T> shared_from_this() const { return weak_ptr_.lock(); }

  friend SharedPtr<T>;
};

template <typename T>
class SharedPtr : public SmartPointer<T> {
 private:
  explicit SharedPtr(BaseControlBlock* control_block);

 public:
  SharedPtr() noexcept : SmartPointer<T>(nullptr) {}
  explicit SharedPtr(std::nullptr_t) noexcept : SharedPtr() {}
  template <typename Y>
  explicit SharedPtr(Y* ptr) : SharedPtr(ptr, std::default_delete<T>()) {}
  template <typename Y, typename Deleter>
  SharedPtr(Y* ptr, Deleter deleter);
  template <typename Y, typename Deleter, typename Alloc>
  SharedPtr(Y* ptr, Deleter deleter, Alloc alloc);
  template <typename Y>
  SharedPtr(const SharedPtr<Y>& shared_ptr);
  SharedPtr(const SharedPtr& shared_ptr);
  template <typename Y>
  SharedPtr(SharedPtr<Y>&& shared_ptr);
  SharedPtr(SharedPtr&& shared_ptr);
  template <typename Y>
  SharedPtr(const WeakPtr<Y>& weak_ptr);

  template <typename Y>
  SharedPtr& operator=(const SharedPtr<Y>& shared_ptr) noexcept;
  SharedPtr& operator=(const SharedPtr& shared_ptr) noexcept;
  template <typename Y>
  SharedPtr& operator=(SharedPtr<Y>&& shared_ptr) noexcept;
  SharedPtr& operator=(SharedPtr&& shared_ptr) noexcept;

  void reset() noexcept { SharedPtr().swap(*this); }
  template <typename Y>
  void reset(Y* ptr);
  template <typename Y, typename Deleter>
  void reset(Y* ptr, Deleter deleter);
  template <typename Y, typename Deleter, typename Alloc>
  void reset(Y* ptr, Deleter deleter, Alloc alloc);
  explicit operator bool() const noexcept;
  T& operator*() const { return *get(); }
  T* operator->() const { return get(); }
  T* get() const;

  ~SharedPtr();

  template <typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&... args);  // NOLINT
  template <typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc& alloc,  // NOLINT
                                     Args&&... args);
  template <typename Y>
  friend class WeakPtr;
  template <typename U>
  friend class SharedPtr;
};

template <typename T>
class WeakPtr : public SmartPointer<T> {
 public:
  WeakPtr() noexcept : SmartPointer<T>(nullptr) {}
  template <typename Y>
  WeakPtr(const WeakPtr<Y>& weak_ptr) noexcept;
  WeakPtr(const WeakPtr& weak_ptr) noexcept;
  template <typename Y>
  WeakPtr(WeakPtr<Y>&& weak_ptr) noexcept;
  WeakPtr(WeakPtr&& weak_ptr) noexcept;
  template <typename Y>
  WeakPtr(const SharedPtr<Y>& shared_ptr);

  template <typename Y>
  WeakPtr& operator=(const WeakPtr<Y>& weak_ptr);
  template <typename Y>
  WeakPtr& operator=(WeakPtr<Y>&& weak_ptr);
  template <typename Y>
  WeakPtr& operator=(const SharedPtr<Y>& shared_ptr);

  [[nodiscard]] bool expired() const noexcept;
  SharedPtr<T> lock() const noexcept;

  ~WeakPtr();

  template <typename Y>
  friend class SharedPtr;
  template <typename Y>
  friend class WeakPtr;
};

BaseControlBlock::BaseControlBlock(std::size_t count, std::size_t weak_count)
    : count(count), weak_count(weak_count) {}

template <typename T>
template <typename Deleter, typename Alloc>
template <typename Y>
SmartPointer<T>::ControlBlockRegular<Deleter, Alloc>::ControlBlockRegular(
    Y* ptr, Deleter deleter, Alloc alloc)
    : ptr(ptr), deleter(deleter), alloc(alloc) {}

template <typename T>
template <typename Deleter, typename Alloc>
void SmartPointer<T>::ControlBlockRegular<Deleter, Alloc>::destroy() {
  using alloc_control_block_t =
      typename std::allocator_traits<Alloc>::template rebind_alloc<
          typename SharedPtr<T>::template ControlBlockRegular<Deleter, Alloc>>;
  alloc_control_block_t alloc_control_block = alloc;
  (&alloc)->~Alloc();
  std::allocator_traits<alloc_control_block_t>::deallocate(alloc_control_block,
                                                           this, 1);
}

template <typename T>
template <typename Alloc>
template <typename... Args>
SmartPointer<T>::ControlBlockMakeShared<Alloc>::ControlBlockMakeShared(
    const Alloc& alloc, Args&&... args)
    : object(std::forward<Args>(args)...), alloc(alloc) {}

template <typename T>
template <typename Alloc>
void SmartPointer<T>::ControlBlockMakeShared<Alloc>::use_deleter(void* ptr) {
  std::allocator_traits<Alloc>::destroy(alloc, reinterpret_cast<T*>(ptr));
}

template <typename T>
template <typename Alloc>
void SmartPointer<T>::ControlBlockMakeShared<Alloc>::destroy() {
  using alloc_control_block_t =
      typename std::allocator_traits<Alloc>::template rebind_alloc<
          typename SharedPtr<T>::template ControlBlockMakeShared<Alloc>>;
  alloc_control_block_t alloc_control_block = alloc;
  (&alloc)->~Alloc();
  std::allocator_traits<alloc_control_block_t>::deallocate(alloc_control_block,
                                                           this, 1);
}

template <typename T>
void SmartPointer<T>::increment_field(
    std::size_t(BaseControlBlock::*field)) const {
  if (control_block_ != nullptr) {
    ++(control_block_->*field);
  }
}

template <typename T>
void SmartPointer<T>::decrement_field(
    std::size_t(BaseControlBlock::*field)) const {
  if (control_block_ != nullptr) {
    --(control_block_->*field);
  }
}

template <typename T>
void SmartPointer<T>::increment_weak_count() const {
  increment_field(&BaseControlBlock::weak_count);
}

template <typename T>
void SmartPointer<T>::decrement_weak_count() const {
  decrement_field(&BaseControlBlock::weak_count);
}

template <typename T>
std::size_t SmartPointer<T>::use_count() const {
  return control_block_ == nullptr ? 0 : control_block_->count;
}

template <typename T>
template <typename Pointer>
void SmartPointer<T>::swap(Pointer& pointer) {
  std::swap(pointer.control_block_, control_block_);
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T, std::allocator<T>, Args...>(
      std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
SharedPtr<T>::SharedPtr(BaseControlBlock* control_block)
    : SmartPointer<T>(control_block) {}

template <typename T>
template <typename Y, typename Deleter>
SharedPtr<T>::SharedPtr(Y* ptr, Deleter deleter)
    : SharedPtr(ptr, deleter, std::allocator<T>()) {}

template <typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using control_block_t =
      typename SharedPtr<T>::template ControlBlockMakeShared<Alloc>;
  using alloc_control_block_t = typename std::allocator_traits<
      Alloc>::template rebind_alloc<control_block_t>;
  alloc_control_block_t alloc_control_block = alloc;
  auto control_block = std::allocator_traits<alloc_control_block_t>::allocate(
      alloc_control_block, 1);
  std::allocator_traits<alloc_control_block_t>::construct(
      alloc_control_block, control_block, alloc, std::forward<Args>(args)...);
  return SharedPtr<T>(static_cast<BaseControlBlock*>(control_block));
}

template <typename T>
template <typename Y, typename Deleter, typename Alloc>
SharedPtr<T>::SharedPtr(Y* ptr, Deleter deleter, Alloc alloc) {
  using control_block_t =
      typename SharedPtr<T>::template ControlBlockRegular<Deleter, Alloc>;
  using alloc_control_block_t = typename std::allocator_traits<
      Alloc>::template rebind_alloc<control_block_t>;
  alloc_control_block_t alloc_control_block = alloc;
  auto control_block = std::allocator_traits<alloc_control_block_t>::allocate(
      alloc_control_block, 1);
  new (control_block) control_block_t(ptr, deleter, alloc);
  SmartPointer<T>::control_block_ = control_block;
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    ptr->weak_ptr = *this;
  }
}

template <typename T>
template <typename Y>
SharedPtr<T>::SharedPtr(const SharedPtr<Y>& shared_ptr)
    : SmartPointer<T>(shared_ptr.control_block_) {
  SmartPointer<T>::increment_count();
}

template <typename T>
SharedPtr<T>::SharedPtr(const SharedPtr& shared_ptr)
    : SmartPointer<T>(shared_ptr.control_block_) {
  SmartPointer<T>::increment_count();
}

template <typename T>
template <typename Y>
SharedPtr<T>::SharedPtr(SharedPtr<Y>&& shared_ptr)
    : SmartPointer<T>(shared_ptr.control_block_) {
  shared_ptr.control_block_ = nullptr;
}

template <typename T>
SharedPtr<T>::SharedPtr(SharedPtr&& shared_ptr)
    : SmartPointer<T>(shared_ptr.control_block_) {
  shared_ptr.control_block_ = nullptr;
}

template <typename T>
template <typename Y>
SharedPtr<T>::SharedPtr(const WeakPtr<Y>& weak_ptr)
    : SmartPointer<T>(weak_ptr.control_block_) {
  SmartPointer<T>::increment_count();
}

template <typename T>
template <typename Y>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr<Y>& shared_ptr) noexcept {
  SharedPtr<T> copy(shared_ptr);
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(const SharedPtr& shared_ptr) noexcept {
  SharedPtr<T> copy(shared_ptr);
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
template <typename Y>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr<Y>&& shared_ptr) noexcept {
  SharedPtr<T> copy(std::move(shared_ptr));
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
SharedPtr<T>& SharedPtr<T>::operator=(SharedPtr&& shared_ptr) noexcept {
  SharedPtr<T> copy(std::move(shared_ptr));
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
template <typename Y>
void SharedPtr<T>::reset(Y* ptr) {
  SharedPtr(ptr).swap(*this);
}

template <typename T>
template <typename Y, typename Deleter>
void SharedPtr<T>::reset(Y* ptr, Deleter deleter) {
  SharedPtr(ptr, deleter).swap(*this);
}

template <typename T>
template <typename Y, typename Deleter, typename Alloc>
void SharedPtr<T>::reset(Y* ptr, Deleter deleter, Alloc alloc) {
  SharedPtr(ptr, deleter, alloc).swap(*this);
}

template <typename T>
SharedPtr<T>::operator bool() const noexcept {
  return SmartPointer<T>::use_count() != 0;
}

template <typename T>
T* SharedPtr<T>::get() const {
  if (SmartPointer<T>::control_block_ == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<T*>(SmartPointer<T>::control_block_->get_ptr());
}

template <typename T>
SharedPtr<T>::~SharedPtr() {
  SmartPointer<T>::decrement_count();
  if (SmartPointer<T>::control_block_ != nullptr and
      SmartPointer<T>::control_block_->count == 0) {
    SmartPointer<T>::control_block_->use_deleter(
        reinterpret_cast<void*>(get()));
    if (SmartPointer<T>::control_block_->weak_count == 0) {
      SmartPointer<T>::control_block_->destroy();
    }
  }
}

template <typename T>
template <typename Y>
WeakPtr<T>::WeakPtr(const WeakPtr<Y>& weak_ptr) noexcept
    : SmartPointer<T>(weak_ptr.control_block_) {
  SmartPointer<T>::increment_weak_count();
}

template <typename T>
WeakPtr<T>::WeakPtr(const WeakPtr& weak_ptr) noexcept
    : SmartPointer<T>(weak_ptr.control_block_) {
  SmartPointer<T>::increment_weak_count();
}

template <typename T>
template <typename Y>
WeakPtr<T>::WeakPtr(WeakPtr<Y>&& weak_ptr) noexcept
    : SmartPointer<T>(weak_ptr.control_block_) {
  weak_ptr.control_block_ = nullptr;
}

template <typename T>
WeakPtr<T>::WeakPtr(WeakPtr&& weak_ptr) noexcept
    : SmartPointer<T>(weak_ptr.control_block_) {
  weak_ptr.control_block_ = nullptr;
}

template <typename T>
template <typename Y>
WeakPtr<T>::WeakPtr(const SharedPtr<Y>& shared_ptr)
    : SmartPointer<T>(shared_ptr.control_block_) {
  SmartPointer<T>::increment_weak_count();
}

template <typename T>
template <typename Y>
WeakPtr<T>& WeakPtr<T>::operator=(const WeakPtr<Y>& weak_ptr) {
  WeakPtr<T> copy(weak_ptr);
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
template <typename Y>
WeakPtr<T>& WeakPtr<T>::operator=(WeakPtr<Y>&& weak_ptr) {
  WeakPtr<T> copy(std::move(weak_ptr));
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
template <typename Y>
WeakPtr<T>& WeakPtr<T>::operator=(const SharedPtr<Y>& shared_ptr) {
  WeakPtr<T> copy(shared_ptr);
  SmartPointer<T>::swap(copy);
  return *this;
}

template <typename T>
bool WeakPtr<T>::expired() const noexcept {
  return SmartPointer<T>::use_count() == 0;
}

template <typename T>
WeakPtr<T>::~WeakPtr() {
  SmartPointer<T>::decrement_weak_count();
  if (SmartPointer<T>::control_block_ != nullptr and
      SmartPointer<T>::control_block_->weak_count == 0 and
      SmartPointer<T>::control_block_->count == 0) {
    SmartPointer<T>::control_block_->destroy();
  }
}

template <typename T>
SharedPtr<T> WeakPtr<T>::lock() const noexcept {
  SmartPointer<T>::increment_count();
  return SharedPtr<T>(SmartPointer<T>::control_block_);
}
