// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/autocomplete_classifier.h"

#include "base/auto_reset.h"
#include "chrome/browser/autocomplete/autocomplete_controller.h"
#include "chrome/browser/autocomplete/autocomplete_input.h"
#include "chrome/browser/autocomplete/autocomplete_match.h"
#include "googleurl/src/gurl.h"

AutocompleteClassifier::AutocompleteClassifier(Profile* profile)
    : controller_(new AutocompleteController(profile, NULL)),
      inside_classify_(false) {
}

AutocompleteClassifier::~AutocompleteClassifier() {
  // We should only reach here after Shutdown() has been called.
  DCHECK(!controller_.get());
}

void AutocompleteClassifier::Classify(const string16& text,
                                      const string16& desired_tld,
                                      bool prefer_keyword,
                                      bool allow_exact_keyword_match,
                                      AutocompleteMatch* match,
                                      GURL* alternate_nav_url) {
  DCHECK(!inside_classify_);
  AutoReset<bool> reset(&inside_classify_, true);
  controller_->Start(text, desired_tld, true, prefer_keyword,
                     allow_exact_keyword_match, AutocompleteInput::BEST_MATCH);
  DCHECK(controller_->done());
  const AutocompleteResult& result = controller_->result();
  if (result.empty()) {
    if (alternate_nav_url)
      *alternate_nav_url = GURL();
    return;
  }

  DCHECK(result.default_match() != result.end());
  *match = *result.default_match();
  if (alternate_nav_url)
    *alternate_nav_url = result.alternate_nav_url();
}

void AutocompleteClassifier::Shutdown() {
  controller_.reset();
}
