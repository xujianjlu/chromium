// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implements the Chrome Extensions WebNavigation API.

#include "chrome/browser/extensions/api/web_navigation/web_navigation_api.h"

#include "base/lazy_instance.h"
#include "chrome/browser/extensions/api/web_navigation/web_navigation_api_constants.h"
#include "chrome/browser/extensions/api/web_navigation/web_navigation_api_helpers.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_contents/retargeting_details.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tab_contents/tab_contents.h"
#include "chrome/browser/view_type_utils.h"
#include "chrome/common/chrome_notification_types.h"
#include "chrome/common/extensions/api/web_navigation.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"

namespace GetFrame = extensions::api::web_navigation::GetFrame;
namespace GetAllFrames = extensions::api::web_navigation::GetAllFrames;

namespace extensions {

namespace helpers = web_navigation_api_helpers;
namespace keys = web_navigation_api_constants;

namespace {

typedef std::map<content::WebContents*, WebNavigationTabObserver*>
    TabObserverMap;
static base::LazyInstance<TabObserverMap> g_tab_observer =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

// WebNavigtionEventRouter -------------------------------------------

WebNavigationEventRouter::PendingWebContents::PendingWebContents()
    : source_web_contents(NULL),
      source_frame_id(0),
      source_frame_is_main_frame(false),
      target_web_contents(NULL),
      target_url() {
}

WebNavigationEventRouter::PendingWebContents::PendingWebContents(
    content::WebContents* source_web_contents,
    int64 source_frame_id,
    bool source_frame_is_main_frame,
    content::WebContents* target_web_contents,
    const GURL& target_url)
    : source_web_contents(source_web_contents),
      source_frame_id(source_frame_id),
      source_frame_is_main_frame(source_frame_is_main_frame),
      target_web_contents(target_web_contents),
      target_url(target_url) {
}

WebNavigationEventRouter::PendingWebContents::~PendingWebContents() {}

WebNavigationEventRouter::WebNavigationEventRouter(Profile* profile)
    : profile_(profile) {}

WebNavigationEventRouter::~WebNavigationEventRouter() {
  BrowserList::RemoveObserver(this);
}

void WebNavigationEventRouter::Init() {
  if (!registrar_.IsEmpty())
    return;
  registrar_.Add(this,
                 chrome::NOTIFICATION_RETARGETING,
                 content::NotificationService::AllSources());
  registrar_.Add(this,
                 chrome::NOTIFICATION_TAB_ADDED,
                 content::NotificationService::AllSources());
  registrar_.Add(this,
                 content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
                 content::NotificationService::AllSources());

  BrowserList::AddObserver(this);
  for (BrowserList::const_iterator iter = BrowserList::begin();
       iter != BrowserList::end(); ++iter) {
    OnBrowserAdded(*iter);
  }
}

void WebNavigationEventRouter::OnBrowserAdded(Browser* browser) {
  if (!profile_->IsSameProfile(browser->profile()))
    return;
  browser->tab_strip_model()->AddObserver(this);
}

void WebNavigationEventRouter::OnBrowserRemoved(Browser* browser) {
  if (!profile_->IsSameProfile(browser->profile()))
    return;
  browser->tab_strip_model()->RemoveObserver(this);
}

void WebNavigationEventRouter::TabReplacedAt(
    TabStripModel* tab_strip_model,
    TabContents* old_contents,
    TabContents* new_contents,
    int index) {
  WebNavigationTabObserver* tab_observer =
      WebNavigationTabObserver::Get(old_contents->web_contents());
  if (!tab_observer) {
    // If you hit this DCHECK(), please add reproduction steps to
    // http://crbug.com/109464.
    DCHECK(chrome::GetViewType(old_contents->web_contents()) !=
           chrome::VIEW_TYPE_TAB_CONTENTS);
    return;
  }
  const FrameNavigationState& frame_navigation_state =
      tab_observer->frame_navigation_state();

  if (!frame_navigation_state.IsValidUrl(
          old_contents->web_contents()->GetURL()) ||
      !frame_navigation_state.IsValidUrl(
          new_contents->web_contents()->GetURL()))
    return;

  helpers::DispatchOnTabReplaced(
      old_contents->web_contents(),
      profile_,
      new_contents->web_contents());
}

void WebNavigationEventRouter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_RETARGETING: {
      Profile* profile = content::Source<Profile>(source).ptr();
      if (profile->GetOriginalProfile() == profile_) {
        Retargeting(
            content::Details<const RetargetingDetails>(details).ptr());
      }
      break;
    }

