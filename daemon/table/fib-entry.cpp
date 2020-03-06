/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2020 Operant Networks, Incorporated.
 * @author: Van Jacobson <info@pollere.net>
 *
 * This works is based substantially on previous work as listed below:
 *
 * Original file: daemon/table/fib-entry.cpp
 * Original repository: https://github.com/named-data/NFD
 *
 * Summary of Changes: Fix delay in communication involving ephemeral processes. https://github.com/pollere/NDNpatches
 *
 * which was originally released under the LGPL license with the following rights:
 *
 * Copyright (c) 2014-2019,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fib-entry.hpp"
#include "name-tree-entry.hpp"
#include "common/logger.hpp"

namespace nfd {
NFD_LOG_INIT(FibEntry);
namespace fib {

Entry::Entry(const Name& prefix)
  : m_prefix(prefix)
{
}

NextHopList::iterator
Entry::findNextHop(const Face& face, EndpointId endpointId)
{
  return std::find_if(m_nextHops.begin(), m_nextHops.end(),
                      [&face, endpointId] (const NextHop& nexthop) {
                        return &nexthop.getFace() == &face && nexthop.getEndpointId() == endpointId;
                      });
}

bool
Entry::hasNextHop(const Face& face, EndpointId endpointId) const
{
  return const_cast<Entry*>(this)->findNextHop(face, endpointId) != m_nextHops.end();
}

void
Entry::sendPendingInterests(const name_tree::Entry* nte, const NextHop& newHop)
{
  if (nte == nullptr || (!nte->hasChildren() && !nte->hasPitEntries())) {
    return;
  }
  // send this entry's pending interests to 'newHop' then recurse
  // on entry's children.
  NFD_LOG_DEBUG("sendPendingInterests for " << nte->getName() << " to face "
                << newHop.getFace().getId());
  if (nte->getStrategyChoiceEntry() != nullptr) {
    NFD_LOG_DEBUG("found strategy " <<
      nte->getStrategyChoiceEntry()->getPrefix() << " " <<
      nte->getStrategyChoiceEntry()->getStrategyInstanceName());
  }
  for (auto pe : nte->getPitEntries()) {
    if (! pe->isSatisfied) {
      newHop.getFace().sendInterest(pe->getInterest(), newHop.getEndpointId());
      NFD_LOG_DEBUG("sent Interest " << pe->getName());
    }
  }
  for (auto ce : nte->getChildren()) {
    sendPendingInterests(ce, newHop);
  }
}

void
Entry::addOrUpdateNextHop(Face& face, EndpointId endpointId, uint64_t cost)
{
  auto it = this->findNextHop(face, endpointId);
  if (it == m_nextHops.end()) {
    m_nextHops.emplace_back(face, endpointId);
    it = std::prev(m_nextHops.end());
  } else if (it->getCost() == cost) {
    // nothing changed
    return;
  }
  it->setCost(cost);
  this->sendPendingInterests(m_nameTreeEntry, *it);
  this->sortNextHops();
}

void
Entry::removeNextHop(const Face& face, EndpointId endpointId)
{
  auto it = this->findNextHop(face, endpointId);
  if (it != m_nextHops.end()) {
    m_nextHops.erase(it);
  }
}

void
Entry::removeNextHopByFace(const Face& face)
{
  auto it = std::remove_if(m_nextHops.begin(), m_nextHops.end(),
                           [&face] (const NextHop& nexthop) {
                             return &nexthop.getFace() == &face;
                           });
  m_nextHops.erase(it, m_nextHops.end());
}

void
Entry::sortNextHops()
{
  std::sort(m_nextHops.begin(), m_nextHops.end(),
            [] (const NextHop& a, const NextHop& b) { return a.getCost() < b.getCost(); });
}

} // namespace fib
} // namespace nfd
