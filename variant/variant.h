#pragma once
#include <algorithm>
#include <type_traits>
#include <variant>
#include <cstddef>
#include <iostream>

/// Meta functions
template <typename Head, typename... Tail>
struct GetMaxSizeOf {
  constexpr static const std::size_t value = std::max(sizeof(Head), GetMaxSizeOf<Tail...>::value);
};

template <typename Head>
struct GetMaxSizeOf<Head> {
  constexpr static const std::size_t value = sizeof(Head);
};

template <typename... Types>
constexpr static const std::size_t GetMaxSizeOf_v = GetMaxSizeOf<Types...>::value;

template <typename Head, typename... Tail>
struct GetMaxAlignOf {
  constexpr static const std::size_t value = std::max(alignof(Head), GetMaxAlignOf<Tail...>::value);
};

template <typename Head>
struct GetMaxAlignOf<Head> {
  constexpr static const std::size_t value = alignof(Head);
};

template <typename... Types>
constexpr static const std::size_t GetMaxAlignOf_v = GetMaxAlignOf<Types...>::value;

template <std::size_t N, typename Head, typename... Tail>
struct GetTypeByIndex {
  using type = typename GetTypeByIndex<N - 1, Tail...>::type;
};

template <typename Head, typename... Tail>
struct GetTypeByIndex<0, Head, Tail...> {
  using type = Head;
};

template <std::size_t N, typename... Types>
using GetTypeByIndex_t = typename GetTypeByIndex<N, Types...>::type;

template <typename T, std::size_t CurrentIndex, typename Head, typename... Tail>
struct GetIndexByTypeHelper {
  constexpr static const std::size_t value =
      []{
        if constexpr (!std::is_same_v<T, Head>) {
          return GetIndexByTypeHelper<T, CurrentIndex + 1, Tail...>::value;
        }
        return CurrentIndex;
      }();
};

template <typename T, std::size_t CurrentIndex, typename Head>
struct GetIndexByTypeHelper<T, CurrentIndex, Head> {
  static_assert(std::is_same_v<T, Head>);
  constexpr static const std::size_t value = CurrentIndex;
};

template <typename T, typename... Types>
constexpr static const std::size_t GetIndexByType_v = GetIndexByTypeHelper<T, 0, Types...>::value;

template <typename T, typename Head, typename... Tail>
struct GetCountEntry {
  constexpr static const std::size_t value = (std::is_same_v<T, Head> ? 1 : 0) + GetCountEntry<T, Tail...>::value;
};

template <typename T, typename Head>
struct GetCountEntry<T, Head> {
  constexpr static const std::size_t value = (std::is_same_v<T, Head> ? 1 : 0);
};

template <typename T, typename... Types>
constexpr static const bool IsOneType_v = GetCountEntry<T, Types...>::value == 1;

template <typename T, typename Head, typename... Tail>
constexpr static const bool IsAnyAssignable_v = std::is_assignable_v<Head, T> || IsAnyAssignable_v<T, Tail...>;

template <typename T, typename Head>
constexpr static const bool IsAnyAssignable_v<T, Head> = std::is_assignable_v<Head, T>;

/// Variant implementation
template <typename... Types>
class Variant;

template <typename... Types>
struct VariantStorage {
  alignas(GetMaxAlignOf_v<Types...>) std::byte buffer_[GetMaxSizeOf_v<Types...>];
  std::size_t index_ = 0;
};

template <typename T, typename... Types>
class VariantChoice {
 private:
  using Derived = Variant<Types...>;

  template <typename U>
  Derived& Assign(U&& value);
  Derived& GetDerived() { return static_cast<Derived&>(*this); }
  template <typename U>
  void Construct(U&& value);

 public:
  VariantChoice() {}
  template <typename U>
  requires std::is_constructible_v<T, U> && std::is_convertible_v<U, T>
  VariantChoice(U&& value) { Construct(std::forward<U>(value));}
  VariantChoice(const T& value) { Construct(value); }
  VariantChoice(std::remove_const_t<T>&& value) { Construct(std::move(value)); }

