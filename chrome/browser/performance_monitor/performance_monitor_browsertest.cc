// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/in_process_browser_test.h"

#include "base/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/performance_monitor/constants.h"
#include "chrome/browser/performance_monitor/database.h"
#include "chrome/browser/performance_monitor/performance_monitor.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/unpacked_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"

using extensions::Extension;
using performance_monitor::Event;

namespace {
// Helper struct to store the information of an extension; this is needed if the
// pointer to the extension ever becomes invalid (e.g., if we uninstall the
// extension).
struct ExtensionBasicInfo {
  // Empty constructor for stl-container-happiness.
  ExtensionBasicInfo() {
  }
  explicit ExtensionBasicInfo(const Extension* extension)
      : description(extension->description()),
        id(extension->id()),
        name(extension->name()),
        url(extension->url().spec()),
        version(extension->VersionString()),
        location(extension->location()) {
  }

  std::string description;
  std::string id;
  std::string name;
  std::string url;
  std::string version;
  Extension::Location location;
};

// Compare the fields of |extension| to those in |value|; this is a check to
// make sure the extension data was recorded properly in the event.
void ValidateExtensionInfo(const ExtensionBasicInfo extension,
                           const DictionaryValue* value) {
  std::string extension_description;
  std::string extension_id;
  std::string extension_name;
  std::string extension_url;
  std::string extension_version;
  int extension_location;

  ASSERT_TRUE(value->GetString("extensionDescription",
                               &extension_description));
  ASSERT_EQ(extension.description, extension_description);
  ASSERT_TRUE(value->GetString("extensionId", &extension_id));
  ASSERT_EQ(extension.id, extension_id);
  ASSERT_TRUE(value->GetString("extensionName", &extension_name));
  ASSERT_EQ(extension.name, extension_name);
  ASSERT_TRUE(value->GetString("extensionUrl", &extension_url));
  ASSERT_EQ(extension.url, extension_url);
  ASSERT_TRUE(value->GetString("extensionVersion", &extension_version));
  ASSERT_EQ(extension.version, extension_version);
  ASSERT_TRUE(value->GetInteger("extensionLocation", &extension_location));
  ASSERT_EQ(extension.location, extension_location);
}

// Verify that a particular event has the proper type.
void CheckEventType(int expected_event_type, const linked_ptr<Event>& event) {
  int event_type = -1;
  ASSERT_TRUE(event->data()->GetInteger("eventType", &event_type));
  ASSERT_EQ(expected_event_type, event_type);
  ASSERT_EQ(expected_event_type, event->type());
}

// Verify that we received the proper number of events, checking the type of
// each one.
void CheckEventTypes(const std::vector<int> expected_event_types,
                 const std::vector<linked_ptr<Event> >& events) {
  ASSERT_EQ(expected_event_types.size(), events.size());

  for (size_t i = 0; i < expected_event_types.size(); ++i)
    CheckEventType(expected_event_types[i], events[i]);
}

// Check that we received the proper number of events, that each event is of the
// proper type, and that each event recorded the proper information about the
// extension.
void CheckExtensionEvents(
    const std::vector<int>& expected_event_types,
    const std::vector<linked_ptr<Event> >& events,
    const std::vector<ExtensionBasicInfo>& extension_infos) {
  CheckEventTypes(expected_event_types, events);

  for (size_t i = 0; i < expected_event_types.size(); ++i) {
    ValidateExtensionInfo(extension_infos[i], events[i]->data());
    int event_type;
    ASSERT_TRUE(events[i]->data()->GetInteger("eventType", &event_type));
    ASSERT_EQ(expected_event_types[i], event_type);
  }
}

}  // namespace

