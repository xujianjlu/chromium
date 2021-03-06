// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "sync/engine/cleanup_disabled_types_command.h"

#include "sync/internal_api/public/base/model_type_test_util.h"
#include "sync/sessions/sync_session.h"
#include "sync/test/engine/syncer_command_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using testing::_;

class CleanupDisabledTypesCommandTest : public MockDirectorySyncerCommandTest {
 public:
  CleanupDisabledTypesCommandTest() {}

  virtual void SetUp() {
    mutable_routing_info()->clear();
    (*mutable_routing_info())[BOOKMARKS] = GROUP_PASSIVE;
    MockDirectorySyncerCommandTest::SetUp();
  }
};

// TODO(tim): Add syncer test to verify previous routing info is set.
TEST_F(CleanupDisabledTypesCommandTest, NoPreviousRoutingInfo) {
  CleanupDisabledTypesCommand command;
  ModelTypeSet expected = ModelTypeSet::All();
  expected.Remove(BOOKMARKS);
  EXPECT_CALL(*mock_directory(),
              PurgeEntriesWithTypeIn(HasModelTypes(expected)));
  command.ExecuteImpl(session());
}

TEST_F(CleanupDisabledTypesCommandTest, NoPurge) {
  CleanupDisabledTypesCommand command;
  EXPECT_CALL(*mock_directory(), PurgeEntriesWithTypeIn(_)).Times(0);

  ModelSafeRoutingInfo prev(routing_info());
  session()->context()->set_previous_session_routing_info(prev);
  (*mutable_routing_info())[AUTOFILL] = GROUP_PASSIVE;
  command.ExecuteImpl(session());

  prev = routing_info();
  command.ExecuteImpl(session());
}

TEST_F(CleanupDisabledTypesCommandTest, TypeDisabled) {
  CleanupDisabledTypesCommand command;

  (*mutable_routing_info())[AUTOFILL] = GROUP_PASSIVE;
  (*mutable_routing_info())[THEMES] = GROUP_PASSIVE;
  (*mutable_routing_info())[EXTENSIONS] = GROUP_PASSIVE;

  ModelSafeRoutingInfo prev(routing_info());
  prev[PASSWORDS] = GROUP_PASSIVE;
  prev[PREFERENCES] = GROUP_PASSIVE;
  session()->context()->set_previous_session_routing_info(prev);

  const ModelTypeSet expected(PASSWORDS, PREFERENCES);
  EXPECT_CALL(*mock_directory(),
              PurgeEntriesWithTypeIn(HasModelTypes(expected)));
  command.ExecuteImpl(session());
}

}  // namespace

}  // namespace syncer
