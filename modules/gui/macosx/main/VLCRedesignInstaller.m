/*****************************************************************************
 * VLCRedesignInstaller.m: self-installer for the VLC Redesign .app
 *****************************************************************************
 * See VLCRedesignInstaller.h.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *****************************************************************************/

#import "VLCRedesignInstaller.h"

#include <dlfcn.h>
#include <stdlib.h>

static NSString *const kVLCRedesignSkipInstallOffer = @"VLCRedesignSkipInstallOffer";
static NSString *const kVLCRedesignAppName = @"VLC Redesign.app";

@implementation VLCRedesignInstaller

/* Where the bundle really is. Apps opened from a quarantined download run
 * from a randomized read-only "App Translocation" path; ask Security.framework
 * for the original location (same approach as Sparkle and LetsMove). */
+ (NSURL *)originalBundleURL
{
    NSURL *url = NSBundle.mainBundle.bundleURL;
    void *security = dlopen("/System/Library/Frameworks/Security.framework/Security", RTLD_LAZY);
    if (!security)
        return url;

    Boolean (*isTranslocated)(CFURLRef, bool *, CFErrorRef *) =
        (Boolean (*)(CFURLRef, bool *, CFErrorRef *))dlsym(security, "SecTranslocateIsTranslocatedURL");
    CFURLRef (*originalPath)(CFURLRef, CFErrorRef *) =
        (CFURLRef (*)(CFURLRef, CFErrorRef *))dlsym(security, "SecTranslocateCreateOriginalPathForURL");

    bool translocated = false;
    if (isTranslocated && originalPath
        && isTranslocated((__bridge CFURLRef)url, &translocated, NULL) && translocated) {
        CFURLRef original = originalPath((__bridge CFURLRef)url, NULL);
        if (original)
            url = CFBridgingRelease(original);
    }
    return url;
}

+ (BOOL)isInApplicationsFolder:(NSString *)path
{
    NSString *userApps = [NSHomeDirectory() stringByAppendingPathComponent:@"Applications/"];
    return [path hasPrefix:@"/Applications/"] || [path hasPrefix:userApps];
}

+ (int)run:(NSString *)tool arguments:(NSArray *)arguments
{
    NSTask *task = [[NSTask alloc] init];
    [task setLaunchPath:tool];
    [task setArguments:arguments];
    @try {
        [task launch];
        [task waitUntilExit];
        return [task terminationStatus];
    } @catch (NSException *e) {
        return -1;
    }
}

+ (NSString *)output:(NSString *)tool arguments:(NSArray *)arguments
{
    NSTask *task = [[NSTask alloc] init];
    NSPipe *pipe = [NSPipe pipe];
    [task setLaunchPath:tool];
    [task setArguments:arguments];
    [task setStandardOutput:pipe];
    [task setStandardError:[NSFileHandle fileHandleWithNullDevice]];
    @try {
        [task launch];
        NSData *data = [[pipe fileHandleForReading] readDataToEndOfFile];
        [task waitUntilExit];
        return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    } @catch (NSException *e) {
        return @"";
    }
}

/* Move to the Trash where available (macOS 10.8+); VLC 3 still targets 10.7. */
+ (BOOL)trash:(NSURL *)url error:(NSError **)error
{
    if (@available(macOS 10.8, *))
        return [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:error];
    return [[NSFileManager defaultManager] removeItemAtURL:url error:error];
}

+ (NSButton *)checkbox:(NSString *)title state:(BOOL)on y:(CGFloat)y
{
    NSButton *box = [[NSButton alloc] initWithFrame:NSMakeRect(0, y, 380, 22)];
    [box setButtonType:NSSwitchButton];
    [box setTitle:title];
    [box setState:on ? NSOnState : NSOffState];
    return box;
}

+ (void)showError:(NSString *)message
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"VLC Redesign couldn't be installed"];
    [alert setInformativeText:message];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

