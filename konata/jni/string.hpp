// konata/jni/string.hpp
//
// Copyright (c) Egtra 2011
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt )

#ifndef KONATA_JNI_STRING_HPP
#define KONATA_JNI_STRING_HPP

#include <memory>
#include <jni.h>

namespace konata
{
namespace jni
{

namespace detail {

template<typename CharT, void (JNIEnv::*ReleaseFunction)(jstring, const CharT*)>
struct string_chars_deleter
{
	typedef void result_type;

	string_chars_deleter(JNIEnv* e, jstring s) : env(e), str(s) {}
	string_chars_deleter(string_chars_deleter const& d) : env(d.env), str(d.str) {}
	string_chars_deleter& operator=(string_chars_deleter const& d)
	{
		env = d.env;
		str = d.str;
		return *this;
	}

	void operator()(CharT const* p) const
	{
		if (p != 0) (env->*ReleaseFunction)(str, p);
	}
private:
	JNIEnv* env;
	jstring str;
};

typedef string_chars_deleter<jchar, &JNIEnv::ReleaseStringChars> release_string_chars_deleter;
typedef string_chars_deleter<char, &JNIEnv::ReleaseStringUTFChars> release_string_utf_chars_deleter;
typedef string_chars_deleter<jchar, &JNIEnv::ReleaseStringCritical> release_string_critical_deleter;

} // namespace detail

typedef std::unique_ptr<const jchar[], detail::release_string_chars_deleter> scoped_chars_ptr;

inline scoped_chars_ptr GetStringChars(JNIEnv* env, jstring str, jboolean* isCopy)
{
	return scoped_chars_ptr(
		env->GetStringChars(str, isCopy),
		detail::release_string_chars_deleter(env, str));
}

typedef std::unique_ptr<const char[], detail::release_string_utf_chars_deleter> scoped_utf_chars_ptr;

scoped_utf_chars_ptr GetStringUTFChars(JNIEnv* env, jstring str, jboolean* isCopy)
{
	return scoped_utf_chars_ptr(
		env->GetStringUTFChars(str, isCopy),
		detail::release_string_utf_chars_deleter(env, str));
}

typedef std::unique_ptr<const jchar[], detail::release_string_critical_deleter> scoped_critical_ptr;

inline scoped_critical_ptr GetStringCritical(JNIEnv* env, jstring str, jboolean* isCopy)
{
	return scoped_critical_ptr(
		env->GetStringCritical(str, isCopy),
		detail::release_string_critical_deleter(env, str));
}

} // namespace jni
} // namspace konata

#endif KONATA_JNI_STRING_HPP
