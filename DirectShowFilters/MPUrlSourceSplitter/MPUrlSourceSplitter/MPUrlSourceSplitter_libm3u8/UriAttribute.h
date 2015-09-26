/*
    Copyright (C) 2007-2010 Team MediaPortal
    http://www.team-mediaportal.com

    This file is part of MediaPortal 2

    MediaPortal 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MediaPortal 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaPortal 2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef __URI_ATTRIBUTE_DEFINED
#define __URI_ATTRIBUTE_DEFINED

#include "Attribute.h"

#define URI_ATTRIBUTE_FLAG_NONE                                       ATTRIBUTE_FLAG_NONE

#define URI_ATTRIBUTE_FLAG_LAST                                       (ATTRIBUTE_FLAG_LAST + 0)

#define URI_ATTRIBUTE_NAME                                            L"URI"

class CUriAttribute : public CAttribute
{
public:
  CUriAttribute(HRESULT *result);
  virtual ~CUriAttribute(void);

  /* get methods */

  // gets URI
  // @return : URI or NULL if not specified
  const wchar_t *GetUri(void);

  /* set methods */

  /* other methods */

  // clears current instance
  virtual void Clear(void);

  // parses name and value
  // @param version : the playlist version
  // @param name : the name of attribute
  // @param value : the value of attribute
  // @return : true if successful, false otherwise
  virtual bool Parse(unsigned int version, const wchar_t *name, const wchar_t *value);

protected:

  // holds URI
  wchar_t *uri;

  /* methods */

  // creates attribute
  // @return : attribute or NULL if error
  virtual CAttribute *CreateAttribute(void);

  // deeply clones current instance to specified attribute
  // @param item : the attribute to clone current instance
  // @return : true if successful, false otherwise
  virtual bool CloneInternal(CAttribute *attribute);
};

#endif