    case chrome::NOTIFICATION_TAB_ADDED:
      TabAdded(content::Details<content::WebContents>(details).ptr());
      break;

    case content::NOTIFICATION_WEB_CONTENTS_DESTROYED:
      TabDestroyed(content::Source<content::WebContents>(source).ptr());
      break;

    default:
      NOTREACHED();
  }
}

void WebNavigationEventRouter::Retargeting(const RetargetingDetails* details) {
  if (details->source_frame_id == 0)
    return;
  WebNavigationTabObserver* tab_observer =
      WebNavigationTabObserver::Get(details->source_web_contents);
  if (!tab_observer) {
    // If you hit this DCHECK(), please add reproduction steps to
    // http://crbug.com/109464.
    DCHECK(chrome::GetViewType(details->source_web_contents) !=
           chrome::VIEW_TYPE_TAB_CONTENTS);
    return;
  }
  const FrameNavigationState& frame_navigation_state =
      tab_observer->frame_navigation_state();

  if (!frame_navigation_state.CanSendEvents(details->source_frame_id))
    return;

  // If the WebContents was created as a response to an IPC from a renderer
  // (and therefore doesn't yet have a TabContents), or if it isn't yet inserted
  // into a tab strip, we need to delay the extension event until the
  // WebContents is fully initialized.
  if (TabContents::FromWebContents(details->target_web_contents) == NULL ||
      details->not_yet_in_tabstrip) {
    pending_web_contents_[details->target_web_contents] =
        PendingWebContents(
            details->source_web_contents,
            details->source_frame_id,
            frame_navigation_state.IsMainFrame(details->source_frame_id),
            details->target_web_contents,
            details->target_url);
  } else {
    helpers::DispatchOnCreatedNavigationTarget(
        details->source_web_contents,
        details->target_web_contents->GetBrowserContext(),
        details->source_frame_id,
        frame_navigation_state.IsMainFrame(details->source_frame_id),
        details->target_web_contents,
        details->target_url);
  }
}

void WebNavigationEventRouter::TabAdded(content::WebContents* tab) {
  std::map<content::WebContents*, PendingWebContents>::iterator iter =
      pending_web_contents_.find(tab);
  if (iter == pending_web_contents_.end())
    return;

  helpers::DispatchOnCreatedNavigationTarget(
      iter->second.source_web_contents,
      iter->second.target_web_contents->GetBrowserContext(),
      iter->second.source_frame_id,
      iter->second.source_frame_is_main_frame,
      iter->second.target_web_contents,
      iter->second.target_url);
  pending_web_contents_.erase(iter);
}

void WebNavigationEventRouter::TabDestroyed(content::WebContents* tab) {
  pending_web_contents_.erase(tab);
  for (std::map<content::WebContents*, PendingWebContents>::iterator i =
           pending_web_contents_.begin(); i != pending_web_contents_.end(); ) {
    if (i->second.source_web_contents == tab)
      pending_web_contents_.erase(i++);
    else
      ++i;
  }
}

// WebNavigationTabObserver ------------------------------------------

WebNavigationTabObserver::WebNavigationTabObserver(
    content::WebContents* web_contents)
    : WebContentsObserver(web_contents),
      render_view_host_(NULL),
      pending_render_view_host_(NULL) {
  g_tab_observer.Get().insert(TabObserverMap::value_type(web_contents, this));
  registrar_.Add(this,
                 content::NOTIFICATION_RESOURCE_RECEIVED_REDIRECT,
                 content::Source<content::WebContents>(web_contents));
}

WebNavigationTabObserver::~WebNavigationTabObserver() {}