  Derived& operator=(const T& value) { return Assign(value); }
  Derived& operator=(T&& value) { return Assign(std::move(value)); }
  template <typename U>
  requires std::is_convertible_v<U, T> && std::is_assignable_v<T, U> && std::is_constructible_v<T, U>
  Derived& operator=(U&& value) { return Assign(std::forward<U>(value)); }

  void Destroy();
  template <typename UVariant>
  void ConstructVariant(UVariant&& variant);
  template <typename UVariant>
  void AssignVariant(UVariant&&  variant);
};

template <std::size_t N, typename... Types>
GetTypeByIndex_t<N, Types...>& get(Variant<Types...>& variant);

template <typename... Types>
class Variant : private VariantStorage<Types...>, private VariantChoice<Types, Types...>... {
   using VariantStorage<Types...>::index_;
   using VariantStorage<Types...>::buffer_;

   template <typename T, typename... Ts>
   friend class VariantChoice;
   template <std::size_t N, typename... Ts>
   friend GetTypeByIndex_t<N, Ts...>& get(Variant<Ts...>& variant);

   void Destroy();

 public:
  using VariantChoice<Types, Types...>::VariantChoice...;
  using VariantChoice<Types, Types...>::operator=...;

  constexpr static const std::size_t size = sizeof...(Types);
  template <std::size_t I>
  using T_i = GetTypeByIndex_t<I, Types...>;

  Variant();
  Variant(Variant&& variant);
  Variant(const Variant& variant);
  Variant& operator=(const Variant& variant);
  Variant& operator=(Variant&& variant);

  std::size_t index() const { return index_; }
  bool valueless_by_exception() const noexcept { return index_ == sizeof...(Types); }
  template <typename T, typename... Args>
  requires IsOneType_v<T, Types...> && std::is_constructible_v<T, Args...>
  T& emplace(Args&&... args);
  template <typename T, typename U, typename... Args>
  requires IsOneType_v<T, Types...> && std::is_constructible_v<T, std::initializer_list<U>, Args...>
  T& emplace(std::initializer_list<U> list, Args&&... args);
  template <size_t N, typename... Args>
  GetTypeByIndex_t<N, Types...>& emplace(Args&&... args) { return emplace<GetTypeByIndex_t<N, Types...>>(std::forward<Args>(args)...); }

  ~Variant() { Destroy(); }
};

template <typename... Types>
void Variant<Types...>::Destroy() {
  (VariantChoice<Types, Types...>::Destroy(), ...);
  index_ = sizeof...(Types);
}

template <typename T, typename... Types>
template <typename U>
void VariantChoice<T, Types...>::Construct(U&& value) {
  new (std::launder(GetDerived().buffer_)) T(std::forward<U>(value));
  GetDerived().index_ = GetIndexByType_v<T, Types...>;
}

template <typename T, typename... Types>
void VariantChoice<T, Types...>::Destroy() {
  if (GetDerived().index_ == GetIndexByType_v<T, Types...>) {
    reinterpret_cast<T*>(std::launder(GetDerived().buffer_))->~T();
  }
}

template <typename T, typename... Types>
template <typename U>
typename VariantChoice<T, Types...>::Derived&
VariantChoice<T, Types...>::Assign(U&& value) {
  if (GetDerived().index_ == GetIndexByType_v<T, Types...>) {
    *reinterpret_cast<T*>(GetDerived().buffer_) = std::forward<U>(value);
  } else {
    GetDerived().Destroy();
    Construct(std::forward<U>(value));
  }
  return GetDerived();
}

template <std::size_t N, typename... Types>
const GetTypeByIndex_t<N, Types...>& get(const Variant<Types...>& variant) {
  return get<N>(const_cast<Variant<Types...>&>(variant));
}

template <std::size_t N, typename... Types>
GetTypeByIndex_t<N, Types...>& get(Variant<Types...>& variant) {
  static_assert(N < sizeof...(Types));
  if (variant.index() != N) {
    throw std::bad_variant_access();
  }
  return *reinterpret_cast<GetTypeByIndex_t<N, Types...>*>(variant.buffer_);
}

