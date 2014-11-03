/*
widnow_impl.hpp: Copyright (c) Egtra 2014

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_WINDOWS_WINDOW_IMPL_HPP
#define KONATA_WINDOWS_WINDOW_IMPL_HPP

#pragma once

#ifndef __ATLAPP_H__
#	define __ATLAPP_H__
#endif
#include <atlcrack.h>

#ifndef __ATLWIN_H__
#	define BEGIN_MSG_MAP(klass) BEGIN_MSG_MAP_EX(klass)
#	define END_MSG_MAP() } return FALSE; }
#	define CHAIN_MSG_MAP(klass) { if (klass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult)) return TRUE; }
#endif

namespace konata
{
namespace windows
{

class hwnd
{
public:
	hwnd() : m_hWnd() {}
	hwnd(_In_ HWND hwnd) : m_hWnd(hwnd) {}

	HWND get_handle() const { return m_hWnd; }
	operator HWND() const { return m_hWnd; }

	hwnd GetDlgItem(_In_ int id) const { return ::GetDlgItem(m_hWnd, id); }

protected:
	HWND m_hWnd;
};

class DECLSPEC_NOVTABLE window_proc_handler
{
public:
	// Compatible for ATL/WTL
	virtual BOOL ProcessWindowMessage(
		_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wp, _In_ LPARAM lp,
		_Inout_ LRESULT& lr, _In_ DWORD msgMapID = 0);
};

class DECLSPEC_NOVTABLE window_impl_base
	: public window_proc_handler
	, public hwnd
{
protected:
	window_impl_base() : m_pCurrentMsg(), m_isDestroyed() {}
	virtual ~window_impl_base() {}
	virtual BOOL DefWindowProc(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wp, _In_ LPARAM lp, _Inout_ LRESULT& lr) = 0;
	virtual void OnFinalMessage(_In_ HWND) {}

	static BOOL static_process_window_message(_In_ HWND hwnd, _In_ LONG_PTR index, _In_ UINT msg, _In_ WPARAM wp, _In_ LPARAM lp, _Inout_ LRESULT& lr)
	{
		if (window_impl_base* pthis = reinterpret_cast<window_impl_base*>(GetWindowLongPtr(hwnd, index)))
		{
			MSG currentMsg = { hwnd, msg, wp, lp };
			const MSG* old = pthis->m_pCurrentMsg;
			pthis->m_pCurrentMsg = &currentMsg;
			BOOL ret = pthis->ProcessWindowMessage(hwnd, msg, wp, lp, lr);
			if (!ret)
				ret = pthis->DefWindowProc(hwnd, msg, wp, lp, lr);
			if (msg == WM_NCDESTROY)
				pthis->m_isDestroyed = true;
			pthis->m_pCurrentMsg = old;
			if (old == nullptr && pthis->m_isDestroyed)
			{
				SetWindowLongPtr(hwnd, index, 0);
				pthis->m_hWnd = nullptr;
				pthis->m_isDestroyed = false;
				pthis->OnFinalMessage(hwnd);
			}
			return ret;
		}
		return FALSE;
	}

	const MSG* GetCurrentMessage() const { return m_pCurrentMsg; }
	const MSG* m_pCurrentMsg;

private:
	bool m_isDestroyed;
};

class DECLSPEC_NOVTABLE window_impl
	: public window_impl_base
{
public:
	static ATOM register_class(_Inout_ WNDCLASSEX& wcx)
	{
		wcx.lpfnWndProc = window_proc_entry;
		return RegisterClassEx(&wcx);
	}

	HWND Create(
		_In_ ATOM atom,
		_In_opt_ HINSTANCE hinst,
		_In_opt_ HWND hwndParent,
		_In_opt_ const RECT* prc = nullptr,
		_In_opt_ PCTSTR windowName = nullptr,
		_In_ DWORD style = 0,
		_In_ DWORD exStyle = 0,
		_In_opt_ HMENU hMenu = nullptr)
	{
		return ::CreateWindowEx(exStyle,
			reinterpret_cast<PCTSTR>(static_cast<UINT_PTR>(atom)),
			windowName,
			style,
			prc != nullptr ? prc->left : CW_USEDEFAULT,
			prc != nullptr ? prc->top : CW_USEDEFAULT,
			prc != nullptr ? prc->right - prc->left : CW_USEDEFAULT,
			prc != nullptr ? prc->bottom - prc->top : CW_USEDEFAULT,
			hwndParent,
			hMenu,
			hinst,
			this);
	}

protected:
	BOOL DefWindowProc(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wp, _In_ LPARAM lp, _Inout_ LRESULT& lr) override
	{
		lr = ::DefWindowProc(hwnd, msg, wp, lp);
		return TRUE;
	}

	static LRESULT CALLBACK window_proc_entry(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wp, _In_ LPARAM lp)
	{
		if (msg == WM_NCCREATE)
		{
			if (GetWindowLongPtr(hwnd, GWLP_USERDATA) == 0)
			{
				const CREATESTRUCT* pcs = reinterpret_cast<const CREATESTRUCT*>(lp);
				window_impl* pthis = static_cast<window_impl*>(pcs->lpCreateParams);

				SetWindowLongPtr(hwnd, GWLP_USERDATA,
					reinterpret_cast<LPARAM>(static_cast<window_impl_base*>(pthis)));

				if (pthis == nullptr)
					return FALSE;
				pthis->m_hWnd = hwnd;
			}
		}
		LRESULT lr = 0;
		if (static_process_window_message(hwnd, GWLP_USERDATA, msg, wp, lp, lr))
			return lr;
		assert(GetWindowLongPtr(hwnd, GWLP_USERDATA) == 0);
		return ::DefWindowProc(hwnd, msg, wp, lp);
	}

#if !defined _MSC_VER || _MSC_VER > 1800
	window_impl(const window_impl&) = delete;
	window_impl& operator=(const window_impl&) = delete;
#endif
};

class DECLSPEC_NOVTABLE dialog_impl : public window_impl_base
{
public:
	INT_PTR DoModal(_In_opt_ HINSTANCE hinst, _In_ PCTSTR templateName, _In_opt_ HWND hwndParent)
	{
		return DialogBoxParam(hinst, templateName, hwndParent, dialog_proc_entry, reinterpret_cast<LPARAM>(this));
	}
	INT_PTR DoModal(_In_opt_ HINSTANCE hinst, _In_ int resourceId, _In_opt_ HWND hwndParent)
	{
		return DialogBoxParam(hinst, MAKEINTRESOURCE(resourceId), hwndParent, dialog_proc_entry, reinterpret_cast<LPARAM>(this));
	}
	INT_PTR DoModal(_In_opt_ HINSTANCE hinst, _In_ const DLGTEMPLATE* dialogTemplate, _In_opt_ HWND hwndParent)
	{
		return DialogBoxIndirectParam(hinst, dialogTemplate, hwndParent, dialog_proc_entry, reinterpret_cast<LPARAM>(this));
	}
	HWND Create(_In_opt_ HINSTANCE hinst, _In_ PCTSTR templateName, _In_opt_ HWND hwndParent)
	{
		return CreateDialogParam(hinst, templateName, hwndParent, dialog_proc_entry, reinterpret_cast<LPARAM>(this));
	}
	HWND Create(_In_opt_ HINSTANCE hinst, _In_ int resourceId, _In_opt_ HWND hwndParent)
	{
		return CreateDialogParam(hinst, MAKEINTRESOURCE(resourceId), hwndParent, dialog_proc_entry, reinterpret_cast<LPARAM>(this));
	}
	HWND Create(_In_opt_ HINSTANCE hinst, _In_ const DLGTEMPLATE* dialogTemplate, _In_opt_ HWND hwndParent)
	{
		return CreateDialogIndirectParam(hinst, dialogTemplate, hwndParent, dialog_proc_entry, reinterpret_cast<LPARAM>(this));
	}

protected:
	BOOL DefWindowProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM, _Inout_ LRESULT&) override
	{
		return FALSE;
	}

private:
	static INT_PTR CALLBACK dialog_proc_entry(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wp, _In_ LPARAM lp)
	{
		if (msg == WM_INITDIALOG)
		{
			dialog_impl* pthis = reinterpret_cast<dialog_impl*>(lp);
			pthis->m_hWnd = hwnd;
			if (GetWindowLongPtr(hwnd, DWLP_USER) == 0)
			{
				SetWindowLongPtr(hwnd, DWLP_USER,
					reinterpret_cast<LONG_PTR>(static_cast<window_impl_base*>(pthis)));
			}
		}

		LRESULT lr = 0;
		if (static_process_window_message(hwnd, DWLP_USER, msg, wp, lp, lr))
			return SetDlgMsgResult(hwnd, msg, lr);
		return FALSE;
	}
};

} // namespace windows
} // namespace konata

#endif // KONATA_WINDOWS_WINDOW_IMPL_HPP
