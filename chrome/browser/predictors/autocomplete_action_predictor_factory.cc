// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/autocomplete_action_predictor_factory.h"

#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/predictors/autocomplete_action_predictor.h"
#include "chrome/browser/predictors/predictor_database_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_dependency_manager.h"

namespace predictors {

// static
AutocompleteActionPredictor* AutocompleteActionPredictorFactory::GetForProfile(
    Profile* profile) {
  return static_cast<AutocompleteActionPredictor*>(
      GetInstance()->GetServiceForProfile(profile, true));
}

// static
AutocompleteActionPredictorFactory*
    AutocompleteActionPredictorFactory::GetInstance() {
  return Singleton<AutocompleteActionPredictorFactory>::get();
}

AutocompleteActionPredictorFactory::AutocompleteActionPredictorFactory()
    : ProfileKeyedServiceFactory("AutocompleteActionPredictor",
                                 ProfileDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(PredictorDatabaseFactory::GetInstance());
}

AutocompleteActionPredictorFactory::~AutocompleteActionPredictorFactory() {}

bool AutocompleteActionPredictorFactory::ServiceHasOwnInstanceInIncognito() {
  return true;
}

ProfileKeyedService*
    AutocompleteActionPredictorFactory::BuildServiceInstanceFor(
        Profile* profile) const {
  return new AutocompleteActionPredictor(profile);
}

}  // namespace predictors
