/*****************************************************************************
 * NSColor+VLCAdditions.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * Authors: Felix Paul Kühne <fkuehne # videolan -dot- org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#import "NSColor+VLCAdditions.h"

@implementation NSColor (VLCAdditions)

+ (NSColor *)VLCAccentColor
{
    // VLC Redesign: the redesign uses VLC orange as its single accent colour,
    // regardless of the system accent colour.
    return [NSColor VLCOrangeElementColor];
}

+ (NSColor *)VLCOrangeElementColor
{
    // #FF8800
    return [NSColor colorWithRed:1. green:136./255. blue:0. alpha:1.];
}

+ (NSColor *)VLCSubtlerAccentColor
{
    return [NSColor.VLCAccentColor colorWithAlphaComponent:0.8];
}

+ (NSColor *)VLClibrarySubtitleColor
{
    return [NSColor colorWithRed:0.52 green:0.57 blue:0.61 alpha:1.];
}

+ (NSColor *)VLClibraryAnnotationColor
{
    return [NSColor whiteColor];
}

+ (NSColor *)VLClibraryAnnotationBackgroundColor
{
    return [NSColor colorWithRed:0. green:0. blue:0. alpha:.2];
}

+ (NSColor *)VLClibrarySeparatorLightColor
{
    return [NSColor colorWithRed:0.89 green:0.91 blue:0.93 alpha:1.];
}

+ (NSColor *)VLClibrarySeparatorDarkColor
{
    // #1E1E1E
    return [NSColor colorWithRed:30./255. green:30./255. blue:30./255. alpha:1.];
}

+ (NSColor *)VLClibraryProgressIndicatorBackgroundColor
{
    return [NSColor colorWithRed:37./255. green:41./255. blue:44./255. alpha:.8];
}

+ (NSColor *)VLCSliderFillColor
{
    return NSColor.VLCAccentColor;
}

+ (NSColor *)VLCSliderLightBackgroundColor
{
    return [NSColor colorWithCalibratedWhite:0.5 alpha:0.5];
}

+ (NSColor *)VLCSliderDarkBackgroundColor
{
    // #262626
    return [NSColor colorWithCalibratedWhite:38./255. alpha:1.0];
}

+ (NSColor *)VLCLightSubtleBorderColor
{
    return [NSColor colorWithCalibratedWhite:0.85 alpha:1.0];
}

+ (NSColor *)VLCDarkSubtleBorderColor
{
    // #2F2F2F
    return [NSColor colorWithCalibratedWhite:47./255. alpha:1.0];
}

+ (NSColor *)VLCSubtleBorderColor
{
    return [NSColor colorNamed:@"VLCSubtleBorderColor"];
}

+ (NSColor *)VLCActionButtonBackgroundColor
{
    return [NSColor.VLCAccentColor colorWithAlphaComponent:0.04];
}

+ (NSColor *)VLCActionButtonPressedBackgroundColor
{
    return [NSColor.VLCAccentColor colorWithAlphaComponent:0.16];
}

+ (NSColor *)VLCActionButtonPressedBorderColor
{
    return [NSColor.VLCAccentColor colorWithAlphaComponent:0.55];
}

+ (NSColor *)VLCDropTargetBackgroundColor
{
    return [NSColor.controlBackgroundColor colorWithAlphaComponent:0.5];
}

+ (NSColor *)VLCDropTargetHighlightColor
{
    return [NSColor.VLCAccentColor colorWithAlphaComponent:0.08];
}

+ (NSColor *)VLCDropTargetBorderColor
{
    return [NSColor.secondaryLabelColor colorWithAlphaComponent:0.5];
}

@end