// static
WebNavigationTabObserver* WebNavigationTabObserver::Get(
    content::WebContents* web_contents) {
  TabObserverMap::iterator i = g_tab_observer.Get().find(web_contents);
  return i == g_tab_observer.Get().end() ? NULL : i->second;
}

void WebNavigationTabObserver::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_RESOURCE_RECEIVED_REDIRECT: {
      content::ResourceRedirectDetails* resource_redirect_details =
          content::Details<content::ResourceRedirectDetails>(details).ptr();
      ResourceType::Type resource_type =
          resource_redirect_details->resource_type;
      if (resource_type == ResourceType::MAIN_FRAME ||
          resource_type == ResourceType::SUB_FRAME) {
        int64 frame_id = resource_redirect_details->frame_id;
        if (!navigation_state_.CanSendEvents(frame_id))
          return;
        navigation_state_.SetIsServerRedirected(frame_id);
      }
      break;
    }

    default:
      NOTREACHED();
  }
}

void WebNavigationTabObserver::AboutToNavigateRenderView(
    content::RenderViewHost* render_view_host) {
  if (!render_view_host_) {
    render_view_host_ = render_view_host;
  } else if (render_view_host != render_view_host_) {
    // TODO(jochen): If pending_render_view_host_ is non-NULL, send error events
    // for all ongoing navigations in that RVH.
    pending_render_view_host_ = render_view_host;
  }
}

void WebNavigationTabObserver::DidStartProvisionalLoadForFrame(
    int64 frame_id,
    bool is_main_frame,
    const GURL& validated_url,
    bool is_error_page,
    content::RenderViewHost* render_view_host) {
  if (!render_view_host_)
    render_view_host_ = render_view_host;
  if (render_view_host != render_view_host_ &&
      render_view_host != pending_render_view_host_)
    return;

  // TODO(jochen): Remove this hack once we properly include the process ID in
  // the events.
  // Ignore navigations of sub frames, if the main frame isn't committed yet.
  // This might happen if a sub frame triggers a navigation for both the main
  // frame and itself. Since the sub frame is about to be deleted, and there's
  // no way for an extension to tell that these navigations belong to an old
  // frame, we just suppress the events here.
  int64 main_frame_id = navigation_state_.GetMainFrameID();
  if (!is_main_frame &&
      !navigation_state_.GetNavigationCommitted(main_frame_id)) {
    return;
  }

  navigation_state_.TrackFrame(frame_id,
                               validated_url,
                               is_main_frame,
                               is_error_page);
  if (!navigation_state_.CanSendEvents(frame_id))
    return;

  helpers::DispatchOnBeforeNavigate(
      web_contents(), frame_id, is_main_frame, validated_url);
}

void WebNavigationTabObserver::DidCommitProvisionalLoadForFrame(
    int64 frame_id,
    bool is_main_frame,
    const GURL& url,
    content::PageTransition transition_type,
    content::RenderViewHost* render_view_host) {
  if (render_view_host != render_view_host_ &&
      render_view_host != pending_render_view_host_)
    return;
  // TODO(jochen): If we switched the RVH, send error events for all ongoing
  // navigations in the old RVH.
  render_view_host_ = render_view_host;
  pending_render_view_host_ = NULL;

  if (!navigation_state_.CanSendEvents(frame_id))
    return;

  bool is_reference_fragment_navigation =
      IsReferenceFragmentNavigation(frame_id, url);
  bool is_history_navigation =
      navigation_state_.GetNavigationCommitted(frame_id);

  // Update the URL as it might have changed.
  navigation_state_.UpdateFrame(frame_id, url);
  navigation_state_.SetNavigationCommitted(frame_id);

  if (is_reference_fragment_navigation) {
    helpers::DispatchOnCommitted(
        keys::kOnReferenceFragmentUpdated,
        web_contents(),
        frame_id,
        is_main_frame,
        url,
        transition_type);
    navigation_state_.SetNavigationCompleted(frame_id);
  } else if (is_history_navigation) {
    // Make the transition type match the one for reference fragment updates.
    transition_type = static_cast<content::PageTransition>(
        transition_type | content::PAGE_TRANSITION_CLIENT_REDIRECT);
    helpers::DispatchOnCommitted(
        keys::kOnHistoryStateUpdated,
        web_contents(),
        frame_id,
        is_main_frame,
        url,
        transition_type);
    navigation_state_.SetNavigationCompleted(frame_id);
  } else {
    if (navigation_state_.GetIsServerRedirected(frame_id)) {
      transition_type = static_cast<content::PageTransition>(
          transition_type | content::PAGE_TRANSITION_SERVER_REDIRECT);
    }
    helpers::DispatchOnCommitted(
        keys::kOnCommitted,
        web_contents(),
        frame_id,
        is_main_frame,
        url,
        transition_type);
  }
}