namespace performance_monitor {

class PerformanceMonitorBrowserTest : public ExtensionBrowserTest {
 public:
  virtual void SetUpOnMainThread() OVERRIDE {
    CHECK(db_dir_.CreateUniqueTempDir());
    performance_monitor_ = PerformanceMonitor::GetInstance();
    performance_monitor_->SetDatabasePath(db_dir_.path());

    // PerformanceMonitor's initialization process involves a significant
    // amount of thread-hopping between the UI thread and the background thread.
    // If we begin the tests prior to full initialization, we cannot predict
    // the behavior or mock synchronicity as we must. Wait for initialization
    // to complete fully before proceeding with the test.
    content::WindowedNotificationObserver windowed_observer(
        chrome::NOTIFICATION_PERFORMANCE_MONITOR_INITIALIZED,
        content::NotificationService::AllSources());

    performance_monitor_->Start();

    windowed_observer.Wait();
  }

  void GetEventsOnBackgroundThread(std::vector<linked_ptr<Event> >* events) {
    // base::Time is potentially flaky in that there is no guarantee that it
    // won't actually decrease between successive calls. If we call GetEvents
    // and the Database uses base::Time::Now() and gets a lesser time, then it
    // will return 0 events. Thus, we use a time that is guaranteed to be in the
    // future (for at least the next couple hundred thousand years).
    *events = performance_monitor_->database()->GetEvents(
        base::Time(), base::Time::FromInternalValue(kint64max));
  }

  // A handle for getting the events from the database, which must be done on
  // the background thread. Since we are testing, we can mock synchronicity
  // with FlushForTesting().
  std::vector<linked_ptr<Event> > GetEvents() {
    // Ensure that any event insertions happen prior to getting events in order
    // to avoid race conditions.
    content::BrowserThread::GetBlockingPool()->FlushForTesting();
    ui_test_utils::RunAllPendingInMessageLoop();

    std::vector<linked_ptr<Event> > events;
    content::BrowserThread::PostBlockingPoolSequencedTask(
        Database::kDatabaseSequenceToken,
        FROM_HERE,
        base::Bind(&PerformanceMonitorBrowserTest::GetEventsOnBackgroundThread,
                   base::Unretained(this),
                   &events));

    content::BrowserThread::GetBlockingPool()->FlushForTesting();
    return events;
  }

  // A handle for inserting a state value into the database, which must be done
  // on the background thread. This is useful for mocking up a scenario in which
  // the database has prior data stored. We mock synchronicity with
  // FlushForTesting().
  void AddStateValue(const std::string& key, const std::string& value) {
    content::BrowserThread::PostBlockingPoolSequencedTask(
        Database::kDatabaseSequenceToken,
        FROM_HERE,
        base::Bind(base::IgnoreResult(&Database::AddStateValue),
                   base::Unretained(performance_monitor()->database()),
                   key,
                   value));

    content::BrowserThread::GetBlockingPool()->FlushForTesting();
  }

  PerformanceMonitor* performance_monitor() const {
    return performance_monitor_;
  }

 protected:
  ScopedTempDir db_dir_;
  PerformanceMonitor* performance_monitor_;
};

// Test that PerformanceMonitor will correctly record an extension installation
// event.
IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest, InstallExtensionEvent) {
  FilePath extension_path;
  PathService::Get(chrome::DIR_TEST_DATA, &extension_path);
  extension_path = extension_path.AppendASCII("performance_monitor")
                                 .AppendASCII("extensions")
                                 .AppendASCII("simple_extension_v1");
  const Extension* extension = LoadExtension(extension_path);

  std::vector<ExtensionBasicInfo> extension_infos;
  extension_infos.push_back(ExtensionBasicInfo(extension));

  std::vector<int> expected_event_types;
  expected_event_types.push_back(EVENT_EXTENSION_INSTALL);

  std::vector<linked_ptr<Event> > events = GetEvents();
  CheckExtensionEvents(expected_event_types, events, extension_infos);
}

