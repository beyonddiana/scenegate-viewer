/** 
 * @file llcurrencywrapper.h
 * @brief Currency wrapping class from the tea viewer helper library
 *
 * Copyright (C) 2012 arminweatherwax (at) lavabit.com
 * Copyright (C) 2015 Cinder Roxley <cinder@sdf.org>
 * You can use it under the following license:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef LL_CURRENCYWRAPPER_H
#define LL_CURRENCYWRAPPER_H

#include <string>
#include <boost/algorithm/string.hpp>

class LLCurrencyWrapper
{
public:
	static void setCurrency(const std::string& currency) { sCurrency = currency; }
	static std::string getCurrency() { return sCurrency; }
	static std::string wrapCurrency(const std::string& to_substitute);
	static void wrapCurrency(std::string& to_substitute);
	
private:
	LLCurrencyWrapper() {}
	~LLCurrencyWrapper() {}
	static std::string sCurrency;
};

#endif //LL_CURRENCYWRAPPER_H