void WebNavigationTabObserver::DidFailProvisionalLoad(
    int64 frame_id,
    bool is_main_frame,
    const GURL& validated_url,
    int error_code,
    const string16& error_description,
    content::RenderViewHost* render_view_host) {
  if (render_view_host != render_view_host_ &&
      render_view_host != pending_render_view_host_)
    return;
  if (render_view_host == pending_render_view_host_)
    pending_render_view_host_ = NULL;

  if (!navigation_state_.CanSendEvents(frame_id))
    return;

  navigation_state_.SetErrorOccurredInFrame(frame_id);
  helpers::DispatchOnErrorOccurred(
      web_contents(), validated_url, frame_id, is_main_frame, error_code);
}

void WebNavigationTabObserver::DocumentLoadedInFrame(
    int64 frame_id,
    content::RenderViewHost* render_view_host) {
  if (render_view_host != render_view_host_)
    return;
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  helpers::DispatchOnDOMContentLoaded(web_contents(),
                                      navigation_state_.GetUrl(frame_id),
                                      navigation_state_.IsMainFrame(frame_id),
                                      frame_id);
}

void WebNavigationTabObserver::DidFinishLoad(
    int64 frame_id,
    const GURL& validated_url,
    bool is_main_frame,
    content::RenderViewHost* render_view_host) {
  if (render_view_host != render_view_host_)
    return;
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  navigation_state_.SetNavigationCompleted(frame_id);
  DCHECK_EQ(navigation_state_.GetUrl(frame_id), validated_url);
  DCHECK_EQ(navigation_state_.IsMainFrame(frame_id), is_main_frame);
  helpers::DispatchOnCompleted(web_contents(),
                               validated_url,
                               is_main_frame,
                               frame_id);
}

void WebNavigationTabObserver::DidFailLoad(
    int64 frame_id,
    const GURL& validated_url,
    bool is_main_frame,
    int error_code,
    const string16& error_description,
    content::RenderViewHost* render_view_host) {
  if (render_view_host != render_view_host_)
    return;
  if (!navigation_state_.CanSendEvents(frame_id))
    return;
  navigation_state_.SetErrorOccurredInFrame(frame_id);
  helpers::DispatchOnErrorOccurred(
      web_contents(), validated_url, frame_id, is_main_frame, error_code);
}

void WebNavigationTabObserver::DidOpenRequestedURL(
    content::WebContents* new_contents,
    const GURL& url,
    const content::Referrer& referrer,
    WindowOpenDisposition disposition,
    content::PageTransition transition,
    int64 source_frame_id) {
  if (!navigation_state_.CanSendEvents(source_frame_id))
    return;

  // We only send the onCreatedNavigationTarget if we end up creating a new
  // window.
  if (disposition != SINGLETON_TAB &&
      disposition != NEW_FOREGROUND_TAB &&
      disposition != NEW_BACKGROUND_TAB &&
      disposition != NEW_POPUP &&
      disposition != NEW_WINDOW &&
      disposition != OFF_THE_RECORD)
    return;

  helpers::DispatchOnCreatedNavigationTarget(
      web_contents(),
      new_contents->GetBrowserContext(),
      source_frame_id,
      navigation_state_.IsMainFrame(source_frame_id),
      new_contents,
      url);
}

