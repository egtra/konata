/*
git_ptr.hpp: Copyright (c) Egtra 2016

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_COM_COMMON_HPP
#define KONATA_COM_GIT_PTR_HPP

#pragma once

#include <system_error>

namespace konata
{
namespace com
{

inline void throw_if_failed(HRESULT hr)
{
	if (FAILED(hr))
		throw std::system_error(static_cast<int>(hr), std::system_category());
}

} // namespace com
} // namespace konata

#endif // KONATA_COM_COMMON_HPP
