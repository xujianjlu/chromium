// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/background/ash_user_wallpaper_delegate.h"

#include "ash/shell.h"
#include "ash/desktop_background/desktop_background_controller.h"
#include "ash/desktop_background/desktop_background_resources.h"
#include "ash/wm/window_animations.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/browser/chromeos/extensions/wallpaper_manager_api.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/notification_service.h"

namespace chromeos {

namespace {

class UserWallpaperDelegate: public ash::UserWallpaperDelegate {
 public:
  UserWallpaperDelegate() {
  }

  virtual ~UserWallpaperDelegate() {
  }

  virtual ash::WindowVisibilityAnimationType GetAnimationType() OVERRIDE {
    if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableNewOobe))
      return ash::WINDOW_VISIBILITY_ANIMATION_TYPE_FADE;

    bool is_registered = WizardController::IsDeviceRegistered();
    // TODO(nkostylev): Figure out whether this would affect autotests as well.
    if (is_registered)
      return ash::WINDOW_VISIBILITY_ANIMATION_TYPE_FADE;
    else
      return ash::WINDOW_VISIBILITY_ANIMATION_TYPE_BRIGHTNESS_GRAYSCALE;
  }

  virtual void InitializeWallpaper() OVERRIDE {
    chromeos::UserManager::Get()->InitializeWallpaper();
  }

  virtual void OpenSetWallpaperPage() OVERRIDE {
    wallpaper_manager_util::OpenWallpaperManager();
  }

  virtual bool CanOpenSetWallpaperPage() OVERRIDE {
    return !chromeos::UserManager::Get()->IsLoggedInAsGuest();
  }

  virtual void OnWallpaperAnimationFinished() OVERRIDE {
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_WALLPAPER_ANIMATION_FINISHED,
        content::NotificationService::AllSources(),
        content::NotificationService::NoDetails());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UserWallpaperDelegate);
};

}  // namespace

ash::UserWallpaperDelegate* CreateUserWallpaperDelegate() {
  return new chromeos::UserWallpaperDelegate();
}

}  // namespace chromeos
