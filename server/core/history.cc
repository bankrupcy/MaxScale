/*
 * Copyright (c) 2023 MariaDB plc
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2027-02-21
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

#include <maxscale/history.hh>
#include <maxscale/service.hh>
#include <utility>

namespace maxscale
{
History::Subscriber::Subscriber(History& history, std::function<void ()> cb)
    : m_history(history)
    , m_cb(std::move(cb))
{
    m_history.pin_responses(this);
}

History::Subscriber::~Subscriber()
{
    m_history.remove(this);
}

void History::Subscriber::set_current_id(uint32_t id)
{
    m_current_id = id;
}

uint32_t History::Subscriber::current_id() const
{
    return m_current_id;
}

bool History::Subscriber::add_response(bool result_is_ok)
{
    if (m_current_id)
    {
        // It's possible that there's already a response for this command. This can happen if the session
        // command is executed multiple times before the accepted answer has arrived. We only care about
        // the latest result.
        m_ids_to_check[m_current_id] = result_is_ok;

        // Reset the ID after storing it to make sure debug assertions will catch any cases where a PS
        // response is read without a pre-assigned ID.
        m_current_id = 0;
    }

    return compare_responses();
}

bool History::Subscriber::compare_responses()
{
    bool ok = true;
    bool found = false;
    auto it = m_ids_to_check.begin();

    while (it != m_ids_to_check.end())
    {
        if (auto cmd = get(it->first))
        {
            m_history.set_position(this, it->first);

            if (it->second != cmd.value())
            {
                ok = false;
                break;
            }

            it = m_ids_to_check.erase(it);
            found = true;
        }
        else
        {
            ++it;
        }
    }

    if (ok && !found && !m_ids_to_check.empty())
    {
        m_history.need_response(this);
    }

    return ok;
}

const std::deque<GWBUF>& History::Subscriber::history() const
{
    return m_history.m_history;
}

std::optional<bool> History::Subscriber::get(uint32_t id) const
{
    std::optional<bool> rval;

    if (auto it = m_history.m_history_responses.find(id); it != m_history.m_history_responses.end())
    {
        rval = it->second;
    }

    return rval;
}

History::History(size_t limit, bool allow_pruning, bool disable_history)
    : m_max_sescmd_history(disable_history ? 0 : limit)
    , m_allow_pruning(allow_pruning)
{
}

void History::add(GWBUF&& buffer, bool ok)
{
    m_history_responses.emplace(buffer.id(), ok);
    m_history.emplace_back(std::move(buffer));

    if (m_history.size() > m_max_sescmd_history)
    {
        prune_history();
    }
}

bool History::erase(uint32_t id)
{
    bool erased = false;
    auto it = std::remove_if(m_history.begin(), m_history.end(), [&](const auto& buf){
        return buf.id() == id;
    });

    if (it != m_history.end())
    {
        m_history.erase(it, m_history.end());
        erased = true;
    }

    return erased;
}

void History::prune_history()
{
    // Using the about-to-be-pruned command as the minimum ID prevents the removal of responses that are still
    // needed when the ID overflows. If only the stored positions were used, the whole history would be
    // cleared.
    uint32_t min_id = m_history.front().id();

    for (const auto& [sub, info] : m_history_info)
    {
        if (info.position > 0 && info.position < min_id)
        {
            min_id = info.position;
        }
        else if (sub->current_id() != 0 && sub->current_id() < min_id)
        {
            min_id = sub->current_id();
        }
    }

    m_history_responses.erase(m_history_responses.begin(), m_history_responses.lower_bound(min_id));
    m_history.pop_front();
    m_history_pruned = true;
}

void History::pin_responses(Subscriber* backend)
{
    uint32_t id = 0;

    if (!m_history.empty())
    {
        id = m_history.front().id();
    }

    m_history_info[backend].position = id;
}

void History::set_position(Subscriber* backend, uint32_t position)
{
    m_history_info[backend].position = position;
}

void History::need_response(Subscriber* backend)
{
    m_history_info[backend].waiting_for_response = true;
}

void History::remove(Subscriber* backend)
{
    m_history_info.erase(backend);
}

void History::check_early_responses()
{
    // Call compare_responses() for any subscribers that responded before the accepted response was received.
    // Collect the subscribers first into a separate vector: the act of checking the history might end up
    // removing the subscription from the history which will modify m_history_info while we're iterating over
    // it.
    std::vector<Subscriber*> subscribers;

    for (auto& [backend, info] : m_history_info)
    {
        if (std::exchange(info.waiting_for_response, false))
        {
            subscribers.push_back(backend);
        }
    }

    for (auto subscriber : subscribers)
    {
        if (!subscriber->compare_responses())
        {
            subscriber->m_cb();
        }
    }
}

size_t History::runtime_size() const
{
    size_t sescmd_history = 0;

    for (const GWBUF& buf : m_history)
    {
        sescmd_history += buf.runtime_size();
    }

    // The map overhead is ignored.
    sescmd_history += m_history_responses.size() * sizeof(decltype(m_history_responses)::value_type);
    sescmd_history += m_history_info.size() * sizeof(decltype(m_history_info)::value_type);

    return sescmd_history;
}

bool History::can_recover_state() const
{
    bool rval = false;

    if (m_history.empty())
    {
        // Connections can always be recovered if no session commands have been executed
        rval = true;
    }
    else if (m_max_sescmd_history > 0)
    {
        // Recovery is also possible if history pruning is enabled or the history limit hasn't exceeded
        // the limit
        if (m_allow_pruning || !m_history_pruned)
        {
            rval = true;
        }
    }

    return rval;
}
}
