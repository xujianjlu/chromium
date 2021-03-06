// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_SCREEN_POSITION_CLIENT_H_
#define UI_AURA_SCREEN_POSITION_CLIENT_H_

#include "ui/aura/aura_export.h"
#include "ui/aura/window.h"

namespace gfx {
class Rect;
}

namespace aura {

class Event;
class RootWindow;
class Window;

namespace client {

// An interface implemented by an object that changes coordinates on a
// RootWindow into system coordinates.
class AURA_EXPORT ScreenPositionClient {
 public:
  // Converts the |screen_point| from a given |window|'s coordinate space
  // into screen coordinate space.
  virtual void ConvertPointToScreen(const Window* window,
                                    gfx::Point* point) = 0;
  virtual void ConvertPointFromScreen(const Window* window,
                                      gfx::Point* point) = 0;
  // Sets the bounds of the window. The implementation is responsible
  // for finding out and translating the right coordinates for the |window|.
  virtual void SetBounds(Window* window, const gfx::Rect& bounds) = 0;
  virtual ~ScreenPositionClient() {}
};

// Sets/Gets the activation client on the Window.
AURA_EXPORT void SetScreenPositionClient(RootWindow* window,
                                         ScreenPositionClient* client);
AURA_EXPORT ScreenPositionClient* GetScreenPositionClient(
    const RootWindow* window);

}  // namespace clients
}  // namespace aura

#endif  // UI_AURA_SCREEN_POSITION_CLIENT_H_
