#ifndef __MORDOR_HTTP_CONNECTION_H__
#define __MORDOR_HTTP_CONNECTION_H__
// Copyright (c) 2009 - Mozy, Inc.

#include <mutex>
#include <boost/function.hpp>

#include "mordor/anymap.h"
#include "http.h"

namespace Mordor {

class Stream;

namespace HTTP {

class Connection
{
public:
    std::shared_ptr<Stream> stream() { return m_stream; }

    static bool hasMessageBody(const GeneralHeaders &general,
        const EntityHeaders &entity,
        const std::string &method,
        Status status,
        bool includeEmpty = true);

    /// @note Be sure to lock the cacheMutex whenever you access the cache
    anymap &cache() { return m_cache; }
    std::mutex &cacheMutex() { return m_cacheMutex; }

    /// lock cacheMutex and return a copy of value
    template <class TagType>
    typename TagType::value_type getCache(const TagType &key)
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        return m_cache[key];
    }

    /// update cache with lock
    template <class TagType>
    void setCache(const TagType &key, const typename TagType::value_type& value)
    {
        std::lock_guard<std::mutex> lock(m_cacheMutex);
        m_cache[key] = value;
    }

protected:
    Connection(std::shared_ptr<Stream> stream);

    std::shared_ptr<Stream> getStream(const GeneralHeaders &general,
        const EntityHeaders &entity,
        const std::string &method,
        Status status,
        boost::function<void ()> notifyOnEof,
        boost::function<void ()> notifyOnException,
        bool forRead);

protected:
    std::shared_ptr<Stream> m_stream;

private:
    std::mutex m_cacheMutex;
    anymap m_cache;
};

}}

#endif