template <std::size_t N, typename... Types>
GetTypeByIndex_t<N, Types...>&& get(Variant<Types...>&& variant) { return std::move(get<N>(variant)); }

template <std::size_t N, typename... Types>
const GetTypeByIndex_t<N, Types...>&& get(const Variant<Types...>&& variant) { return std::move(get<N>(variant)); }

template <typename T, typename... Types>
T& get(Variant<Types...>& variant) {
  return get<GetIndexByType_v<T, Types...>, Types...>(variant);
}

template <typename T, typename... Types>
const T& get(const Variant<Types...>& variant) {
  return get<GetIndexByType_v<T, Types...>, Types...>(variant);
}

template <typename T, typename... Types>
T&& get(Variant<Types...>&& variant) {
  return get<GetIndexByType_v<T, Types...>, Types...>(std::move(variant));
}

template <typename T, typename... Types>
const T&& get(const Variant<Types...>&& variant) {
  return get<GetIndexByType_v<T, Types...>, Types...>(std::move(variant));
}

template <typename... Types>
template <typename T, typename... Args>
requires IsOneType_v<T, Types...> && std::is_constructible_v<T, Args...>
T& Variant<Types...>::emplace(Args&&... args) {
  Destroy();
  new (std::launder(buffer_)) T(std::forward<Args>(args)...);
  index_ = GetIndexByType_v<T, Types...>;
  return get<T>(*this);
}

template <typename... Types>
template <typename T, typename U, typename... Args>
requires IsOneType_v<T, Types...> && std::is_constructible_v<T, std::initializer_list<U>, Args...>
T& Variant<Types...>::emplace(std::initializer_list<U> list, Args&&... args) {
  Destroy();
  new (std::launder(buffer_)) T(list, std::forward<Args>(args)...);
  index_ = GetIndexByType_v<T, Types...>;
  return get<T>(*this);
}

template <typename... Types>
Variant<Types...>::Variant() {
  new (buffer_) GetTypeByIndex_t<0, Types...>();
}

template <typename... Types>
Variant<Types...>::Variant(const Variant& variant) {
  (VariantChoice<Types, Types...>::ConstructVariant(variant), ...);
  index_ = variant.index_;
}

template <typename... Types>
Variant<Types...>& Variant<Types...>::operator=(const Variant& variant) {
  if (variant.index_ == index_) {
    (VariantChoice<Types, Types...>::AssignVariant(variant), ...);
  } else {
    Destroy();
    (VariantChoice<Types, Types...>::ConstructVariant(variant), ...);
  }
  index_ = variant.index_;
  return *this;
}

template <typename... Types>
Variant<Types...>& Variant<Types...>::operator=(Variant&& variant) {
  if (variant.index_ == index_) {
    (VariantChoice<Types, Types...>::AssignVariant(std::move(variant)), ...);
  } else {
    Destroy();
    (VariantChoice<Types, Types...>::ConstructVariant(std::move(variant)), ...);
  }
  index_ = variant.index_;
  return *this;
}

template <typename... Types>
Variant<Types...>::Variant(Variant&& variant) {
  (VariantChoice<Types, Types...>::ConstructVariant(std::move(variant)), ...);
  index_ = variant.index_;
}

template <typename T, typename... Types>
template <typename UVariant>
void VariantChoice<T, Types...>::ConstructVariant(UVariant&& variant) {
  if (variant.index() == GetIndexByType_v<T, Types...>) {
    new (std::launder(GetDerived().buffer_)) T(get<T>(std::forward<UVariant>(variant)));
  }
}

template <typename T, typename... Types>
template <typename UVariant>
void VariantChoice<T, Types...>::AssignVariant(UVariant&& variant) {
  if (variant.index() == GetIndexByType_v<T, Types...>) {
    if (GetDerived().index() == GetIndexByType_v<T, Types...>) {
      *reinterpret_cast<T*>(std::launder(GetDerived().buffer_)) = get<T>(std::forward<UVariant>(variant));
    }
  }
}

