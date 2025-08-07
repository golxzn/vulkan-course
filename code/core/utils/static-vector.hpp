#pragma once

#include <array>
#include <compare>
#include <iterator>

namespace vc::utils {

#pragma range iterators

template<class T>
class static_vector_iterator;

template<class T>
class static_vector_const_iterator {
	friend class static_vector_iterator<T>;
public:
	using self = static_vector_const_iterator<T>;
	using iterator_concept  = std::contiguous_iterator_tag;
	using iterator_category = std::random_access_iterator_tag;
	using value_type        = T;
	using difference_type   = ptrdiff_t;
	using pointer           = const T *;
	using reference         = const T &;

	constexpr static_vector_const_iterator() noexcept = default;
	constexpr explicit static_vector_const_iterator(pointer start) noexcept : m_current{ start } {}

	[[nodiscard]] constexpr auto operator*() const noexcept -> reference { return *m_current; }
	[[nodiscard]] constexpr auto operator->() const noexcept -> pointer { return *m_current; }
	[[nodiscard]] constexpr auto operator++() noexcept -> self & { ++m_current; return *this; }
	[[nodiscard]] constexpr auto operator--() noexcept -> self & { --m_current; return *this; }

	[[nodiscard]] constexpr auto operator++(int) noexcept -> self {
		auto tmp{ *this };
		++m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator--(int) noexcept -> self {
		auto tmp{ *this };
		--m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator+=(const difference_type offset) noexcept -> self & {
		m_current += offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-=(const difference_type offset) noexcept -> self & {
		m_current -= offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-(const self &other) const noexcept -> difference_type {
		return m_current - other.m_current;
	}

	[[nodiscard]] constexpr auto operator[](const difference_type index) const noexcept -> reference {
		return m_current[index];
	}

	[[nodiscard]] constexpr auto operator==(const self &other) const noexcept -> bool {
		return m_current == other.m_current;
	}

	[[nodiscard]] constexpr auto operator<=>(const self &other) const noexcept -> std::strong_ordering {
		return m_current <=> other.m_current;
	}

	[[nodiscard]] constexpr auto get() const noexcept -> pointer { return m_current; }

private:
	pointer m_current{};
};

template<class T>
class static_vector_iterator : public static_vector_const_iterator<T> {
public:
	using self = static_vector_iterator<T>;
	using super = static_vector_const_iterator<T>;
	using iterator_concept  = std::contiguous_iterator_tag;
	using iterator_category = std::random_access_iterator_tag;
	using value_type        = T;
	using difference_type   = ptrdiff_t;
	using pointer           = T *;
	using reference         = T &;


	[[nodiscard]] constexpr auto operator*() noexcept -> reference { return *m_current; }
	[[nodiscard]] constexpr auto operator->() noexcept -> pointer { return *m_current; }
	[[nodiscard]] constexpr auto operator++() noexcept -> self & { ++m_current; return *this; }
	[[nodiscard]] constexpr auto operator--() noexcept -> self & { --m_current; return *this; }

	[[nodiscard]] constexpr auto operator++(int) noexcept -> self {
		auto tmp{ *this };
		++m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator--(int) noexcept -> self {
		auto tmp{ *this };
		--m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator+=(const difference_type offset) noexcept -> self & {
		m_current += offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-=(const difference_type offset) noexcept -> self & {
		m_current -= offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-(const self &other) noexcept -> difference_type {
		return m_current - other.m_current;
	}

	[[nodiscard]] constexpr auto operator[](const difference_type index) noexcept -> reference {
		return m_current[index];
	}

	[[nodiscard]] constexpr auto operator==(const self &other) noexcept -> bool {
		return m_current == other.m_current;
	}
};


template<class T>
class static_vector_reverse_iterator;

template<class T>
class static_vector_const_reverse_iterator {
	friend class static_vector_reverse_iterator<T>;
public:
	using self = static_vector_const_reverse_iterator<T>;

	using iterator_concept  = std::contiguous_iterator_tag;
	using iterator_category = std::random_access_iterator_tag;
	using value_type        = T;
	using difference_type   = ptrdiff_t;
	using pointer           = const T *;
	using reference         = const T &;

	constexpr static_vector_const_reverse_iterator() noexcept = default;
	constexpr explicit static_vector_const_reverse_iterator(pointer start) noexcept : m_current{ start } {}

	[[nodiscard]] constexpr auto operator*() const noexcept -> reference { return *m_current; }
	[[nodiscard]] constexpr auto operator->() const noexcept -> pointer { return *m_current; }
	[[nodiscard]] constexpr auto operator++() noexcept -> self & { --m_current; return *this; }
	[[nodiscard]] constexpr auto operator--() noexcept -> self & { ++m_current; return *this; }

	[[nodiscard]] constexpr auto operator++(int) noexcept -> self {
		auto tmp{ *this };
		--m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator--(int) noexcept -> self {
		auto tmp{ *this };
		++m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator+=(const difference_type offset) noexcept -> self & {
		m_current -= offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-=(const difference_type offset) noexcept -> self & {
		m_current += offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-(const self &other) const noexcept -> difference_type {
		return m_current - other.m_current;
	}

	[[nodiscard]] constexpr auto operator[](const difference_type index) const noexcept -> reference {
		return m_current[index];
	}

	[[nodiscard]] constexpr auto operator==(const self &other) const noexcept -> bool {
		return m_current == other.m_current;
	}

	[[nodiscard]] constexpr auto operator<=>(const self &other) const noexcept -> std::strong_ordering {
		return m_current <=> other.m_current;
	}

	[[nodiscard]] constexpr auto get() const noexcept -> pointer { return m_current; }

private:
	pointer m_current{};
};

template<class T>
class static_vector_reverse_iterator : public static_vector_const_reverse_iterator<T> {
public:
	using self = static_vector_reverse_iterator<T>;
	using super = static_vector_const_reverse_iterator<T>;
	using iterator_concept  = std::contiguous_iterator_tag;
	using iterator_category = std::random_access_iterator_tag;
	using value_type        = T;
	using difference_type   = ptrdiff_t;
	using pointer           = T *;
	using reference         = T &;


	[[nodiscard]] constexpr auto operator*() noexcept -> reference { return *m_current; }
	[[nodiscard]] constexpr auto operator->() noexcept -> pointer { return *m_current; }
	[[nodiscard]] constexpr auto operator++() noexcept -> self & { --m_current; return *this; }
	[[nodiscard]] constexpr auto operator--() noexcept -> self & { ++m_current; return *this; }

	[[nodiscard]] constexpr auto operator++(int) noexcept -> self {
		auto tmp{ *this };
		--m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator--(int) noexcept -> self {
		auto tmp{ *this };
		++m_current;
		return *tmp;
	}

	[[nodiscard]] constexpr auto operator+=(const difference_type offset) noexcept -> self & {
		m_current -= offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-=(const difference_type offset) noexcept -> self & {
		m_current += offset;
		return *this;
	}

	[[nodiscard]] constexpr auto operator-(const self &other) noexcept -> difference_type {
		return m_current - other.m_current;
	}

	[[nodiscard]] constexpr auto operator[](const difference_type index) noexcept -> reference {
		return m_current[index];
	}

	[[nodiscard]] constexpr auto operator==(const self &other) noexcept -> bool {
		return m_current == other.m_current;
	}
};

#pragma endrange iterators


template<class T, size_t Length>
class static_vector {
public:
	using value_type      = T;
	using size_type       = size_t;
	using difference_type = ptrdiff_t;
	using pointer         = T *;
	using const_pointer   = const T *;
	using reference       = T &;
	using const_reference = const T &;

	using iterator       = static_vector_iterator<T>;
	using const_iterator = static_vector_const_iterator<T>;

	using reverse_iterator       = static_vector_reverse_iterator<T>;
	using const_reverse_iterator = static_vector_const_reverse_iterator<T>;

	constexpr static_vector() noexcept = default;

	constexpr static_vector(std::convertible_to<T> auto ...values) : m_size(std::min(sizeof...(values), Length)) {

	}

private:
	std::array<T, Length> m_data{};
	size_t m_size{};
};

} // namespace vc::utils
