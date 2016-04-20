/*
atl_object.hpp: Copyright (c) Egtra 2016

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_COM_ATL_OBJECT_HPP
#define KONATA_COM_ATL_OBJECT_HPP

#pragma once

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

} // namespace com
} // namespace konata

#endif // KONATA_COM_ATL_OBJECT_HPP
