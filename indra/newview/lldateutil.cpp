/** 
* @file lldateutil.cpp
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
* 
* Second Life Viewer Source Code
* The source code in this file ("Source Code") is provided by Linden Lab
* to you under the terms of the GNU General Public License, version 2.0
* ("GPL"), unless you have obtained a separate licensing agreement
* ("Other License"), formally executed by you and Linden Lab.  Terms of
* the GPL can be found in doc/GPL-license.txt in this distribution, or
* online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
* 
* There are special exceptions to the terms and conditions of the GPL as
* it is applied to this Source Code. View the full text of the exception
* in the file doc/FLOSS-exception.txt in this software distribution, or
* online at
* http://secondlifegrid.net/programs/open_source/licensing/flossexception
* 
* By copying, modifying or distributing this software, you acknowledge
* that you have read and understood your obligations described above,
* and agree to abide by those obligations.
* 
* ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
* WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
* COMPLETENESS OR PERFORMANCE.
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"

#include "lldateutil.h"

// Linden libraries
#include "lltrans.h"
#include "llui.h"

static S32 age_days_from_date(const std::string& date_string,
							  const LLDate& now)
{
	// Convert string date to malleable representation
	S32 month, day, year;
	S32 matched = sscanf(date_string.c_str(), "%d/%d/%d", &month, &day, &year);
	if (matched != 3) return S32_MIN;

	// Create ISO-8601 date string
	std::string iso8601_date_string =
		llformat("%04d-%02d-%02dT00:00:00Z", year, month, day);
	LLDate date(iso8601_date_string);

	// Correct for the fact that account creation dates are in Pacific time,
	// == UTC - 8
	F64 date_secs_since_epoch = date.secondsSinceEpoch();
	date_secs_since_epoch += 8.0 * 60.0 * 60.0;

	// Convert seconds from epoch to seconds from now
	F64 now_secs_since_epoch = now.secondsSinceEpoch();
	F64 age_secs = now_secs_since_epoch - date_secs_since_epoch;

	// We don't care about sub-day times
	const F64 SEC_PER_DAY = 24.0 * 60.0 * 60.0;
	S32 age_days = lltrunc(age_secs / SEC_PER_DAY);

	return age_days;
}

static S32 DAYS_PER_MONTH_NOLEAP[] =
	{ 31, 28, 21, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static S32 DAYS_PER_MONTH_LEAP[] =
	{ 31, 29, 21, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static S32 days_from_month(S32 year, S32 month)
{
	if (year % 4 == 0 
		&& year % 100 != 0)
	{
		// leap year
		return DAYS_PER_MONTH_LEAP[month];
	}
	else
	{
		return DAYS_PER_MONTH_NOLEAP[month];
	}
}

std::string LLDateUtil::ageFromDate(const std::string& date_string,
									const LLDate& now)
{
#define BAD_DATE_MATH 0
#if BAD_DATE_MATH
	S32 age_days = age_days_from_date(date_string, now);
	if (age_days == S32_MIN) return "???";
	S32 age_years = age_days / 365;
	age_days = age_days % 365;
	// *NOTE: This is wrong.  Not all months have 30 days, but we don't have a library
	// for relative date arithmetic. :-(  JC
	S32 age_months = age_days / 30;
	age_days = age_days % 30;
#else
	S32 born_month, born_day, born_year;
	S32 matched = sscanf(date_string.c_str(), "%d/%d/%d", &born_month, &born_day, &born_year);
	if (matched != 3) return "???";
	LLDate born_date;
	born_date.fromYMDHMS(born_year, born_month, born_day);
	F64 born_date_secs_since_epoch = born_date.secondsSinceEpoch();
	// Correct for the fact that account creation dates are in Pacific time,
	// == UTC - 8
	born_date_secs_since_epoch += 8.0 * 60.0 * 60.0;
	born_date.secondsSinceEpoch(born_date_secs_since_epoch);
	// explode out to month/day/year again
	born_date.split(&born_year, &born_month, &born_day);

	S32 now_year, now_month, now_day;
	now.split(&now_year, &now_month, &now_day);

	// Do grade-school subtraction, from right-to-left, borrowing from the left
	// when things go negative
	S32 age_days = (now_day - born_day);
	if (age_days < 0)
	{
		now_month -= 1;
		if (now_month == 0)
		{
			now_year -= 1;
			now_month = 12;
		}
		age_days += days_from_month(now_year, now_month);
	}
	S32 age_months = (now_month - born_month);
	if (age_months < 0)
	{
		now_year -= 1;
		age_months += 12;
	}
	S32 age_years = (now_year - born_year);
#endif

	// Noun pluralization depends on language
	std::string lang = LLUI::getLanguage();

	// Try for age in round number of years
	LLStringUtil::format_map_t args;

	if (age_months > 0 || age_years > 0)
	{
		args["[AGEYEARS]"] =
			LLTrans::getCountString(lang, "AgeYears", age_years);
		args["[AGEMONTHS]"] =
			LLTrans::getCountString(lang, "AgeMonths", age_months);

		// We want to display times like:
		// 2 year 2 months
		// 2 years (implicitly 0 months)
		// 11 months
		if (age_years > 0)
		{
			if (age_months > 0)
			{
				return LLTrans::getString("YearsMonthsOld", args);
			}
			else
			{
				return LLTrans::getString("YearsOld", args);
			}
		}
		else // age_years == 0
		{
			return LLTrans::getString("MonthsOld", args);
		}
	}
	// you're 0 months old, display in weeks or days

	// Now for age in weeks
	S32 age_weeks = age_days / 7;
	age_days = age_days % 7;
	if (age_weeks > 0)
	{
		args["[AGEWEEKS]"] = 
			LLTrans::getCountString(lang, "AgeWeeks", age_weeks);
		return LLTrans::getString("WeeksOld", args);
	}

	// Down to days now
	if (age_days > 0)
	{
		args["[AGEDAYS]"] =
			LLTrans::getCountString(lang, "AgeDays", age_days);
		return LLTrans::getString("DaysOld", args);
	}

	return LLTrans::getString("TodayOld");
}

std::string LLDateUtil::ageFromDate(const std::string& date_string)
{
	return ageFromDate(date_string, LLDate::now());
}
