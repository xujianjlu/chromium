// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_IMAGE_IMAGE_SKIA_UTIL_MAC_H_
#define UI_GFX_IMAGE_IMAGE_SKIA_UTIL_MAC_H_

#include "ui/base/ui_export.h"

#ifdef __LP64__
typedef CGSize NSSize;
#else
typedef struct _NSSize NSSize;
#endif

#ifdef __OBJC__
@class NSImage;
#else
class NSImage;
#endif

namespace gfx {
class ImageSkia;

// Converts to ImageSkia from NSImage.
UI_EXPORT gfx::ImageSkia ImageSkiaFromNSImage(NSImage* image);

// Resizes NSImage to |size| DIP and then converts to ImageSkia.
UI_EXPORT gfx::ImageSkia ImageSkiaFromResizedNSImage(NSImage* image,
                                                     NSSize size);

// Resizes |[NSImage imageNamed:@NSApplicationIcon]| to have edge width of
// |size| DIP and returns result as ImageSkia.
UI_EXPORT gfx::ImageSkia ApplicationIconAtSize(int size);

// Converts to NSImage from ImageSkia.
UI_EXPORT NSImage* NSImageFromImageSkia(const gfx::ImageSkia& image_skia);

}  // namespace gfx

#endif  // UI_GFX_IMAGE_IMAGE_SKIA_UTIL_MAC_H_
