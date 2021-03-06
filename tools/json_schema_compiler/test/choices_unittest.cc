// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/json_schema_compiler/test/choices.h"

#include "testing/gtest/include/gtest/gtest.h"

using namespace test::api::choices;

TEST(JsonSchemaCompilerChoicesTest, TakesIntegersParamsCreate) {
  {
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(Value::CreateBooleanValue(true));
    scoped_ptr<TakesIntegers::Params> params(
        TakesIntegers::Params::Create(*params_value));
    EXPECT_FALSE(params.get());
  }
  {
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(Value::CreateIntegerValue(6));
    scoped_ptr<TakesIntegers::Params> params(
        TakesIntegers::Params::Create(*params_value));
    EXPECT_TRUE(params.get());
    EXPECT_EQ(TakesIntegers::Params::NUMS_INTEGER, params->nums_type);
    EXPECT_FALSE(params->nums_array.get());
    EXPECT_EQ(6, *params->nums_integer);
  }
  {
    scoped_ptr<ListValue> params_value(new ListValue());
    scoped_ptr<ListValue> integers(new ListValue());
    integers->Append(Value::CreateIntegerValue(6));
    integers->Append(Value::CreateIntegerValue(8));
    params_value->Append(integers.release());
    scoped_ptr<TakesIntegers::Params> params(
        TakesIntegers::Params::Create(*params_value));
    EXPECT_TRUE(params.get());
    EXPECT_EQ(TakesIntegers::Params::NUMS_ARRAY, params->nums_type);
    EXPECT_EQ((size_t) 2, (*params->nums_array).size());
    EXPECT_EQ(6, (*params->nums_array)[0]);
    EXPECT_EQ(8, (*params->nums_array)[1]);
  }
}

TEST(JsonSchemaCompilerChoicesTest, ObjectWithChoicesParamsCreate) {
  {
    scoped_ptr<DictionaryValue> object_param(new DictionaryValue());
    object_param->SetWithoutPathExpansion("strings",
        Value::CreateStringValue("asdf"));
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(object_param.release());
    scoped_ptr<ObjectWithChoices::Params> params(
        ObjectWithChoices::Params::Create(*params_value));
    EXPECT_TRUE(params.get());
    EXPECT_EQ(ObjectWithChoices::Params::StringInfo::STRINGS_STRING,
        params->string_info.strings_type);
    EXPECT_EQ("asdf", *params->string_info.strings_string);
  }
  {
    scoped_ptr<DictionaryValue> object_param(new DictionaryValue());
    object_param->SetWithoutPathExpansion("strings",
        Value::CreateStringValue("asdf"));
    object_param->SetWithoutPathExpansion("integers",
        Value::CreateIntegerValue(6));
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(object_param.release());
    scoped_ptr<ObjectWithChoices::Params> params(
        ObjectWithChoices::Params::Create(*params_value));
    EXPECT_TRUE(params.get());
    EXPECT_EQ(ObjectWithChoices::Params::StringInfo::STRINGS_STRING,
        params->string_info.strings_type);
    EXPECT_EQ("asdf", *params->string_info.strings_string);
    EXPECT_EQ(ObjectWithChoices::Params::StringInfo::INTEGERS_INTEGER,
        params->string_info.integers_type);
    EXPECT_EQ(6, *params->string_info.integers_integer);
  }
}

TEST(JsonSchemaCompilerChoicesTest, ObjectWithChoicesParamsCreateFail) {
  {
    scoped_ptr<DictionaryValue> object_param(new DictionaryValue());
    object_param->SetWithoutPathExpansion("strings",
        Value::CreateIntegerValue(5));
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(object_param.release());
    scoped_ptr<ObjectWithChoices::Params> params(
        ObjectWithChoices::Params::Create(*params_value));
    EXPECT_FALSE(params.get());
  }
  {
    scoped_ptr<DictionaryValue> object_param(new DictionaryValue());
    object_param->SetWithoutPathExpansion("strings",
        Value::CreateStringValue("asdf"));
    object_param->SetWithoutPathExpansion("integers",
        Value::CreateStringValue("asdf"));
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(object_param.release());
    scoped_ptr<ObjectWithChoices::Params> params(
        ObjectWithChoices::Params::Create(*params_value));
    EXPECT_FALSE(params.get());
  }
  {
    scoped_ptr<DictionaryValue> object_param(new DictionaryValue());
    object_param->SetWithoutPathExpansion("integers",
        Value::CreateIntegerValue(6));
    scoped_ptr<ListValue> params_value(new ListValue());
    params_value->Append(object_param.release());
    scoped_ptr<ObjectWithChoices::Params> params(
        ObjectWithChoices::Params::Create(*params_value));
    EXPECT_FALSE(params.get());
  }
}

TEST(JsonSchemaCompilerChoicesTest, ReturnChoices) {
  {
    std::vector<int> integers;
    integers.push_back(1);
    integers.push_back(2);
    scoped_ptr<ListValue> array_results =
        ReturnChoices::Results::Create(integers);

    ListValue expected;
    ListValue* expected_argument = new ListValue();
    expected_argument->Append(Value::CreateIntegerValue(1));
    expected_argument->Append(Value::CreateIntegerValue(2));
    expected.Append(expected_argument);
    EXPECT_TRUE(array_results->Equals(&expected));
  }
  {
    scoped_ptr<ListValue> integer_results = ReturnChoices::Results::Create(5);
    ListValue expected;
    expected.Append(Value::CreateIntegerValue(5));
    EXPECT_TRUE(integer_results->Equals(&expected));
  }
}
