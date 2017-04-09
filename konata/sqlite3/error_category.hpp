/*
sqlite3/error_category.hpp: Copyright (c) Egtra 2015-2017

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_SQLITE3_ERROR_CATEGORY_HPP
#define KONATA_SQLITE3_ERROR_CATEGORY_HPP

#pragma once

#include <system_error>
#include "sqlite3.h"

namespace konata
{
namespace sqlite3
{

inline const std::error_category& sqlite3_error_category()
{
	class sqlite3_error_category : public std::error_category
	{
		const char* name() const noexcept override
		{
			return "sqlite3";
		}

		std::string message(int ev) const override
		{
			if (const char* s = sqlite3_errstr(ev))
				return s;
			else
				return "unknown error";
		}

		std::error_condition default_error_condition(int ev) const noexcept override
		{
			switch (ev)
			{
				case 0: return std::error_condition(); // SQLITE_OK
				case SQLITE_PERM: return std::error_condition(std::errc::permission_denied); // EACCES
				case SQLITE_ABORT: return std::error_condition(std::errc::operation_canceled); //ECANCELED
				case SQLITE_BUSY: return std::error_condition(std::errc::device_or_resource_busy); // EBUSY
				case SQLITE_NOMEM: return std::error_condition(std::errc::not_enough_memory); // ENOMEM
				case SQLITE_READONLY: return std::error_condition(std::errc::permission_denied); // EACCES
				case SQLITE_INTERRUPT: return std::error_condition(std::errc::operation_canceled); //ECANCELED
				case SQLITE_IOERR: return std::error_condition(std::errc::io_error); // EIO
				case SQLITE_FULL: return std::error_condition(std::errc::no_space_on_device); // ENOSPC
				case SQLITE_TOOBIG: return std::error_condition(std::errc::invalid_argument); // EINVAL
				case SQLITE_AUTH: return std::error_condition(std::errc::permission_denied); // EACCES
				case SQLITE_RANGE: return std::error_condition(std::errc::invalid_argument); // EINVAL
				default: return std::error_condition(ev, sqlite3_error_category());
			}
		}
	};

	static const sqlite3_error_category category;
	return category;
}
} // namespace com
} // namespace konata

#endif // KONATA_SQLITE3_ERROR_CATEGORY_HPP