void WebNavigationTabObserver::WebContentsDestroyed(content::WebContents* tab) {
  g_tab_observer.Get().erase(tab);
  for (FrameNavigationState::const_iterator frame = navigation_state_.begin();
       frame != navigation_state_.end(); ++frame) {
    if (!navigation_state_.GetNavigationCompleted(*frame) &&
        navigation_state_.CanSendEvents(*frame)) {
      helpers::DispatchOnErrorOccurred(
        tab,
        navigation_state_.GetUrl(*frame),
        *frame,
        navigation_state_.IsMainFrame(*frame),
        net::ERR_ABORTED);
    }
  }
}

// See also NavigationController::IsURLInPageNavigation.
bool WebNavigationTabObserver::IsReferenceFragmentNavigation(
    int64 frame_id,
    const GURL& url) {
  GURL existing_url = navigation_state_.GetUrl(frame_id);
  if (existing_url == url)
    return false;

  url_canon::Replacements<char> replacements;
  replacements.ClearRef();
  return existing_url.ReplaceComponents(replacements) ==
      url.ReplaceComponents(replacements);
}

bool GetFrameFunction::RunImpl() {
  scoped_ptr<GetFrame::Params> params(GetFrame::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->details.tab_id;
  int frame_id = params->details.frame_id;

  SetResult(Value::CreateNullValue());

  TabContents* tab_contents;
  if (!ExtensionTabUtil::GetTabById(tab_id,
                                    profile(),
                                    include_incognito(),
                                    NULL, NULL,
                                    &tab_contents,
                                    NULL) ||
      !tab_contents) {
    return true;
  }

  content::WebContents* web_contents = tab_contents->web_contents();
  WebNavigationTabObserver* observer =
      WebNavigationTabObserver::Get(web_contents);
  DCHECK(observer);

  const FrameNavigationState& frame_navigation_state =
      observer->frame_navigation_state();

  if (frame_id == 0)
    frame_id = frame_navigation_state.GetMainFrameID();
  if (!frame_navigation_state.IsValidFrame(frame_id))
    return true;

  GURL frame_url = frame_navigation_state.GetUrl(frame_id);
  if (!frame_navigation_state.IsValidUrl(frame_url))
    return true;

  GetFrame::Results::Details frame_details;
  frame_details.url = frame_url.spec();
  frame_details.error_occurred =
      frame_navigation_state.GetErrorOccurredInFrame(frame_id);
  results_ = GetFrame::Results::Create(frame_details);
  return true;
}

bool GetAllFramesFunction::RunImpl() {
  scoped_ptr<GetAllFrames::Params> params(GetAllFrames::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->details.tab_id;

  SetResult(Value::CreateNullValue());

  TabContents* tab_contents;
  if (!ExtensionTabUtil::GetTabById(tab_id,
                                    profile(),
                                    include_incognito(),
                                    NULL, NULL,
                                    &tab_contents,
                                    NULL) ||
      !tab_contents) {
    return true;
  }

  content::WebContents* web_contents = tab_contents->web_contents();
  WebNavigationTabObserver* observer =
      WebNavigationTabObserver::Get(web_contents);
  DCHECK(observer);

  const FrameNavigationState& navigation_state =
      observer->frame_navigation_state();

  std::vector<linked_ptr<GetAllFrames::Results::DetailsElement> > result_list;
  for (FrameNavigationState::const_iterator it = navigation_state.begin();
       it != navigation_state.end(); ++it) {
    int64 frame_id = *it;
    GURL frame_url = navigation_state.GetUrl(frame_id);
    if (!navigation_state.IsValidUrl(frame_url))
      continue;
    linked_ptr<GetAllFrames::Results::DetailsElement> frame(
        new GetAllFrames::Results::DetailsElement());
    frame->url = frame_url.spec();
    frame->frame_id = helpers::GetFrameId(
        navigation_state.IsMainFrame(frame_id), frame_id);
    frame->error_occurred = navigation_state.GetErrorOccurredInFrame(frame_id);
    result_list.push_back(frame);
  }
  results_ = GetAllFrames::Results::Create(result_list);
  return true;
}

}  // namespace extensions
