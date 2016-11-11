﻿#region Copyright (C) 2005-2011 Team MediaPortal

// Copyright (C) 2005-2011 Team MediaPortal
// http://www.team-mediaportal.com
// 
// MediaPortal is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// MediaPortal is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with MediaPortal. If not, see <http://www.gnu.org/licenses/>.

#endregion

using System;
using System.Globalization;
using System.Windows.Forms;
using Mediaportal.TV.Server.Common.Types.Channel;

namespace Mediaportal.TV.Server.SetupControls.UserInterfaceControls
{
  public class MPChannelNumberUpDown : NumericUpDown
  {
    public MPChannelNumberUpDown()
    {
      DecimalPlaces = 3;
      Increment = 1;
      Minimum = 1;
      Maximum = ushort.MaxValue + 0.999M;  // 3 decimal places => 0.999
      Value = Minimum;
    }

    protected override void UpdateEditText()
    {
      if (DecimalPlaces == 0)
      {
        base.UpdateEditText();
        return;
      }

      // Minimise display of trailing zeroes, except when the user
      // intentionally adds them. "1.100" is different to "1.10", which is also
      // different to "1.1". However "1.1" is equivalent to "1.01" and "1.001".
      int separatorPositionText = Text.IndexOf(LogicalChannelNumber.SEPARATOR);
      char formatter = '#';
      int subChannelNumberDigitCount = DecimalPlaces;
      if (separatorPositionText >= 0 && Text.IndexOfAny("123456789".ToCharArray(), separatorPositionText) >= 0)
      {
        // The separator position in the value and text is not necessarily the
        // same. If you forget that, using the increment/decrement buttons can
        // add spurious trailing zeroes when a digit is added on the left of
        // the separator.
        string valueString = Value.ToString();
        int separatorPositionValueString = valueString.IndexOf(CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator);

        subChannelNumberDigitCount = Text.Length - separatorPositionText - LogicalChannelNumber.SEPARATOR.ToString().Length;
        if (separatorPositionValueString >= 0)
        {
          subChannelNumberDigitCount = Math.Max(subChannelNumberDigitCount, valueString.Length - separatorPositionValueString - CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator.Length);
        }
        subChannelNumberDigitCount = Math.Min(DecimalPlaces, subChannelNumberDigitCount);
        formatter = '0';
      }

      string newValue = Value.ToString("0." + new string(formatter, subChannelNumberDigitCount));
      Text = newValue.Replace(CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator, LogicalChannelNumber.SEPARATOR.ToString());
    }

    protected override void ValidateEditText()
    {
      // Convert the text into a value. We override because the LCN and decimal
      // separators may not match.
      decimal d;
      if (decimal.TryParse(Text.Replace(LogicalChannelNumber.SEPARATOR.ToString(), CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator), out d))
      {
        d = Math.Max(d, Minimum);
        d = Math.Min(d, Maximum);
        Value = d;
      }
      base.UserEdit = false;
      UpdateEditText();
    }

    protected override void OnTextBoxKeyPress(object source, KeyPressEventArgs e)
    {
      base.OnTextBoxKeyPress(source, e);

      // If the LCN and decimal separators are different, allow input of the
      // LCN separator and disallow input of the decimal separator.
      string separator = CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator;
      if (!string.Equals(separator, LogicalChannelNumber.SEPARATOR.ToString()))
      {
        if (e.KeyChar == LogicalChannelNumber.SEPARATOR)
        {
          e.Handled = false;
        }
        else if (string.Equals(e.KeyChar.ToString(), separator))
        {
          e.Handled = true;
        }
      }
    }
  }
}