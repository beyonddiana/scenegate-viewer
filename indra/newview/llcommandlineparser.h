/** 
 * @file llcommandlineparser.h
 * @brief LLCommandLineParser class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLCOMMANDLINEPARSER_H
#define LL_LLCOMMANDLINEPARSER_H

#include <boost/function/function1.hpp>

/** 
 * @class LLCommandLineParser
 * @brief Handle defining and parsing the command line.
 */
class LLCommandLineParser
{
public:
    typedef std::vector< std::string > token_vector_t;

    /**
     * @brief Add a value-less option to the command line description.
     * @param option_name The long name of the cmd-line option. 
     * @param description The text description of the option usage.
     */
    void addOptionDesc(
        const LLString& option_name, 
        boost::function1<void, const token_vector_t&> notify_callback = 0,
        unsigned int num_tokens = 0,
        const LLString& description = LLString::null,
        const LLString& short_name = LLString::null,
        bool composing = false,
        bool positional = false,
        bool last_option = false);


    /** 
     * @brief Parse the command line given by argc/argv.
     */
	bool parseCommandLine(int argc, char **argv);

    /** 
     * @brief Parse the command line contained by the given file.
     */
    bool parseCommandLineString(const std::string& str);

    /** 
     * @brief Parse the command line contained by the given file.
     */
    bool parseCommandLineFile(const std::basic_istream< char >& file);

    /** 
     * @brief Call callbacks associated with option descriptions.
     * 
     * Use this to handle the results of parsing. 
     */
    void notify();

    /** @brief Print a description of the configured options.
     *
     * Use this to print a description of options to the
     * given ostream. Useful for displaying usage info.
     */
    std::ostream& printOptionsDesc(std::ostream& os) const;

    /** @brief Manual option setting accessors.
     * 
     * Use these to retrieve get the values set for an option.
     * getOption will return an empty value if the option isn't
     * set. 
     */
    bool hasOption(const std::string& name) const;
    const token_vector_t& getOption(const std::string& name) const;

    void printOptions() const;
};

inline std::ostream& operator<<(std::ostream& out, const LLCommandLineParser& clp)
{
    return clp.printOptionsDesc(out);
}

class LLControlGroup; 

/** 
 * @class LLControlGroupCLP
 * @brief Uses the CLP to configure an LLControlGroup
 *
 * 
 */
class LLControlGroupCLP : public LLCommandLineParser
{
public:
    /**
     * @brief Configure the command line parser according the given config file.
     *
     * @param config_filename The name of the XML based LLSD config file. 
     * @param clp A reference to the command line parser object to configure.
     *
     * *FIX:Mani Specify config file format.
     */
    void configure(const LLString& config_filename, 
                   LLControlGroup* controlGroup);
};

#endif // LL_LLCOMMANDLINEPARSER_H
