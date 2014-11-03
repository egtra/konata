/*
theme_dialog.hpp: Copyright (c) Egtra 2014

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_WINDOWS_THEME_DIALOG_HPP
#define KONATA_WINDOWS_THEME_DIALOG_HPP

#pragma once

#include <cstring>
#include <vector>
#include <utility> // pair

#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>

#include <atlgdi.h>

#pragma comment(lib, "uxtheme.lib")

#include <konata/windows/window_impl.hpp>

namespace konata
{
namespace windows
{

// http://msdn.microsoft.com/en-us/library/windows/desktop/ms645398%28v=vs.85%29.aspx
struct DLGTEMPLATEEX
{
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cDlgItems;
	std::int16_t x;
	std::int16_t y;
	std::int16_t cx;
	std::int16_t cy;
};

class DECLSPEC_NOVTABLE dialog_with_themed_font_impl
	: public dialog_impl
{
public:
	INT_PTR DoModal(_In_opt_ HINSTANCE hinst, _In_ PCTSTR templateName, _In_opt_ HWND hwndParent)
	{
		std::vector<BYTE> buffer = load_dialog_convert_font(hinst, templateName);
		if (buffer.empty())
			return dialog_impl::DoModal(hinst, templateName, hwndParent);
		else
			return dialog_impl::DoModal(hinst, reinterpret_cast<const DLGTEMPLATE*>(&buffer[0]), hwndParent);
	}
	INT_PTR DoModal(_In_opt_ HINSTANCE hinst, _In_ int resourceId, _In_opt_ HWND hwndParent)
	{
		return DoModal(hinst, MAKEINTRESOURCE(resourceId), hwndParent);
	}
	HWND Create(_In_opt_ HINSTANCE hinst, _In_ PCTSTR templateName, _In_opt_ HWND hwndParent)
	{
		std::vector<BYTE> buffer = load_dialog_convert_font(hinst, templateName);
		if (buffer.empty())
			return dialog_impl::Create(hinst, templateName, hwndParent);
		else
			return dialog_impl::Create(hinst, reinterpret_cast<const DLGTEMPLATE*>(&buffer[0]), hwndParent);
	}
	HWND Create(_In_opt_ HINSTANCE hinst, _In_ int resourceId, _In_opt_ HWND hwndParent)
	{
		return Create(hinst, MAKEINTRESOURCE(resourceId), hwndParent);
	}

private:
	static const WORD* skip_string_or_id(const WORD* p)
	{
		if (*p == 0xffff)
			return p + 2;
		while (*p++ != 0)
			;
		return p;
	}

	static const BYTE* find_font_size_field(const DLGTEMPLATEEX* dlg)
	{
		const WORD* p = reinterpret_cast<const WORD*>(dlg + 1);
		// skip menu and window class
		return reinterpret_cast<const BYTE*>(skip_string_or_id(skip_string_or_id(p)));
	}

	static const BYTE* find_dialog_item(const DLGTEMPLATEEX* dlg, const void* font)
	{
		const BYTE* p = static_cast<const BYTE*>(font);
		if ((dlg->style & DS_SETFONT) != 0)
			p += 2 + 2 + 1 + 1; // skip size, weight, italic, and charset
		std::uintptr_t q = reinterpret_cast<std::uintptr_t>(skip_string_or_id(reinterpret_cast<const WORD*>(p)));
		return reinterpret_cast<const BYTE*>((q + 3) & ~3);
	}

	static void push_back(_Inout_ std::vector<BYTE>& buffer, _In_ const void* src, _In_ std::size_t size)
	{
		const BYTE* p = reinterpret_cast<const BYTE*>(src);
		buffer.insert(buffer.end(), p, p + size);
	}

	static std::pair<const void*, DWORD> load_dialog_resource(_In_opt_ HMODULE hmod, _In_ PCTSTR name)
	{
		HRSRC hrsrc = ::FindResource(hmod, name, RT_DIALOG);
		DWORD size = ::SizeofResource(hmod, hrsrc);
		HGLOBAL hg = ::LoadResource(hmod, hrsrc);
		return std::make_pair(::LockResource(hg), size);
	}

	static int logfont_height_to_point(_In_ const LOGFONTW& lf)
	{
		WTL::CDC dc = ::CreateCompatibleDC(nullptr);
		std::int_fast64_t height;
		if (lf.lfHeight > 0)
		{
			// http://support.microsoft.com/kb/74299/en-us
			TEXTMETRIC tm = {};
			dc.GetTextMetrics(&tm);
			height = lf.lfHeight - tm.tmInternalLeading;
		}
		else
		{
			height = -lf.lfHeight;
		}
		return static_cast<int>(height * 72 / dc.GetDeviceCaps(LOGPIXELSY));
	}

	static std::vector<BYTE> dialog_template_convert_font(
		_In_ const DLGTEMPLATEEX* dialog, DWORD size,
		_In_ PCWSTR fontFamily, WORD point, WORD weight, BYTE italic, BYTE charSet)
	{
		// DLGTEMPLATE (Not DLGTEMPLATEEX) is not supported.
		if (dialog->signature != 0xffff)
			return std::vector<BYTE>();

		std::vector<BYTE> buffer;
		buffer.reserve(size + LF_FACESIZE);

		const void* p = dialog;

		const BYTE* font = find_font_size_field(dialog);

		buffer.insert(buffer.end(), reinterpret_cast<const BYTE*>(p), font);
		reinterpret_cast<DLGTEMPLATEEX*>(&buffer[0])->style |= DS_SETFONT;

		push_back(buffer, &point, sizeof (WORD));
		push_back(buffer, &weight, sizeof (WORD));
		buffer.push_back(italic);
		buffer.push_back(charSet);
		push_back(buffer, fontFamily, (std::wcslen(fontFamily) + 1) * sizeof (WCHAR));

		buffer.resize(buffer.size() + 3 & ~3);
		const BYTE* itemFirst = find_dialog_item(dialog, font);
		const BYTE* templateEnd = reinterpret_cast<const BYTE*>(p) + size;
		buffer.insert(buffer.end(), itemFirst, templateEnd);

		return buffer;
	}

	static bool get_theme_font(_Out_ LOGFONTW& lf)
	{
		HTHEME hTheme = ::OpenThemeData(nullptr, VSCLASS_TEXTSTYLE);
		if (hTheme == nullptr)
			return false;
		HRESULT hr = ::GetThemeFont(hTheme, nullptr, TEXT_CONTROLLABEL, 0, TMT_FONT, &lf);
		CloseThemeData(hTheme);
		return SUCCEEDED(hr);
	}

	static std::vector<BYTE> load_dialog_convert_font(_In_opt_ HMODULE hmod, PCTSTR resourceName)
	{
		std::pair<const void*, DWORD> res = load_dialog_resource(hmod, resourceName);
		if (res.first == nullptr)
			return std::vector<BYTE>();
		LOGFONTW lf;
		if (!get_theme_font(lf))
			return std::vector<BYTE>();
		return dialog_template_convert_font(
			static_cast<const DLGTEMPLATEEX*>(res.first),
			res.second,
			lf.lfFaceName,
			static_cast<WORD>(logfont_height_to_point(lf)),
			static_cast<WORD>(lf.lfWeight),
			lf.lfItalic,
			lf.lfCharSet);
	}
};

} // namespace windows
} // namespace konata

#endif // KONATA_WINDOWS_THEME_DIALOG_HPP
