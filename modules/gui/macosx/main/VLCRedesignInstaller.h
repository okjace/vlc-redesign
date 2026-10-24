/*****************************************************************************
 * VLCRedesignInstaller.h: self-installer for the VLC Redesign .app
 *****************************************************************************
 * VLC Redesign is distributed as a bare "VLC Redesign.app" (zipped). The first
 * time it is opened from outside an Applications folder (typically straight
 * from Downloads), it offers to install itself, the way Jace Launcher does:
 * copy into /Applications, clear the quarantine flag that makes macOS call
 * unsigned downloads "damaged", add Dock and Desktop shortcuts, then relaunch
 * the installed copy.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#import <Cocoa/Cocoa.h>

@interface VLCRedesignInstaller : NSObject

/* Shows the install offer when needed. Returns YES when the installed copy
 * has been launched and the caller should quit this one. */
+ (BOOL)offerInstallIfNeeded;

@end
