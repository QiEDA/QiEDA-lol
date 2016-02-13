/*
 * This program source code file is part of QiEDA
 *
 * Copyright (C) 2016 QiEDA Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SEXPR_PARSER_H_
#define SEXPR_PARSER_H_

#include "sexpr/sexpr.h"
#include <string>
#include <vector>


namespace SEXPR
{
    class PARSER
    {
    public:
        PARSER();
        ~PARSER();
        SEXPR* Parse(const std::string &aString);
        SEXPR* ParseFromFile(const std::string &filename);
        static std::string GetFileContents(const std::string &filename);
    private:
        SEXPR* parseString(const std::string& aString, std::string::const_iterator& it);
        static const std::string whitespaceCharacters;
        int m_lineNumber;
        int m_lineOffset;
    };
}

#endif