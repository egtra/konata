/*
handle_stream.hpp: Copyright (c) Egtra 2015

Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt )
*/

#ifndef KONATA_COM_HANDLE_STREAM_HPP
#define KONATA_COM_HANDLE_STREAM_HPP

#pragma once

#include <ole2.h>

namespace konata
{
namespace com
{

class handle_stream_impl : public IStream
{
public:
  IFACEMETHOD(Read)(
    _Out_writes_bytes_to_(cb, *pcbRead) void* pv,
    _In_ ULONG cb,
    _Out_opt_ ULONG* pcbRead) override
  {
    DWORD read;
    if (!ReadFile(m_h, pv, cb, &read, nullptr))
      return HRESULT_FROM_WIN32(GetLastError());
    if (pcbRead != nullptr)
      *pcbRead = read;
    return read < cb ? S_FALSE : S_OK;
  }
  IFACEMETHOD(Write)(
    _In_reads_bytes_(cb) const void* pv,
    _In_ ULONG cb,
    _Out_opt_ ULONG* pcbWritten) override
  {
    DWORD written;
    if (!WriteFile(m_h, pv, cb, &written, nullptr))
      return HRESULT_FROM_WIN32(GetLastError());
    if (pcbWritten != nullptr)
      *pcbWritten = written;
    return S_OK;
  }
  IFACEMETHOD(Seek)(
    LARGE_INTEGER /*dlibMove*/,
    DWORD /*dwOrigin*/,
    _Out_opt_ ULARGE_INTEGER* /*plibNewPosition*/) override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(SetSize)(
    ULARGE_INTEGER /*libNewSize*/) override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(CopyTo)(
    _In_ IStream* /*pstm*/,
    ULARGE_INTEGER /*cb*/,
    _Out_opt_ ULARGE_INTEGER* /*pcbRead*/,
    _Out_opt_ ULARGE_INTEGER* /*pcbWritten*/) override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(Commit)(DWORD /*grfCommitFlags*/) override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(Revert)() override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(LockRegion)(
    ULARGE_INTEGER /*libOffset*/,
    ULARGE_INTEGER /*cb*/,
    DWORD /*dwLockType*/) override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(UnlockRegion)(
    ULARGE_INTEGER /*libOffset*/,
    ULARGE_INTEGER /*cb*/,
    DWORD /*dwLockType*/) override
  {
    return E_NOTIMPL;
  }
  IFACEMETHOD(Stat)(
    _Out_ STATSTG* pstatstg,
    DWORD /*grfStatFlag*/) override
  {
    if (pstatstg != nullptr)
      *pstatstg = STATSTG();
    return E_NOTIMPL;
  }
  IFACEMETHOD(Clone)(_COM_Outptr_ IStream** ppstm) override
  {
    if (ppstm != nullptr)
      *ppstm = nullptr;
    return E_NOTIMPL;
  }

protected:
  explicit handle_stream_impl(HANDLE h = nullptr) : m_h(h) {}
  void set_handle(HANDLE h) noexcept { m_h = h; }

private:
  HANDLE m_h;
};

} // namespace com
} // namespace konata

#endif // KONATA_COM_HANDLE_STREAM_HPP
