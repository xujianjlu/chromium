// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_STARTUP_DEFAULT_BROWSER_PROMPT_H_
#define CHROME_BROWSER_UI_STARTUP_DEFAULT_BROWSER_PROMPT_H_

class Profile;

namespace chrome {

// Shows a prompt UI to set the default browser if necessary.
void ShowDefaultBrowserPrompt(Profile* profile);

// Shows a prompt UI to set the default browser on first run
// (if necessary and permitted).
void ShowFirstRunDefaultBrowserPrompt(Profile* profile);

}  // namespace chrome

#endif  // CHROME_BROWSER_UI_STARTUP_DEFAULT_BROWSER_PROMPT_H_
