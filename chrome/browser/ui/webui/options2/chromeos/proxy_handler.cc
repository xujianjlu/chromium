// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options2/chromeos/proxy_handler.h"

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/stl_util.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/web_ui.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace chromeos {
namespace options2 {

ProxyHandler::ProxyHandler() {
}

ProxyHandler::~ProxyHandler() {
}

void ProxyHandler::GetLocalizedValues(
    DictionaryValue* localized_strings) {
  DCHECK(localized_strings);

  // Proxy page - ChromeOS
  static OptionsStringResource resources[] = {
    { "proxyPage", IDS_OPTIONS_PROXY_TAB_LABEL },
    { "proxyDirectInternetConnection", IDS_PROXY_DIRECT_CONNECTION },
    { "proxyManual", IDS_PROXY_MANUAL_CONFIG },
    { "sameProxyProtocols", IDS_PROXY_SAME_FORALL },
    { "httpProxy", IDS_PROXY_HTTP_PROXY },
    { "secureHttpProxy", IDS_PROXY_HTTP_SECURE_HTTP_PROXY },
    { "ftpProxy", IDS_PROXY_FTP_PROXY },
    { "socksHost", IDS_PROXY_SOCKS_HOST },
    { "proxyAutomatic", IDS_PROXY_AUTOMATIC },
    { "proxyConfigUrl", IDS_PROXY_CONFIG_URL },
    { "advancedProxyConfig", IDS_PROXY_ADVANCED_CONFIG },
    { "addHost", IDS_PROXY_ADD_HOST },
    { "removeHost", IDS_PROXY_REMOVE_HOST },
    { "proxyPort", IDS_PROXY_PORT },
    { "proxyBypass", IDS_PROXY_BYPASS },
    { "policyManagedPrefsBannerText", IDS_OPTIONS_POLICY_MANAGED_PREFS },
    { "extensionManagedPrefsBannerText", IDS_OPTIONS_EXTENSION_MANAGED_PREFS },
    { "unmodifiablePrefsBannerText", IDS_OPTIONS_UNMODIFIABLE_PREFS }
  };

  RegisterStrings(localized_strings, resources, arraysize(resources));

  localized_strings->SetString("enableSharedProxiesBannerText",
      l10n_util::GetStringFUTF16(
          IDS_OPTIONS_SETTINGS_INTERNET_OPTIONS_ENABLE_SHARED_PROXIES_HINT,
          l10n_util::GetStringUTF16(IDS_OPTIONS_SETTINGS_USE_SHARED_PROXIES)));
}

}  // namespace options2
}  // namespace chromeos
