// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/move.h"
#include "base/time.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/stream_parser_buffer.h"
#include "media/base/test_data_util.h"
#include "media/base/video_decoder_config.h"
#include "media/mp4/mp4_stream_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;

namespace media {
namespace mp4 {

class MP4StreamParserTest : public testing::Test {
 public:
  MP4StreamParserTest()
      : parser_(new MP4StreamParser(false)),
        got_configs_(false) {
  }

 protected:
  scoped_ptr<MP4StreamParser> parser_;
  base::TimeDelta segment_start_;
  bool got_configs_;

  bool AppendData(const uint8* data, size_t length) {
    parser_->Parse(data, length);
    return true;
  }

  bool AppendDataInPieces(const uint8* data, size_t length) {
    return AppendDataInPieces(data, length, 7);
  }

  bool AppendDataInPieces(const uint8* data, size_t length, size_t piece_size) {
    const uint8* start = data;
    const uint8* end = data + length;
    while (start < end) {
      size_t append_size = std::min(piece_size,
                                    static_cast<size_t>(end - start));
      if (!AppendData(start, append_size))
        return false;
      start += append_size;
    }
    return true;
  }

  void InitF(bool init_ok, base::TimeDelta duration) {
    DVLOG(1) << "InitF: ok=" << init_ok
              << ", dur=" << duration.InMilliseconds();
  }

  bool NewConfigF(const AudioDecoderConfig& ac,
                   const VideoDecoderConfig& vc) {
    DVLOG(1) << "NewConfigF: audio=" << ac.IsValidConfig()
             << ", video=" << vc.IsValidConfig();

    // TODO(strobe): Until http://crbug.com/122913 is fixed, we want to make
    // sure that this callback isn't called more than once per stream. Remove
    // when that bug is fixed.
    EXPECT_FALSE(got_configs_);
    got_configs_ = true;
    return true;
  }

  bool NewBuffersF(const StreamParser::BufferQueue& bufs) {
    DVLOG(2) << "NewBuffersF: " << bufs.size() << " buffers";
    for (StreamParser::BufferQueue::const_iterator buf = bufs.begin();
         buf != bufs.end(); buf++) {
      DVLOG(3) << "  n=" << buf - bufs.begin()
               << ", size=" << (*buf)->GetDataSize()
               << ", dur=" << (*buf)->GetDuration().InMilliseconds();
      EXPECT_LE(segment_start_, (*buf)->GetTimestamp());
    }
    return true;
  }

  bool KeyNeededF(scoped_array<uint8> init_data, int init_data_size) {
    DVLOG(1) << "KeyNeededF: " << init_data_size;
    return true;
  }

  void NewSegmentF(TimeDelta start_dts) {
    DVLOG(1) << "NewSegmentF: " << start_dts.InMilliseconds();
    segment_start_ = start_dts;
  }

  void InitializeParser() {
    parser_->Init(
        base::Bind(&MP4StreamParserTest::InitF, base::Unretained(this)),
        base::Bind(&MP4StreamParserTest::NewConfigF, base::Unretained(this)),
        base::Bind(&MP4StreamParserTest::NewBuffersF, base::Unretained(this)),
        base::Bind(&MP4StreamParserTest::NewBuffersF, base::Unretained(this)),
        base::Bind(&MP4StreamParserTest::KeyNeededF, base::Unretained(this)),
        base::Bind(&MP4StreamParserTest::NewSegmentF, base::Unretained(this)));
  }

  bool ParseMP4File(const std::string& filename, int append_size) {
    InitializeParser();

    scoped_refptr<DecoderBuffer> buffer = ReadTestDataFile(filename);
    EXPECT_TRUE(AppendDataInPieces(buffer->GetData(),
                                   buffer->GetDataSize(),
                                   append_size));
    return true;
  }
};

TEST_F(MP4StreamParserTest, TestParseBearDASH) {
  ParseMP4File("bear.1280x720_dash.mp4", 512);
}

TEST_F(MP4StreamParserTest, TestMultiFragmentAppend) {
  ParseMP4File("bear.1280x720_dash.mp4", 768432);
}

TEST_F(MP4StreamParserTest, TestReinitialization) {
  InitializeParser();

  scoped_refptr<DecoderBuffer> buffer =
      ReadTestDataFile("bear.1280x720_dash.mp4");
  EXPECT_TRUE(AppendDataInPieces(buffer->GetData(),
                                 buffer->GetDataSize(),
                                 512));
  EXPECT_TRUE(AppendDataInPieces(buffer->GetData(),
                                 buffer->GetDataSize(),
                                 512));
}

}  // namespace mp4
}  // namespace media