template <typename T, typename... Types>
bool holds_alternative(const Variant<Types...>& variant) noexcept {
  return variant.index() == GetIndexByType_v<T, Types...>;
}

/// Visit implementation

template <std::size_t Result, typename Var, typename... Variants, std::size_t HeadInd, std::size_t... TailInd>
constexpr std::size_t GetNumb(const std::index_sequence<HeadInd, TailInd...>&) {
  if constexpr (sizeof...(TailInd) > 0) {
    return GetNumb<Result * std::decay_t<Var>::size + HeadInd, Variants...>(std::index_sequence<TailInd...>());
  }
  return Result * std::decay_t<Var>::size + HeadInd;
}

template <typename Sequence, typename... Variants>
constexpr static const std::size_t GetNumb_v = GetNumb<0, Variants...>(Sequence());

template <std::size_t Ind, std::size_t... Indexes>
constexpr std::index_sequence<Ind, Indexes...> MakeSequence(const std::index_sequence<Indexes...>&) { return std::index_sequence<Ind, Indexes...>(); }

template <typename Var, typename... Variants>
struct GetProductSizes {
  constexpr static const std::size_t value = std::decay_t<Var>::size * GetProductSizes<Variants...>::value;
};

template <typename Var>
struct GetProductSizes<Var> {
  constexpr static const std::size_t value = std::decay_t<Var>::size;
};

template <typename... Variants>
constexpr static const std::size_t GetProductSizes_v = GetProductSizes<Variants...>::value;

template <std::size_t Numb, typename Var, typename... Variants>
struct GetNextSequenceHelper {
  constexpr static const std::size_t current_ind = Numb / GetProductSizes_v<Variants...>;
  constexpr static const auto sequence = MakeSequence<current_ind>(GetNextSequenceHelper<Numb % GetProductSizes_v<Variants...>, Variants...>::sequence);
};

template <std::size_t Numb, typename Var>
struct GetNextSequenceHelper<Numb, Var> {
  constexpr static const auto sequence = std::index_sequence<Numb>();
};

template <typename Sequence, typename... Variants>
struct GetNextSequence {
  constexpr static const std::size_t number = GetNumb_v<Sequence, Variants...> + 1;
  constexpr static const auto value = GetNextSequenceHelper<number, Variants...>::sequence;
};

template <std::size_t N>
constexpr auto GetZeros() {
  if constexpr (N != 1) {
    return MakeSequence<0>(GetZeros<N - 1>());
  }
  if constexpr (N == 1) {
    return std::index_sequence<0>();
  }
}

template <typename... Variants, std::size_t... Indexes>
constexpr bool GetIsFinal(const std::index_sequence<Indexes...>&) {
  return ((Indexes + 1 == std::decay_t<Variants>::size) && ...);
}

template <typename Sequence, typename... Variants>
struct IsFinal {
  constexpr static const bool value = GetIsFinal<Variants...>(Sequence());
};

template <typename Visitor, std::size_t... Indexes, typename... Variants>
auto visitImpl(Visitor&& visitor, const std::index_sequence<Indexes...>&, Variants&&... vars) {
  if constexpr (!IsFinal<std::index_sequence<Indexes...>, Variants...>::value) {
    if (((Indexes == vars.index()) && ...)) {
      return std::invoke(std::forward<Visitor>(visitor), get<Indexes>(std::forward<Variants>(vars))...);
    } else {
      return visitImpl(std::forward<Visitor>(visitor), GetNextSequence<std::index_sequence<Indexes...>, Variants...>::value, std::forward<Variants>(vars)...);
    }
  }
  return std::invoke(std::forward<Visitor>(visitor), get<Indexes>(std::forward<Variants>(vars))...);
}

template <typename Visitor, typename... Variants>
auto visit(Visitor&& visitor, Variants&&... vars) {
  return visitImpl(std::forward<Visitor>(visitor), GetZeros<sizeof...(Variants)>(), std::forward<Variants>(vars)...);
}