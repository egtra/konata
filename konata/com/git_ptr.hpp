/*
git_ptr.hpp: Copyright (c) Egtra 2014

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_COM_GIT_PTR_HPP
#define KONATA_COM_GIT_PTR_HPP

#pragma once

#include <atomic>
#include <system_error>

#include <utility> // swap
#include <comdef.h>

namespace konata
{
namespace com
{

class git_ptr_base
{
protected:
	static void throw_if_failed(HRESULT hr)
	{
		if (FAILED(hr))
			throw std::system_error(static_cast<int>(hr), std::system_category());
	}

	static IGlobalInterfaceTablePtr get_git()
	{
		IGlobalInterfaceTablePtr ret;
		throw_if_failed(get_git_nothrow(ret));
		return ret;
	}

	static HRESULT get_git_nothrow(IGlobalInterfaceTablePtr& result) throw()
	{
		return CoCreateInstance(CLSID_StdGlobalInterfaceTable, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&result));
	}

	template<typename T>
	static DWORD register_git(T* p)
	{
		if (p == nullptr)
			return 0;
		DWORD ret;
		throw_if_failed(get_git()->RegisterInterfaceInGlobal(p, __uuidof(T), &ret));
		return ret;
	}

	static void revoke_git(DWORD cookie)
	{
		if (cookie == 0)
			return;
		IGlobalInterfaceTablePtr git;
		if (SUCCEEDED(get_git_nothrow(git)))
		{
			(void)git->RevokeInterfaceFromGlobal(cookie);
		}
	}

	template<typename T>
	static T* get_from_git(DWORD cookie)
	{
		if (cookie == 0)
			return nullptr;
		T* ret;
		throw_if_failed(get_git()->GetInterfaceFromGlobal(cookie, IID_PPV_ARGS(&ret)));
		return ret;
	}

	template<typename Interface>
	struct com_smart_ptr
	{
		_COM_SMARTPTR_TYPEDEF(Interface, __uuidof(Interface));
		typedef InterfacePtr type;
	};
};

template<typename T>
class git_ptr : protected git_ptr_base
{
public:
	git_ptr() throw() : m_cookie() {}
	git_ptr(_In_opt_ T* p) : m_cookie(register_git(p)) {}
	explicit git_ptr(DWORD cookie) throw() : m_cookie(cookie) {}

#if _MSC_VER >= 1600
	git_ptr(git_ptr&& y) throw() : m_cookie(y.release()) {}

	git_ptr& operator=(git_ptr&& y) throw()
	{
		reset(y.release());
		return *this;
	}
#endif

	git_ptr& operator=(_In_opt_ T* p)
	{
		reset(p);
		return *this;
	}

#if _MSC_VER >= 1800
	git_ptr(const git_ptr&) = delete;
	git_ptr& operator=(const git_ptr&) = delete;
#endif

	~git_ptr() { revoke_git(m_cookie); }

	void reset() throw() { revoke_git(release()); }
	void reset(_In_opt_ T* p) { reset(register_git(p)); }
	void reset(DWORD cookie) { git_ptr(cookie).swap(*this); }
	void swap(git_ptr& y) { std::swap(m_cookie, y.m_cookie); }

	DWORD release() throw()
	{
		DWORD ret = m_cookie;
		m_cookie = 0;
		return ret;
	}

#if _MSC_VER >= 1800
	explicit operator bool() const throw() { return m_cookie != 0; }
#endif
	typename com_smart_ptr<T>::type get() const throw() { return get_from_git<T>(m_cookie); }
	DWORD get_cookie() const throw() { return m_cookie; }

private:
	DWORD m_cookie;
};

template<typename T>
class atomic_git_ptr : protected git_ptr_base
{
public:
	atomic_git_ptr() throw() : m_cookie() {}
	atomic_git_ptr(_In_opt_ T* p) : m_cookie(register_git(p)) {}
	explicit atomic_git_ptr(DWORD cookie) throw() : m_cookie(cookie) {}
#if _MSC_VER >= 1600
	atomic_git_ptr(git_ptr<T>&& y) throw() : m_cookie(y.release()) {}

	atomic_git_ptr& operator=(git_ptr<T>&& y) throw()
	{
		reset(y.release());
		return *this;
	}
#endif

	atomic_git_ptr& operator=(_In_opt_ T* p)
	{
		reset(p);
		return *this;
	}

#if _MSC_VER >= 1800
	atomic_git_ptr(atomic_git_ptr&&) = delete;
	atomic_git_ptr(const atomic_git_ptr&) = delete;
	atomic_git_ptr& operator=(atomic_git_ptr&&) = delete;
	atomic_git_ptr& operator=(const atomic_git_ptr&) = delete;
#endif

	~atomic_git_ptr()
	{
		//revoke_git(m_cookie.non_atomic_load());
		revoke_git(m_cookie.load(std::memory_order_relaxed));
	}

	void reset() throw() { revoke_git(release()); }
	void reset(_In_opt_ T* p) { reset(register_git(p)); }
	void reset(DWORD cookie) { revoke_git(m_cookie.exchange(cookie)); }

#if _MSC_VER >= 1600
	git_ptr<T> exchange(git_ptr<T>&& desired) throw()
	{
		return git_ptr<T>(m_cookie.exchange(desired.release()));
	}
#endif

	DWORD release() throw() { return m_cookie.exchange(0); }

#if _MSC_VER >= 1800
	explicit operator bool() const throw() { return m_cookie != 0; }
#endif
	typename com_smart_ptr<T>::type get() const throw() { return get_from_git<T>(m_cookie); }
	DWORD get_cookie() const throw() { return m_cookie; }
	bool is_lock_free() const throw() { return m_cookie.is_lock_free(); }

private:
	std::atomic<DWORD> m_cookie;
};

template<typename T>
git_ptr<T> make_git(_In_ T* p)
{
	return git_ptr<T>(p);
}

} // namespace com
} // namespace konata

#endif // KONATA_COM_GIT_PTR_HPP