+ (BOOL)offerInstallIfNeeded
{
    if (getenv("VLC_REDESIGN_NO_SETUP"))
        return NO;

    NSURL *source = [self originalBundleURL];
    NSString *sourcePath = [source path];
    if (!sourcePath || [self isInApplicationsFolder:NSBundle.mainBundle.bundlePath]
        || [self isInApplicationsFolder:sourcePath])
        return NO;

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if ([defaults boolForKey:kVLCRedesignSkipInstallOffer])
        return NO;

    NSFileManager *fm = [NSFileManager defaultManager];
    NSString *systemApps = @"/Applications";
    NSString *targetDir = [fm isWritableFileAtPath:systemApps]
        ? systemApps : [NSHomeDirectory() stringByAppendingPathComponent:@"Applications"];
    NSString *target = [targetDir stringByAppendingPathComponent:kVLCRedesignAppName];
    BOOL upgrade = [fm fileExistsAtPath:target];
    BOOL fromDownloads = [sourcePath hasPrefix:[NSHomeDirectory() stringByAppendingPathComponent:@"Downloads/"]];

    NSView *options = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 380, 78)];
    NSButton *dock = [self checkbox:@"Add VLC Redesign to the Dock" state:YES y:54];
    NSButton *desktop = [self checkbox:@"Add a shortcut to the Desktop" state:YES y:28];
    NSButton *trash = [self checkbox:@"Move the downloaded copy to the Trash" state:fromDownloads y:2];
    [options addSubview:dock];
    [options addSubview:desktop];
    [options addSubview:trash];

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:upgrade ? @"Update VLC Redesign?" : @"Install VLC Redesign?"];
    [alert setInformativeText:[NSString stringWithFormat:
        @"VLC Redesign will be %@ %@, so it shows up in Launchpad and Spotlight and macOS stops "
        @"warning that it is \"damaged\".\n\n"
        @"VLC media player with the interface from “I Redesigned VLC After 25 Years” by "
        @"Juxtopposed. VLC is made by VideoLAN; this build is not affiliated with VideoLAN.",
        upgrade ? @"updated in" : @"installed to", targetDir]];
    [alert setAccessoryView:options];
    [alert addButtonWithTitle:upgrade ? @"Update" : @"Install"];
    [alert addButtonWithTitle:@"Not Now"];
    [alert setShowsSuppressionButton:YES];
    [[alert suppressionButton] setTitle:@"Don't ask again"];

    [NSApp activateIgnoringOtherApps:YES];
    NSInteger response = [alert runModal];
    if ([[alert suppressionButton] state] == NSOnState)
        [defaults setBool:YES forKey:kVLCRedesignSkipInstallOffer];
    if (response != NSAlertFirstButtonReturn)
        return NO;

    NSError *error = nil;
    [fm createDirectoryAtPath:targetDir withIntermediateDirectories:YES attributes:nil error:nil];
    if (upgrade && ![self trash:[NSURL fileURLWithPath:target] error:&error]
        && ![fm removeItemAtPath:target error:&error]) {
        [self showError:[NSString stringWithFormat:@"The old copy at %@ couldn't be replaced: %@",
                         target, [error localizedDescription]]];
        return NO;
    }
    if (![fm copyItemAtPath:sourcePath toPath:target error:&error]) {
        [self showError:[NSString stringWithFormat:@"Copying to %@ failed: %@",
                         targetDir, [error localizedDescription]]];
        return NO;
    }

    /* unsigned downloads are "damaged" while they carry the quarantine flag */
    [self run:@"/usr/bin/xattr" arguments:@[@"-dr", @"com.apple.quarantine", target]];

    if ([dock state] == NSOnState) {
        NSString *url = [[NSURL fileURLWithPath:target isDirectory:YES] absoluteString];
        NSString *current = [self output:@"/usr/bin/defaults"
                               arguments:@[@"read", @"com.apple.dock", @"persistent-apps"]];
        if ([current rangeOfString:url].location == NSNotFound) {
            NSString *tile = [NSString stringWithFormat:
                @"<dict><key>tile-data</key><dict><key>file-data</key><dict>"
                @"<key>_CFURLString</key><string>%@</string>"
                @"<key>_CFURLStringType</key><integer>15</integer>"
                @"</dict></dict></dict>", url];
            [self run:@"/usr/bin/defaults"
                arguments:@[@"write", @"com.apple.dock", @"persistent-apps", @"-array-add", tile]];
            [self run:@"/usr/bin/killall" arguments:@[@"Dock"]];
        }
    }

    if ([desktop state] == NSOnState) {
        NSString *link = [[NSHomeDirectory() stringByAppendingPathComponent:@"Desktop"]
                          stringByAppendingPathComponent:@"VLC Redesign"];
        [fm removeItemAtPath:link error:nil];
        [fm createSymbolicLinkAtPath:link withDestinationPath:target error:nil];
    }

    if ([trash state] == NSOnState)
        [self trash:source error:nil];

    /* start the installed copy and let the caller quit this one */
    if ([self run:@"/usr/bin/open" arguments:@[@"-n", target]] != 0) {
        [self showError:[NSString stringWithFormat:
            @"VLC Redesign was installed to %@ but couldn't be started. Open it from there.", targetDir]];
        return NO;
    }
    return YES;
}

@end
