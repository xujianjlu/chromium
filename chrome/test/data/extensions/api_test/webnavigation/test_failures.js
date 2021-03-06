// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function runTests() {
  var getURL = chrome.extension.getURL;
  chrome.tabs.create({"url": "about:blank"}, function(tab) {
    var tabId = tab.id;

    chrome.test.runTests([
      // Navigates to a non-existant page.
      function nonExistant() {
        expect([
          { label: "onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/nonexistant.html') }},
          { label: "onErrorOccurred",
            event: "onErrorOccurred",
            details: { error: "net::ERR_FILE_NOT_FOUND",
                       frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/nonexistant.html') }}],
          [["onBeforeNavigate", "onErrorOccurred"]]);
        chrome.tabs.update(tabId, { url: getURL('failures/nonexistant.html') });
      },

      // An page that tries to load an non-existant iframe.
      function nonExistantIframe() {
        expect([
          { label: "a-onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/d.html') }},
          { label: "a-onCommitted",
            event: "onCommitted",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       transitionQualifiers: [],
                       transitionType: "link",
                       url: getURL('failures/d.html') }},
          { label: "a-onDOMContentLoaded",
            event: "onDOMContentLoaded",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/d.html') }},
          { label: "a-onCompleted",
            event: "onCompleted",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/d.html') }},
          { label: "b-onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/c.html') }},
          { label: "b-onErrorOccurred",
            event: "onErrorOccurred",
            details: { error: "net::ERR_FILE_NOT_FOUND",
                       frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/c.html') }}],
          [navigationOrder("a-"),
            ["a-onCommitted", "b-onBeforeNavigate", "b-onErrorOccurred",
             "a-onCompleted"]]);
        chrome.tabs.update(tabId, { url: getURL('failures/d.html') });
      },

      // An iframe navigates to a non-existant page.
      function nonExistantIframeNavigation() {
        expect([
          { label: "a-onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/a.html') }},
          { label: "a-onCommitted",
            event: "onCommitted",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       transitionQualifiers: [],
                       transitionType: "link",
                       url: getURL('failures/a.html') }},
          { label: "a-onDOMContentLoaded",
            event: "onDOMContentLoaded",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/a.html') }},
          { label: "a-onCompleted",
            event: "onCompleted",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/a.html') }},
          { label: "b-onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/b.html') }},
          { label: "b-onCommitted",
            event: "onCommitted",
            details: { frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       transitionQualifiers: [],
                       transitionType: "auto_subframe",
                       url: getURL('failures/b.html') }},
          { label: "b-onDOMContentLoaded",
            event: "onDOMContentLoaded",
            details: { frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/b.html') }},
          { label: "b-onCompleted",
            event: "onCompleted",
            details: { frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/b.html') }},
          { label: "c-onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/c.html') }},
          { label: "c-onErrorOccurred",
            event: "onErrorOccurred",
            details: { error: "net::ERR_FILE_NOT_FOUND",
                       frameId: 1,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/c.html') }}],
          [ navigationOrder("a-"),
            navigationOrder("b-"),
            isIFrameOf("b-", "a-"),
            isLoadedBy("c-", "b-"),
            ["c-onBeforeNavigate", "c-onErrorOccurred"]]);
        chrome.tabs.update(tabId, { url: getURL('failures/a.html') });
      },

      // Cancel a navigation after it is already committed.
      function cancel() {
        expect([
          { label: "onBeforeNavigate",
            event: "onBeforeNavigate",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/e.html') }},
          { label: "onCommitted",
            event: "onCommitted",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       transitionQualifiers: [],
                       transitionType: "link",
                       url: getURL('failures/e.html') }},
          { label: "onDOMContentLoaded",
            event: "onDOMContentLoaded",
            details: { frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/e.html') }},
          { label: "onErrorOccurred",
            event: "onErrorOccurred",
            details: { error: "net::ERR_ABORTED",
                       frameId: 0,
                       tabId: 0,
                       timeStamp: 0,
                       url: getURL('failures/e.html') }}],
          [["onBeforeNavigate", "onCommitted", "onDOMContentLoaded",
            "onErrorOccurred"]]);
        chrome.tabs.update(tabId, { url: getURL('failures/e.html') });
      },
    ]);
  });
}
