// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/default_browser_prompt.h"

#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/set_as_default_browser_ui.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"

using content::BrowserThread;

namespace {

// Show the page prompting the user to make Chrome the default browser on
// Windows 8 (which means becoming "the browser" in Metro mode). The page
// will be shown at the first appropriate opportunity. It can be placed in
// a tab or in a dialog, depending on other settings.
class SetMetroBrowserFlowLauncher : public content::NotificationObserver {
 public:
  static void LaunchSoon(Profile* profile) {
    // The instance will manage its own lifetime.
    new SetMetroBrowserFlowLauncher(profile);
  }

 private:
  explicit SetMetroBrowserFlowLauncher(Profile* profile)
      : profile_(profile) {
    registrar_.Add(this, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
                   content::NotificationService::AllSources());
  }

  // content::NotificationObserver override:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE;

  content::NotificationRegistrar registrar_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SetMetroBrowserFlowLauncher);
};

void SetMetroBrowserFlowLauncher::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(type, content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME);
  Browser* browser = browser::FindBrowserWithWebContents(
      content::Source<content::WebContents>(source).ptr());

  if (!browser || !browser->is_type_tabbed())
    return;

  // Unregister and delete.
  registrar_.RemoveAll();
  bool as_dialog =
      profile_->GetPrefs()->GetBoolean(prefs::kDefaultBrowserFlowDialog);

  SetAsDefaultBrowserUI::Show(profile_, browser, as_dialog);
  delete this;
}

}  // namespace

namespace chrome {

void ShowFirstRunDefaultBrowserPrompt(Profile* profile) {
  if ((ShellIntegration::IsDefaultBrowser() ==
           ShellIntegration::NOT_DEFAULT_WEB_CLIENT) &&
      (ShellIntegration::CanSetAsDefaultBrowser() ==
           ShellIntegration::SET_DEFAULT_INTERACTIVE)) {
    // If the only available mode of setting the default browser requires
    // user interaction, it means this couldn't have been done yet.
    SetMetroBrowserFlowLauncher::LaunchSoon(profile);
  }
}

}  // namespace chrome
