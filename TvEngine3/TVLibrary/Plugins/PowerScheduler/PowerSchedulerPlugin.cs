#region Copyright (C) 2007 Team MediaPortal
/* 
 *	Copyright (C) 2007 Team MediaPortal
 *	http://www.team-mediaportal.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#endregion

#region Usings
using System;
using System.Collections.Generic;
using System.Text;
using TvControl;
using SetupTv;
#endregion

namespace TvEngine.PowerScheduler
{
  public class PowerSchedulerPlugin : ITvServerPlugin
  {
    #region Variables
    /// <summary>
    /// Reference to the tvservice's TVcontroller
    /// </summary>
    IController _controller;
    #endregion

    #region Constructor
    /// <summary>
    /// Creates a new PowerSchedulerPlugin
    /// </summary>
    public PowerSchedulerPlugin()
    {
    }
    #endregion

    #region ITvServerPlugin implementation
    /// <summary>
    /// Called by the tvservice PluginLoader to start the PowerScheduler plugin
    /// </summary>
    /// <param name="controller">Reference to the tvservice's TVController</param>
    public void Start(IController controller)
    {
      _controller = controller;
      PowerScheduler.Instance.Start(controller);
    }

    /// <summary>
    /// Called by the tvservice PluginLoader to stop the PowerScheduler plugin
    /// </summary>
    public void Stop()
    {
      PowerScheduler.Instance.Stop();
    }
    /// <summary>
    /// Author of this plugin
    /// </summary>
    public string Author
    {
      get { return "micheloe"; }
    }
    /// <summary>
    /// Should this plugin run only on a master tvserver?
    /// </summary>
    public bool MasterOnly
    {
      get { return false; }
    }
    /// <summary>
    /// Name of this plugin
    /// </summary>
    public string Name
    {
      get { return "Power Scheduler"; }
    }
    /// <summary>
    /// Returns the SectionSettings setup part of this plugin
    /// </summary>
    public SectionSettings Setup
    {

      get
      {
        return new PowerSchedulerMasterSetup();
        if (_controller.IsMaster)
          return new PowerSchedulerMasterSetup();
        else
          // return new PowerSchedulerSlaveSetup();
          return new PowerSchedulerMasterSetup();
      }
    }
    /// <summary>
    /// Plugin version
    /// </summary>
    public string Version
    {
      get { return "0.1.0.0"; }
    }
    #endregion
  }
}
