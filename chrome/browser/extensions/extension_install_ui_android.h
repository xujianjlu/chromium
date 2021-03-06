// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_UI_ANDROID_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_UI_ANDROID_H_

#include "chrome/browser/extensions/extension_install_ui.h"

class ExtensionInstallUIAndroid : public ExtensionInstallUI {
 public:
  ExtensionInstallUIAndroid();
  virtual ~ExtensionInstallUIAndroid();

  // ExtensionInstallUI implementation:
  virtual void OnInstallSuccess(const extensions::Extension* extension,
                                SkBitmap* icon);
  virtual void OnInstallFailure(const CrxInstallerError& error);
  virtual void SetSkipPostInstallUI(bool skip_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallUIAndroid);
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_INSTALL_UI_ANDROID_H_
