// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/v8_inspector/RemoteObjectId.h"

namespace v8_inspector {

RemoteObjectIdBase::RemoteObjectIdBase() : m_injectedScriptId(0) { }

std::unique_ptr<protocol::DictionaryValue> RemoteObjectIdBase::parseInjectedScriptId(const String16& objectId)
{
    std::unique_ptr<protocol::Value> parsedValue = protocol::parseJSON(objectId);
    if (!parsedValue || parsedValue->type() != protocol::Value::TypeObject)
        return nullptr;

    std::unique_ptr<protocol::DictionaryValue> parsedObjectId(protocol::DictionaryValue::cast(parsedValue.release()));
    bool success = parsedObjectId->getInteger(u8"injectedScriptId", &m_injectedScriptId);
    if (success)
        return parsedObjectId;
    return nullptr;
}

RemoteObjectId::RemoteObjectId() : RemoteObjectIdBase(), m_id(0) { }

std::unique_ptr<RemoteObjectId> RemoteObjectId::parse(ErrorString* errorString, const String16& objectId)
{
    std::unique_ptr<RemoteObjectId> result(new RemoteObjectId());
    std::unique_ptr<protocol::DictionaryValue> parsedObjectId = result->parseInjectedScriptId(objectId);
    if (!parsedObjectId) {
        *errorString = u8"Invalid remote object id";
        return nullptr;
    }

    bool success = parsedObjectId->getInteger(u8"id", &result->m_id);
    if (!success) {
        *errorString = u8"Invalid remote object id";
        return nullptr;
    }
    return result;
}

RemoteCallFrameId::RemoteCallFrameId() : RemoteObjectIdBase(), m_frameOrdinal(0) { }

std::unique_ptr<RemoteCallFrameId> RemoteCallFrameId::parse(ErrorString* errorString, const String16& objectId)
{
    std::unique_ptr<RemoteCallFrameId> result(new RemoteCallFrameId());
    std::unique_ptr<protocol::DictionaryValue> parsedObjectId = result->parseInjectedScriptId(objectId);
    if (!parsedObjectId) {
        *errorString = u8"Invalid call frame id";
        return nullptr;
    }

    bool success = parsedObjectId->getInteger(u8"ordinal", &result->m_frameOrdinal);
    if (!success) {
        *errorString = u8"Invalid call frame id";
        return nullptr;
    }

    return result;
}

String16 RemoteCallFrameId::serialize(int injectedScriptId, int frameOrdinal)
{
    return u8"{\"ordinal\":" + String16::fromInteger(frameOrdinal) + u8",\"injectedScriptId\":" + String16::fromInteger(injectedScriptId) + u8"}";
}

} // namespace v8_inspector
