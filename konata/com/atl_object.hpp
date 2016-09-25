/*
atl_object.hpp: Copyright (c) Egtra 2016

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_COM_ATL_OBJECT_HPP
#define KONATA_COM_ATL_OBJECT_HPP

#pragma once

#include <future>
#include <exception>
#include <utility>

#include <konata/com/common.hpp>

namespace konata
{
namespace com
{

template<typename T>
class atl_scoped_object : public T
{
public:
	template<typename... Args>
	atl_scoped_object(Args&&... args) : T(std::forward<Args>(args)...)
	{
		throw_if_failed(_AtlInitialConstruct());
		throw_if_failed(FinalConstruct());
	}

	~atl_scoped_object()
	{
		FinalRelease();
	}

	IFACEMETHOD_(ULONG, AddRef)() noexcept override
	{
		return 0;
	}

	IFACEMETHOD_(ULONG, Release)() noexcept override
	{
		return 0;
	}

	IFACEMETHOD(QueryInterface)(_In_ REFIID iid, _COM_Outptr_ void** ppv) noexcept override
	{
		return _InternalQueryInterface(iid, ppv);
	}
};

template<typename T>
class atl_unique_thread_creator
{
public:
	static HRESULT WINAPI CreateInstance(
		_In_opt_ void*,
		_In_ REFIID riid,
		_COM_Outptr_ void** ppv) noexcept
	{
		try
		{
			std::promise<IStream*> p;
			auto f = p.get_future();
			std::thread([p = std::move(p)]() mutable
			{
				thread_entry(p);
			}).detach();
			auto s = f.get();
			return CoGetInterfaceAndReleaseStream(s, riid, ppv);
		}
		catch (const std::system_error& e)
		{
			if (e.code().category() == std::system_category())
			{
				return e.code().value();
			}
			else
			{
				return E_FAIL;
			}
		}
		catch (const std::bad_alloc&)
		{
			return E_OUTOFMEMORY;
		}
		catch (...)
		{
			return E_FAIL;
		}
	}

private:
	static void thread_entry(_Inout_ std::promise<IStream*>& p)
	{
		ATL::ModuleLockHelper lock;
		auto hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hrInit))
		{
			p.set_exception(make_exception_ptr_from_hresult(hrInit));
			return;
		}
		try
		{
			thread_run(p);
		}
		catch (...)
		{
			try
			{
				p.set_exception(std::current_exception());
			}
			catch (...) // future_error
			{
			}
		}
		CoUninitialize();
	}

	static void thread_run(_Inout_ std::promise<IStream*>& p)
	{
		object_without_initialize obj;
		auto hrInit = obj.initialize();
		if (FAILED(hrInit))
		{
			p.set_exception(make_exception_ptr_from_hresult(hrInit));
			return;
		}
		IStream* s;
		auto hrMarshal = CoMarshalInterThreadInterfaceInStream(IID_IUnknown, obj.GetUnknown(), &s);
		if (FAILED(hrMarshal))
		{
			p.set_exception(make_exception_ptr_from_hresult(hrMarshal));
			return;
		}

		p.set_value(s);
		while (obj.m_dwRef != 0)
		{
			WaitMessage();
			MSG msg;
			while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					return;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	class object_without_initialize : public T
	{
	public:
		object_without_initialize() = default;

		~object_without_initialize()
		{
			::CoDisconnectObject(GetUnknown(), 0);
			FinalRelease();
		}

		HRESULT initialize()
		{
			auto hr = _AtlInitialConstruct();
			if (FAILED(hr))
			{
				return hr;
			}
			return FinalConstruct();
		}

		IFACEMETHOD_(ULONG, AddRef)() noexcept override
		{
			return InternalAddRef();
		}

		IFACEMETHOD_(ULONG, Release)() noexcept override
		{
			auto l = InternalRelease();
			if (l == 0)
			{
				PostQuitMessage(0);
			}
			return l;
		}

		IFACEMETHOD(QueryInterface)(_In_ REFIID iid, _COM_Outptr_ void** ppv) noexcept override
		{
			return _InternalQueryInterface(iid, ppv);
		}
	};

	static std::exception_ptr make_exception_ptr_from_hresult(HRESULT hr)
	{
		return std::make_exception_ptr(std::system_error(static_cast<int>(hr), std::system_category()));
	}
};

} // namespace com
} // namespace konata

#endif // KONATA_COM_ATL_OBJECT_HPP