// Test that PerformanceMonitor will correctly record events as an extension is
// disabled and enabled.
IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest,
                       DisableAndEnableExtensionEvent) {
  const int kNumEvents = 3;

  FilePath extension_path;
  PathService::Get(chrome::DIR_TEST_DATA, &extension_path);
  extension_path = extension_path.AppendASCII("performance_monitor")
                                 .AppendASCII("extensions")
                                 .AppendASCII("simple_extension_v1");
  const Extension* extension = LoadExtension(extension_path);

  DisableExtension(extension->id());
  EnableExtension(extension->id());

  std::vector<ExtensionBasicInfo> extension_infos;
  // There will be three events in all, each pertaining to the same extension:
  //   Extension Install
  //   Extension Unload
  //   Extension Enable
  for (int i = 0; i < kNumEvents; ++i)
    extension_infos.push_back(ExtensionBasicInfo(extension));

  std::vector<int> expected_event_types;
  expected_event_types.push_back(EVENT_EXTENSION_INSTALL);
  expected_event_types.push_back(EVENT_EXTENSION_UNLOAD);
  expected_event_types.push_back(EVENT_EXTENSION_ENABLE);

  std::vector<linked_ptr<Event> > events = GetEvents();
  CheckExtensionEvents(expected_event_types, events, extension_infos);

  // There will be an additional field on the unload event: Unload Reason.
  int unload_reason = -1;
  ASSERT_TRUE(events[1]->data()->GetInteger("unloadReason", &unload_reason));
  ASSERT_EQ(extension_misc::UNLOAD_REASON_DISABLE, unload_reason);
}

// Test that PerformanceMonitor correctly records an extension update event.
IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest, UpdateExtensionEvent) {
  ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  FilePath test_data_dir;
  PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
  test_data_dir = test_data_dir.AppendASCII("performance_monitor")
                               .AppendASCII("extensions");

  // We need two versions of the same extension.
  FilePath pem_path = test_data_dir.AppendASCII("simple_extension.pem");
  FilePath path_v1_ = PackExtensionWithOptions(
      test_data_dir.AppendASCII("simple_extension_v1"),
      temp_dir.path().AppendASCII("simple_extension1.crx"),
      pem_path,
      FilePath());
  FilePath path_v2_ = PackExtensionWithOptions(
      test_data_dir.AppendASCII("simple_extension_v2"),
      temp_dir.path().AppendASCII("simple_extension2.crx"),
      pem_path,
      FilePath());

  const extensions::Extension* extension = InstallExtension(path_v1_, 1);

  std::vector<ExtensionBasicInfo> extension_infos;
  extension_infos.push_back(ExtensionBasicInfo(extension));

  ExtensionService* extension_service =
      browser()->profile()->GetExtensionService();

  CrxInstaller* crx_installer = NULL;

  // Create an observer to wait for the update to finish.
  content::WindowedNotificationObserver windowed_observer(
      chrome::NOTIFICATION_CRX_INSTALLER_DONE,
      content::Source<CrxInstaller>(crx_installer));
  ASSERT_TRUE(extension_service->
      UpdateExtension(extension->id(), path_v2_, GURL(), &crx_installer));
  windowed_observer.Wait();

  extension = extension_service->GetExtensionById(
      extension_infos[0].id, false); // don't include disabled extensions.

  // The total series of events for this process will be:
  //   Extension Install - install version 1
  //   Extension Install - install version 2
  //   Extension Unload  - disable version 1
  //   Extension Update  - signal the udate to version 2
  // We push back the corresponding ExtensionBasicInfos.
  extension_infos.push_back(ExtensionBasicInfo(extension));
  extension_infos.push_back(extension_infos[0]);
  extension_infos.push_back(extension_infos[1]);

  std::vector<int> expected_event_types;
  expected_event_types.push_back(EVENT_EXTENSION_INSTALL);
  expected_event_types.push_back(EVENT_EXTENSION_INSTALL);
  expected_event_types.push_back(EVENT_EXTENSION_UNLOAD);
  expected_event_types.push_back(EVENT_EXTENSION_UPDATE);

  std::vector<linked_ptr<Event> > events = GetEvents();

  CheckExtensionEvents(expected_event_types, events, extension_infos);

  // There will be an additional field: The unload reason.
  int unload_reason = -1;
  ASSERT_TRUE(events[2]->data()->GetInteger("unloadReason", &unload_reason));
  ASSERT_EQ(extension_misc::UNLOAD_REASON_UPDATE, unload_reason);
}

IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest, UninstallExtensionEvent) {
  const int kNumEvents = 3;
  FilePath extension_path;
  PathService::Get(chrome::DIR_TEST_DATA, &extension_path);
  extension_path = extension_path.AppendASCII("performance_monitor")
                                 .AppendASCII("extensions")
                                 .AppendASCII("simple_extension_v1");
  const Extension* extension = LoadExtension(extension_path);

  std::vector<ExtensionBasicInfo> extension_infos;
  // There will be three events in all, each pertaining to the same extension:
  //   Extension Install
  //   Extension Disable (Unload)
  //   Extension Uninstall
  for (int i = 0; i < kNumEvents; ++i)
    extension_infos.push_back(ExtensionBasicInfo(extension));

  UninstallExtension(extension->id());

  std::vector<int> expected_event_types;
  expected_event_types.push_back(EVENT_EXTENSION_INSTALL);
  expected_event_types.push_back(EVENT_EXTENSION_UNLOAD);
  expected_event_types.push_back(EVENT_EXTENSION_UNINSTALL);

  std::vector<linked_ptr<Event> > events = GetEvents();

  CheckExtensionEvents(expected_event_types, events, extension_infos);

  // There will be an additional field: The unload reason.
  int unload_reason = -1;
  ASSERT_TRUE(events[1]->data()->GetInteger("unloadReason", &unload_reason));
  ASSERT_EQ(extension_misc::UNLOAD_REASON_UNINSTALL, unload_reason);
}

IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest, NewVersionEvent) {
  const char kOldVersion[] = "0.0";

  // The version in the database right now will be the current version of chrome
  // (gathered at initialization of PerformanceMonitor). Replace this with an
  // older version so an event is generated.
  AddStateValue(kStateChromeVersion, kOldVersion);

  content::BrowserThread::PostBlockingPoolSequencedTask(
      Database::kDatabaseSequenceToken,
      FROM_HERE,
      base::Bind(&PerformanceMonitor::CheckForVersionUpdateOnBackgroundThread,
                 base::Unretained(performance_monitor())));

  // Wait for event insertion.
  content::BrowserThread::GetBlockingPool()->FlushForTesting();

  chrome::VersionInfo version;
  ASSERT_TRUE(version.is_valid());
  std::string version_string = version.Version();

  std::vector<linked_ptr<Event> > events = GetEvents();
  ASSERT_EQ(1u, events.size());
  ASSERT_EQ(EVENT_CHROME_UPDATE, events[0]->type());

  const base::DictionaryValue* value;
  ASSERT_TRUE(events[0]->data()->GetAsDictionary(&value));

  std::string previous_version;
  std::string current_version;

  ASSERT_TRUE(value->GetString("previousVersion", &previous_version));
  ASSERT_EQ(kOldVersion, previous_version);
  ASSERT_TRUE(value->GetString("currentVersion", &current_version));
  ASSERT_EQ(version_string, current_version);
}

#if !defined(OS_WIN)
// Disabled on Windows due to a bug where Windows will return a normal exit
// code in the testing environment, even if the process died (this is not the
// case when hand-testing). This code can be traced to MSDN functions in
// base::GetTerminationStatus(), so there's not much we can do.
IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest, KilledByOSEvent) {
  ui_test_utils::CrashTab(chrome::GetActiveWebContents(browser()));

  std::vector<linked_ptr<Event> > events = GetEvents();

  ASSERT_EQ(1u, events.size());
  CheckEventType(EVENT_KILLED_BY_OS_CRASH, events[0]);
}
#endif  // !defined(OS_WIN)

IN_PROC_BROWSER_TEST_F(PerformanceMonitorBrowserTest,
                       DISABLED_RendererCrashEvent) {
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUICrashURL));

  std::vector<linked_ptr<Event> > events = GetEvents();
  ASSERT_EQ(1u, events.size());

  CheckEventType(EVENT_RENDERER_CRASH, events[0]);
}

}  // namespace performance_monitor
