// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTIONS_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTIONS_API_H_

#include "chrome/browser/extensions/extension_function.h"
#include "chrome/common/extensions/extension_action.h"

namespace base {
class DictionaryValue;
}
namespace extensions {
class TabHelper;
}
class TabContents;

// Implementation of the browserAction, pageAction, and scriptBadge APIs.
//
// Divergent behaviour between the three is minimal (pageAction and scriptBadge
// have required tabIds while browserAction's are optional, they have different
// internal browser notification requirements, and not all functions are defined
// for all APIs).
class ExtensionActionFunction : public SyncExtensionFunction {
 public:
  static bool ParseCSSColorString(const std::string& color_string,
                                  SkColor* result);

 protected:
  ExtensionActionFunction();
  virtual ~ExtensionActionFunction();
  virtual bool RunImpl() OVERRIDE;
  virtual bool RunExtensionAction() = 0;
  void NotifyChange();
  void NotifyBrowserActionChange();
  void NotifyLocationBarChange();
  bool SetVisible(bool visible);

  // Extension-related information for |tab_id_|.
  // CHECK-fails if there is no tab.
  extensions::TabHelper& tab_helper() const;

  // All the extension action APIs take a single argument called details that
  // is a dictionary.
  base::DictionaryValue* details_;

  // The tab id the extension action function should apply to, if any, or
  // kDefaultTabId if none was specified.
  int tab_id_;

  // Tab content for |tab_id_| if one exists.
  TabContents* contents_;

  // The extension action for the current extension.
  ExtensionAction* extension_action_;
};

//
// Implementations of each extension action API.
//
// pageAction and browserAction bindings are created for these by extending them
// then declaring an EXTENSION_FUNCTION_NAME.
//

// show
class ExtensionActionShowFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionShowFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// hide
class ExtensionActionHideFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionHideFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// setIcon
class ExtensionActionSetIconFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionSetIconFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// setTitle
class ExtensionActionSetTitleFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionSetTitleFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// setPopup
class ExtensionActionSetPopupFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionSetPopupFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// setBadgeText
class ExtensionActionSetBadgeTextFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionSetBadgeTextFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// setBadgeBackgroundColor
class ExtensionActionSetBadgeBackgroundColorFunction
    : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionSetBadgeBackgroundColorFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// getTitle
class ExtensionActionGetTitleFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionGetTitleFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// getPopup
class ExtensionActionGetPopupFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionGetPopupFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// getBadgeText
class ExtensionActionGetBadgeTextFunction : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionGetBadgeTextFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

// getBadgeBackgroundColor
class ExtensionActionGetBadgeBackgroundColorFunction
    : public ExtensionActionFunction {
 protected:
  virtual ~ExtensionActionGetBadgeBackgroundColorFunction() {}
  virtual bool RunExtensionAction() OVERRIDE;
};

#endif  // CHROME_BROWSER_EXTENSIONS_API_EXTENSION_ACTION_EXTENSION_ACTIONS_API_H_
