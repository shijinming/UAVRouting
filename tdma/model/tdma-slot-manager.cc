#include "tdma-slot-manager.h"
#include "ns3/random-variable-stream.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/node-list.h"


NS_LOG_COMPONENT_DEFINE ("TdmaSlotManager");

namespace tdma{
    TdmaSlot::TdmaSlot (uint32_t index)
            : m_internal(false),
              m_state(TdmaSlot::FREE),
              m_previousState(TdmaSlot::FREE),
              m_index(index),
              m_timeout(0),
              m_internalTimeout(0),
              m_notBefore(ns3::Seconds(0)) {}

    TdmaSlot::~TdmaSlot() {}
    TdmaSlot::State TdmaSlot::GetState() {return m_state;}
    bool TdmaSlot::IsInternallyAllocated() {return m_internal;}
    void TdmaSlot::MarkAsFree() {
        m_previousState = m_state;
        m_state = TdmaSlot::FREE;
        m_internal = false;
        m_timeout = 0;
        m_internalTimeout = 0;
        m_notBefore = ns3::Seconds(0);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlot:MarkAsFree() marked slot "
                                           << m_index << " as free (timeout = "
                                           << (uint32_t) m_timeout << ")");
    }
    bool TdmaSlot::IsFree() {
        return IsFree(ns3::Seconds(0));
    }
    bool TdmaSlot::IsFree(ns3::Time until) {
        return (m_state == TdmaSlot::FREE || (m_previousState == TdmaSlot::FREE && until < m_notBefore));
    }
    void TdmaSlot::MarkAsAllocated(uint8_t timeout, ns3::Mac48Address owner, ns3::Vector position,
                                   ns3::Time notBefore) {
        NS_LOG_FUNCTION_NOARGS();
        if (IsAllocated()) {
            if (timeout > m_timeout) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlot:MarkAsAllocated() updating slot "
                                                   << m_index << " to new timeout = " << (uint32_t) timeout);
                m_timeout = timeout;
            }
            else {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlot:MarkAsAllocated() not updating slot " << m_index
                                                   << " to new timeout since already known timeout value is larger ("
                                                   << (uint32_t) timeout << "<=" << (uint32_t) m_timeout << ")");
            }
            if (notBefore < m_notBefore) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlot:MarkAsAllocated() updated notBefore to "
                                                   << notBefore.GetSeconds());
                m_notBefore = notBefore;
            }
        }
        else {
            m_previousState = m_state;
            m_timeout = timeout;
            m_state = TdmaSlot::ALLOCATED;
            m_notBefore = notBefore;
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlot:MarkAsAllocated() marking slot "
                                               << m_index << " as allocated with timeout = "
                                               << (uint32_t) m_timeout << " and notBefore = "
                                               << notBefore.GetSeconds());
        }
        m_position = position;
        m_owner = owner;
    }
    bool TdmaSlot::IsAllocated() {
        return (m_state == TdmaSlot::ALLOCATED && ns3::Simulator::Now() >= m_notBefore);
    }
    void TdmaSlot::MarkAsInternallyAllocated(uint8_t timeout) {
        NS_LOG_FUNCTION((uint32_t) timeout);
        m_internal = true;
        m_internalTimeout = timeout;
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlot:MarkAsInternallyAllocated() marked slot "
                                           << m_index << " as internally allocated (timeout = "
                                           << (uint32_t) m_internalTimeout << ")");
    }
    void TdmaSlot::MarkAsBusy() {
        m_previousState = m_state;
        m_state = TdmaSlot::BUSY;
        m_notBefore = ns3::Seconds(0);
        m_timeout = 1;
    }
    bool TdmaSlot::IsBusy() {
        return (m_state == TdmaSlot::BUSY && ns3::Simulator::Now() >= m_notBefore);
    }
    void TdmaSlot::RebaseIndex(uint32_t index) {
        m_index = index;
    }

    uint32_t TdmaSlot::GetSlotIndex() {
        return m_index;
    }
    void TdmaSlot::SetTimeout(uint8_t t) {
        m_timeout = t;
    }
    void TdmaSlot::SetInternalTimeout(uint8_t t) {
        m_internalTimeout = t;
    }
    uint8_t TdmaSlot::GetTimeout() {
        return m_timeout;
    }
    uint8_t TdmaSlot::GetInternalTimeout() {
        return m_internalTimeout;
    }
    ns3::Vector TdmaSlot::GetPosition() {
        return m_position;
    }
    ns3::Mac48Address TdmaSlot::GetOwner() {
        return m_owner;
    }

    RandomAccessDetails::RandomAccessDetails ()
            : m_when(ns3::Seconds(0)),
              m_p(0.0),
              m_slotsLeft(0) {}
    RandomAccessDetails::~RandomAccessDetails() {}
    void RandomAccessDetails::SetWhen (ns3::Time when) {
        m_when = when;
    }
    void RandomAccessDetails::SetProbability (double p) {
        m_p = p;
    }
    void RandomAccessDetails::SetRemainingSlots (uint16_t left) {
        m_slotsLeft = left;
    }
    ns3::Time RandomAccessDetails::GetWhen() {
        return m_when;
    }
    double RandomAccessDetails::GetProbability() {
        return m_p;
    }
    uint16_t RandomAccessDetails::GetRemainingSlots() {
        return m_slotsLeft;
    }

    ns3::TypeId TdmaSlotManager::GetTypeId (void) {
        static ns3::TypeId tid = ns3::TypeId("tdma::TdmaSlotManager")
                .SetParent<Object>().AddConstructor<TdmaSlotManager>()
                .AddTraceSource("NominalSlotSelection",
                                "This event is triggered when the station selects its nominal slot set",
                                ns3::MakeTraceSourceAccessor (&TdmaSlotManager::m_nominalSlotTrace))
                .AddTraceSource("SlotReservation",
                                "This event is triggered when the station selects a slot for the first time",
                                ns3::MakeTraceSourceAccessor (&TdmaSlotManager::m_reservationTrace))
                .AddTraceSource("SlotReReservation",
                                "This event is triggered when the station performs a re-reservation of a slot",
                                ns3::MakeTraceSourceAccessor (&TdmaSlotManager::m_reReservationTrace));
        return tid;
    }
    TdmaSlotManager::TdmaSlotManager()
            : m_slotDuration(ns3::Seconds(0)),
              m_rate(0),
              m_ni(0),
              m_siHalf(0),
              m_current(0),
              m_mininumCandidates(0),
              m_numSlots(0) {}
    TdmaSlotManager::~TdmaSlotManager() {}

    void TdmaSlotManager::Setup(ns3::Time start, ns3::Time frameDuration,
                                ns3::Time slotDuration, uint32_t candidateSlots) {
        NS_LOG_FUNCTION(start << frameDuration << slotDuration << candidateSlots);
        m_mininumCandidates = candidateSlots;
        m_slotDuration = slotDuration;
        m_numSlots = floor(frameDuration.GetSeconds() / m_slotDuration.GetSeconds());
        m_frameDuration = ns3::NanoSeconds(m_slotDuration.GetNanoSeconds() * m_numSlots);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:TdmaSlotManager() frameDuration = "
                                           << m_frameDuration.GetSeconds()
                                           << ", slotDuration = " << m_slotDuration.GetSeconds()
                                           << ", numSlots = " << m_numSlots);
        // Then determine the properly aligned starting time of the frame
        double slotsUntilStart = start.GetSeconds() / m_slotDuration.GetSeconds();
        if (slotsUntilStart == floor(slotsUntilStart)) {
            m_start = start;
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:TdmaSlotManager() setting m_start to "
                                               << m_start << " (start was " << start << ")");
        }
        else {
            m_start = ns3::NanoSeconds(m_slotDuration.GetNanoSeconds() * ceil(slotsUntilStart));
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:TdmaSlotManager() setting m_start to "
                                               << m_start << " (start was " << start << ")");
        }
        // Afterwards initialize the vector of slot objects
        for (uint32_t i = 0; i < m_numSlots; i++) {
            ns3::Ptr<TdmaSlot> slot = ns3::Create<TdmaSlot>(i);
            m_slots[i] = slot;
        }
        // Remember the start of the last/current frame
        m_lastFrameStart = m_start;
    }
    ns3::Time TdmaSlotManager::GetStart() {
        return m_start;
    }
    ns3::Time TdmaSlotManager::GetFrameDuration() {
        return m_frameDuration;
    }
    uint32_t TdmaSlotManager::GetSlotsPerFrame() {
        return m_numSlots;
    }
    void TdmaSlotManager::SetReportRate(uint32_t rate) {
        m_rate = rate;
        m_ni = floor(1.0 * m_numSlots / m_rate);
    }
    void TdmaSlotManager::SetMinimumCandidateSlotSetSize (uint32_t size) {
        m_mininumCandidates = size;
    }
    void TdmaSlotManager::SetSelectionIntervalRatio(double ratio) {
        m_siHalf = floor(0.5 * (m_ni-1) * ratio);
    }
    void TdmaSlotManager::SelectNominalSlots() {
        NS_LOG_FUNCTION_NOARGS();
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectNominalSlots() now = " << ns3::Simulator::Now());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectNominalSlots() m_lastFrameStart = "
                                           << m_lastFrameStart << ", m_frameDuration = " << m_frameDuration);
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        // First clear all previous NTS values
        m_nss.clear();
        // Randomly calculate the nominal start slot within the interval
        // [0,NI)
        ns3::Ptr<ns3::UniformRandomVariable> randomizer = ns3::CreateObject<ns3::UniformRandomVariable> ();;
        uint32_t NSS = floor (randomizer->GetValue(0.0, m_ni));
        NS_ASSERT(NSS < m_ni);
        m_nss.push_back(NSS);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectNominalSlots() NTS = " << NSS);
        // Then derive the remaining NS values from NSS
        for (uint32_t i = 1; i < m_rate; i++) {
            NS_ASSERT((NSS + (i * m_ni)) < m_numSlots);
            m_nss.push_back(NSS + (i * m_ni));
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:SelectNominalSlots() "
                                               << i << ". NS = " << NSS + (i * m_ni));
        }
        // Trace the nominal slots, but re-base them to an imaginary frame start at 0 seconds
        std::vector<uint32_t> tracedNss;
        for (uint32_t i = 0; i < m_nss.size(); i++) {
            uint32_t index = m_nss[i];
            ns3::Time t = GetTimeForSlotIndex(index);
            uint32_t globalIndex = GetGlobalSlotIndexForTimestamp(t);
            index = (globalIndex % m_numSlots);
            NS_ASSERT(index >= 0 && index < m_numSlots);
            tracedNss.push_back(index);
        }
        m_nominalSlotTrace(tracedNss);
    }
    ns3::Time TdmaSlotManager::GetTimeForSlotIndex(uint32_t index) {
        NS_LOG_FUNCTION(index);
        ns3::Time delayInFrame = ns3::NanoSeconds(index * m_slotDuration.GetNanoSeconds());
        return m_lastFrameStart + delayInFrame;
    }
    uint32_t TdmaSlotManager::GetCurrentReservationNo() {
        NS_LOG_FUNCTION_NOARGS();
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        uint32_t result = m_current;
        if (m_current < (m_rate - 1)) {
            m_current++;
        }
        else {
            m_current = 0;
        }
        return result;
    }
    void TdmaSlotManager::SelectTransmissionSlotForReservationWithNo (uint32_t n, uint8_t timeout) {
        NS_LOG_FUNCTION(this << n << (uint32_t) timeout);
        NS_ASSERT(n < m_nss.size());
        ns3::Time until = ns3::Simulator::Now() + ns3::Seconds(timeout * m_frameDuration.GetSeconds());
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        // 1) get the nominal slot for this packet
        uint32_t NS = m_nss[n];
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                   "nominal slot = " << NS);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                   "numSlots = " << m_numSlots);
        // 2) compile the list of candidate slots
        std::vector<uint32_t> candidates;
        std::map<double, uint32_t> usedSlots;
        int32_t lowerBound = (int32_t) NS - (int32_t) m_siHalf;
        int32_t upperBound = (int32_t) NS + (int32_t) m_siHalf;
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                   "selection interval: [" << lowerBound << ":" << upperBound << "]");
        for (int32_t i = lowerBound; i <= upperBound; i++) {
            // Handle negative indices properly by wrapping around
            int32_t j = i;
            // ... but skip negative indices when this is the first packet to be sent at all
            if (i < 0 && m_selections.empty()) {
                continue;
            }
            else if (i < 0) {
                j = m_numSlots + i;
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                           "negative index, wrapping around: " << i << " -> " << j);
            }
            else if (i >= int32_t(m_numSlots)) {
                j = i - m_numSlots;
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                           "too large index, wrapping around: " << i << " -> " << j);
            }
            // Then fetch the corresponding slot object and add it to the list according to its current status
            NS_ASSERT(j >= 0);
            NS_ASSERT(j < int32_t(m_slots.size()));
            ns3::Ptr<TdmaSlot> slot = m_slots[j];
            if (slot->IsFree(until)) {
                candidates.push_back(slot->GetSlotIndex());
            }
            else if (slot->IsAllocated() && !slot->IsInternallyAllocated()) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                           "not considering slot " << slot->GetSlotIndex()
                                                   << " yet since it is already allocated by someone else.");
                ns3::Vector owner = slot->GetPosition();
                ns3::Vector myself =
                        ns3::NodeList::GetNode(ns3::Simulator::GetContext())->GetObject<ns3::MobilityModel>()->GetPosition();
                double distance = ns3::CalculateDistance(owner, myself);
                while (usedSlots.find(distance) != usedSlots.end()) {
                    distance += 0.000001;
                }
                usedSlots[distance] = slot->GetSlotIndex();
            }
            else if (slot->IsBusy()) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                           "not considering slot " << slot->GetSlotIndex()
                                                   << " yet since it is marked busy.");
            }
        }
        uint32_t numFree = candidates.size();
        // If we do not have enough slots yet, we will add
        if (candidates.size() < m_mininumCandidates) {
            // Add as many allocated slots until we have at least 4 candidates
            // but only add slots owned by vehicles with which we do not collide yet
            uint32_t missing = m_mininumCandidates - candidates.size();
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                               << missing << " slots need to be added that are already allocated");
            std::map<double, uint32_t>::reverse_iterator it;
            for (it = usedSlots.rbegin(); it != usedSlots.rend(); ++it) {
                uint32_t index = it->second;
                ns3::Mac48Address owner = m_slots[index]->GetOwner();
                // Add this slot if we do not already have a collision with the owner of it
                if (m_collisions.find(owner) == m_collisions.end()) {
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                               "adding already allocated slot " << index
                                                       << " to candidate set since its owner is "
                                                       << it->first << " meters away");
                    candidates.push_back(index);
                    missing--;
                    if (missing == 0) {
                        break;
                    }
                }
                else {
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                               "not adding already allocated slot " << index
                                                       << " to candidate set because we already have a collision with the owner.");
                }
            }
        }
        // 3) randomly select one of these candidate slots
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                           << candidates.size() << " candidates available to choose from");
        ns3::Ptr<ns3::UniformRandomVariable> randomizer = ns3::CreateObject<ns3::UniformRandomVariable> ();;
        uint32_t selection = floor(randomizer->GetValue(0.0, candidates.size()-0.00001));
        NS_ASSERT(selection < candidates.size());
        uint32_t index = candidates[selection];
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:SelectTransmissionSlotForReservationWithNo() "
                                                   "reserved slot: " << index << ", timeout will be set to: "
                                           << (uint32_t) timeout);
        // 4a) mark this slot as internally allocated
        ns3::Ptr<TdmaSlot> slot = m_slots[index];
        bool wasFree = slot->IsFree();
        slot->MarkAsInternallyAllocated(timeout);
        m_slots[index] = slot;
        // 4b) and remember that we now introduced a collision with this node in case
        //     this slot was already externally allocated
        if (slot->IsAllocated()) {
            ns3::Mac48Address owner = slot->GetOwner();
            m_collisions[owner] = index;
        }
        // 5) Save this slot also in the map of scheduled transmissions
        m_selections[n] = slot;
        // Trace this event...
        m_reservationTrace(candidates.size(), numFree, wasFree);
    }

    uint32_t TdmaSlotManager::ReSelectTransmissionSlotForReservationWithNo (uint32_t n, uint8_t timeout)
    {
        NS_LOG_FUNCTION(this << n << (uint32_t) timeout);
        NS_ASSERT(m_selections.find(n) != m_selections.end());
        NS_ASSERT(n < m_nss.size());
        ns3::Time until = ns3::Simulator::Now() + ns3::Seconds(timeout * m_frameDuration.GetSeconds());
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        // We basically perform exactly the same procedure as in the first frame phase, except that
        // we need to release the old slot before
        uint32_t oldIndex = m_selections[n]->GetSlotIndex();
        m_slots[oldIndex]->MarkAsFree();
        // 1) get the nominal slot for this packet
        uint32_t NS = m_nss[n];
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                   "nominal slot = " << NS);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                   "numSlots = " << m_numSlots);
        // 2) compile the list of candidate slots
        std::vector<uint32_t> candidates;
        std::map<double, uint32_t> usedSlots;
        int32_t lowerBound = (int32_t) NS - (int32_t) m_siHalf;
        int32_t upperBound = (int32_t) NS + (int32_t) m_siHalf;
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                   "selection interval: [" << lowerBound << ":" << upperBound << "]");
        for (int32_t i = lowerBound; i <= upperBound; i++) {
            // Handle negative indices properly by wrapping around
            int32_t j = i;
            // Wrap around if the index is smaller than zero...
            if (i < 0) {
                j = m_numSlots + i;
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                           "negative index, wrapping around: " << i << " -> " << j);
            }
            else if (i >= int32_t(m_numSlots)) { // ... or greater than the maximum number of slots per frame
                j = i - m_numSlots;
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                           "too large index, wrapping around: " << i << " -> " << j);
            }
            // Then fetch the corresponding slot object and add it to the list according to its current status
            NS_ASSERT(j >= 0);
            NS_ASSERT(j < int32_t(m_slots.size()));
            ns3::Ptr<TdmaSlot> slot = m_slots[j];
            if (slot->IsFree(until)) {
                candidates.push_back(slot->GetSlotIndex());
            }
            else if (slot->IsAllocated() && !slot->IsInternallyAllocated()) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                           "not considering slot " << slot->GetSlotIndex()
                                                   << " yet since it is already allocated by someone else.");
                ns3::Vector owner = slot->GetPosition();
                ns3::Vector myself =
                        ns3::NodeList::GetNode(ns3::Simulator::GetContext())->GetObject<ns3::MobilityModel>()->GetPosition();
                double distance = ns3::CalculateDistance(owner, myself);
                while (usedSlots.find(distance) != usedSlots.end()) {
                    distance += 0.000001;
                }
                usedSlots[distance] = slot->GetSlotIndex();
            }
            else if (slot->IsBusy()) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                           "not considering slot " << slot->GetSlotIndex()
                                                   << " yet since it is marked busy.");
            }
        }
        uint32_t numFree = candidates.size();
        // If we do not have enough slots yet, we will add
        if (candidates.size() < m_mininumCandidates) {
            // Add as many allocated slots until we have at least 4 candidates
            // but only add slots owned by vehicles with which we do not collide yet
            uint32_t missing = m_mininumCandidates - candidates.size();
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                               << missing << " slots need to be added that are already allocated");
            std::map<double, uint32_t>::reverse_iterator it;
            for (it = usedSlots.rbegin(); it != usedSlots.rend(); ++it) {
                uint32_t index = it->second;
                ns3::Mac48Address owner = m_slots[index]->GetOwner();

                // Add this slot if we do not already have a collision with the owner of it
                if (m_collisions.find(owner) == m_collisions.end()) {
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                               "adding already allocated slot " << index
                                                       << " to candidate set since its owner is " << it->first << " meters away");
                    candidates.push_back(index);
                    missing--;
                    if (missing == 0) {
                        break;
                    }
                }
                else {
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                               "not adding already allocated slot " << index
                                                       << " to candidate set because we already have a collision with the owner.");
                }
            }
        }
        // 3) randomly select one of these candidate slots
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                           << candidates.size() << " candidates available to choose from");
        ns3::Ptr<ns3::UniformRandomVariable> randomizer = ns3::CreateObject<ns3::UniformRandomVariable> ();;
        uint32_t selection = floor(randomizer->GetValue(0.0, candidates.size()-0.00001));
        NS_ASSERT(selection < candidates.size());
        uint32_t newIndex = candidates[selection];
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:ReSelectTransmissionSlotForReservationWithNo() "
                                                   "newly reserved slot: " << newIndex);
        // 4a) mark this slot as internally allocated
        ns3::Ptr<TdmaSlot> slot = m_slots[newIndex];
        bool wasFree = slot->IsFree();
        bool isSame = (oldIndex == newIndex);
        slot->MarkAsInternallyAllocated(timeout);
        NS_ASSERT(slot->IsInternallyAllocated());
        NS_ASSERT(slot->GetInternalTimeout() == timeout);
        m_slots[newIndex] = slot;
        // 4b) and remember that we now introduced a collision with this node in case
        //     this slot was already externally allocated
        if (slot->IsAllocated()) {
            ns3::Mac48Address owner = slot->GetOwner();
            m_collisions[owner] = newIndex;
        }
        // 5) Save this slot also in the map of scheduled transmissions
        m_selections[n] = slot;
        // Trace this event...
        m_reReservationTrace(candidates.size(), numFree, wasFree, isSame);
        // 6) Return offset to previously reserved slot
        return (newIndex - oldIndex) + m_numSlots;
    }
    ns3::Time TdmaSlotManager::GetTimeUntilTransmissionOfReservationWithNo (uint32_t n) {
        NS_LOG_FUNCTION(this << n);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        // Starting point: the clean time that has passed since the start of this slot manager
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetTimeUntilTransmissionOfReservationWithNo() "
                                                   "last frame started at " << m_lastFrameStart);
        // ...determine the slot that is going to be used for this packet and derive the additional waiting time.
        NS_ASSERT(m_selections.find(n) != m_selections.end());
        ns3::Ptr<TdmaSlot> slot = m_selections[n];
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetTimeUntilTransmissionOfReservationWithNo() "
                                                   "slot chosen: " << slot->GetSlotIndex());
        ns3::Time delayInFrame = ns3::NanoSeconds(slot->GetSlotIndex() * m_slotDuration.GetNanoSeconds());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetTimeUntilTransmissionOfReservationWithNo() "
                                                   "delay within frame: " << delayInFrame.GetSeconds());
        // The result is then the starting time of the current frame plus the delay within the frame until the
        // specific slot, minus the current time stamp.
        ns3::Time delay;
        if (m_lastFrameStart > ns3::Simulator::Now()) {
            ns3::Time timeToCurrentFrame = m_lastFrameStart - ns3::Simulator::Now();
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext() << " TdmaSlotManager:GetTimeUntilTransmissionOfReservationWithNo() time to current frame: " << timeToCurrentFrame);
            delay = timeToCurrentFrame + m_lastFrameStart + delayInFrame - ns3::Simulator::Now() - m_slotDuration;
        }
        else {
            delay = m_lastFrameStart + delayInFrame - ns3::Simulator::Now();
        }
        // In case we are calculating the scheduled time for a packet in the next frame, we have to add one frame duration
        if (delay.IsNegative()) {
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:GetTimeUntilTransmissionOfReservationWithNo() "
                                                       "slot is in next frame, will add one frame duration to correct for that");
            delay += m_frameDuration;
        }
        return delay;
    }
    uint32_t TdmaSlotManager::GetSlotIndexForTimestamp (ns3::Time t) {
        NS_LOG_FUNCTION(t << m_lastFrameStart);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        ns3::Time baseTime;
        if (t.GetNanoSeconds() >= m_lastFrameStart.GetNanoSeconds()) {
            baseTime = t - m_lastFrameStart;
            NS_LOG_DEBUG ("TdmaSlotManager:GetSlotIndexForTimestamp() t is above m_lastFrameStart, baseTime = " << baseTime);
        }
        else {
            baseTime = t - m_start;
            NS_LOG_DEBUG ("TdmaSlotManager:GetSlotIndexForTimestamp() t is below m_lastFrameStart, baseTime = " << baseTime);
        }
        uint64_t withinFrame = (baseTime.GetNanoSeconds() % m_frameDuration.GetNanoSeconds());
        return (withinFrame / m_slotDuration.GetNanoSeconds());
    }
    uint64_t TdmaSlotManager::GetGlobalSlotIndexForTimestamp(ns3::Time t) {
        // Depending on whether the time stamp refers to the beginning of a slot, or to any time
        // during the start and the end
        if (t.GetNanoSeconds() % m_slotDuration.GetNanoSeconds() == 0) {
            return (t.GetNanoSeconds() / m_slotDuration.GetNanoSeconds());
        }
        else {
            return floor(t.GetNanoSeconds() / m_slotDuration.GetNanoSeconds());
        }
    }
    uint32_t TdmaSlotManager::CalculateSlotOffsetBetweenTransmissions(uint32_t k, uint32_t l) {
        NS_LOG_FUNCTION(this << k << l);
        if (k == l) return 0;
        NS_ASSERT(m_selections.find(k) != m_selections.end());
        NS_ASSERT(m_selections.find(l) != m_selections.end());
        ns3::Ptr<TdmaSlot> first = m_selections[k];
        ns3::Ptr<TdmaSlot> second = m_selections[l];
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:CalculateSlotOffsetBetweenTransmissions() first  = "
                                           << first->GetSlotIndex());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:CalculateSlotOffsetBetweenTransmissions() second = "
                                           << second->GetSlotIndex());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:CalculateSlotOffsetBetweenTransmissions() numSlots = "
                                           << m_numSlots);
        if (second->GetSlotIndex() > first->GetSlotIndex()) {
            return second->GetSlotIndex() - first->GetSlotIndex();
        }
        else {
            uint32_t remaining = (m_numSlots - first->GetSlotIndex());
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:CalculateSlotOffsetBetweenTransmissions() remaining = "
                                               << remaining);
            return (remaining + second->GetSlotIndex());
        }
    }
    bool TdmaSlotManager::NeedsReReservation (uint32_t n) {
        NS_ASSERT(m_selections.find(n) != m_selections.end());
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        // If the internal timeout of this slot is zero (or less), we need to perform a re-reservation
        if (m_selections[n]->GetInternalTimeout() <= 0) {
            return true;
        }
        else {
            return false;
        }
    }
    uint8_t TdmaSlotManager::DecreaseTimeOutOfReservationWithNumber(uint32_t n) {
        NS_LOG_FUNCTION (n);
        NS_ASSERT(m_selections.find(n) != m_selections.end());
        NS_LOG_DEBUG("isAllocated: " << m_selections[n]->IsAllocated());
        NS_LOG_DEBUG("isInternallyAllocated: " << m_selections[n]->IsInternallyAllocated());
        NS_LOG_DEBUG("isBusy: " << m_selections[n]->IsBusy());
        NS_LOG_DEBUG("isFree: " << m_selections[n]->IsFree());
        NS_LOG_DEBUG("internal timeout: " << (uint32_t) m_selections[n]->GetInternalTimeout());
        NS_LOG_DEBUG("external timeout: " << (uint32_t) m_selections[n]->GetTimeout());
        NS_LOG_DEBUG("id: " << (uint32_t) m_selections[n]->GetSlotIndex());
        NS_ASSERT(m_selections[n]->IsInternallyAllocated());
        NS_ASSERT(m_selections[n]->GetInternalTimeout() > 0);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        // Update the timeout
        uint8_t newValue = m_selections[n]->GetInternalTimeout() - 1;
        m_slots[m_selections[n]->GetSlotIndex()]->SetInternalTimeout(newValue);
        m_selections[n]->SetInternalTimeout(newValue);
        return newValue;
    }
    void TdmaSlotManager::MarkSlotAsAllocated(uint32_t index, uint8_t timeout, ns3::Mac48Address node, ns3::Vector position) {
        MarkSlotAsAllocated(index, timeout, node, position, ns3::Seconds(0));
    }
    void TdmaSlotManager::MarkSlotAsAllocated(uint32_t index, uint8_t timeout, ns3::Mac48Address node, ns3::Vector position,
                                              ns3::Time notbefore) {
        NS_LOG_FUNCTION(index << (uint32_t) timeout << position);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        NS_ASSERT(m_slots.find(index) != m_slots.end());
        m_slots[index]->MarkAsAllocated(timeout, node, position, notbefore);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:MarkSlotAsAllocated() marked slot " << index
                                           << " as allocated (notBefore = " << notbefore.GetSeconds()
                                           << "), has current timeout value of " << (uint32_t) m_slots[index]->GetTimeout());
    }
    void TdmaSlotManager::UpdateSlotObservations() {
        NS_LOG_FUNCTION_NOARGS();
        m_lastFrameStart += m_frameDuration;
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:UpdateSlotObservations() called at node "
                                           << ns3::Simulator::GetContext() << " at " << ns3::Simulator::Now().GetSeconds());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:UpdateSlotObservations() m_lastFrameStart is now: "
                                           << m_lastFrameStart);
        // Iterate over all slots and update their status
        for (uint32_t i = 0; i < m_slots.size(); i++) {
            // If this slot is marked as internally allocated, but has an internal timeout value equal or less than zero,
            // we can free this slot
            if (m_slots[i]->IsInternallyAllocated() && m_slots[i]->GetInternalTimeout() <= 0) {
                // But before we release check whether we introduced a collision on this slot, because if we need
                // to remove the initial owner from our memory
                if (m_slots[i]->IsAllocated()) {
                    ns3::Mac48Address owner = m_slots[i]->GetOwner();
                    NS_ASSERT(m_collisions.find(owner) != m_collisions.end());
                    m_collisions.erase(owner);
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:UpdateSlotObservations() --> slot "
                                                       << i << " had been shared with node at " << owner
                                                       << ", hence we can forget about it now.");
                }
                m_slots[i]->MarkAsFree();
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:UpdateSlotObservations() --> marking slot "
                                                   << i << " as FREE again (was internally allocated before)");
            }
            // If this slot had been allocated by an external station, either decrease the timeout value of
            // this slot or mark it as free again
            if (m_slots[i]->IsAllocated() && !m_slots[i]->IsInternallyAllocated()) {
                if (m_slots[i]->GetTimeout() > 0) {
                    uint8_t newValue = m_slots[i]->GetTimeout() - 1;
                    m_slots[i]->SetTimeout(newValue);
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:UpdateSlotObservations() --> reducing "
                                                               "timeout of slot " << i << " to " << (uint32_t) newValue);
                }
                else {
                    m_slots[i]->MarkAsFree();
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                       << " TdmaSlotManager:UpdateSlotObservations() --> marking slot "
                                                       << i << " as FREE again (was externally allocated before)");
                }
            }
            // Handle the slots that are busy correctly and count their timeout down properly
            // If the timeout is zero or less, then we can mark them as free again (because we will set
            // a timeout value of 1 every time the slot is detected CCS busy)
            if (m_slots[i]->IsBusy() && !m_slots[i]->IsInternallyAllocated()) {
                if (m_slots[i]->GetTimeout() > 0) {
                    uint8_t newValue = m_slots[i]->GetTimeout() - 1;
                    m_slots[i]->SetTimeout(newValue);
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext() << " TdmaSlotManager:UpdateSlotObservations() --> reducing timeout of BUSY slot " << i << " to " << (uint32_t) newValue);
                }
                else {
                    m_slots[i]->MarkAsFree();
                    NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext() << " TdmaSlotManager:UpdateSlotObservations() --> marking slot " << i << " as FREE again (was busy before)");
                }
            }
        }
    }
    void TdmaSlotManager::MarkSlotAsFreeAgain(uint32_t index) {
        NS_LOG_FUNCTION(this << index);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        NS_ASSERT(m_slots.find(index) != m_slots.end());
        m_slots[index]->MarkAsFree();
    }
    void TdmaSlotManager::MarkSlotAsBusy(uint32_t index) {
        NS_LOG_FUNCTION(this << index);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        NS_ASSERT(m_slots.find(index) != m_slots.end());
        m_slots[index]->MarkAsBusy();
    }
    void TdmaSlotManager::RebaseFrameStart(ns3::Time now) {
        NS_LOG_FUNCTION (this << now);
        // Re-basing is only allowed prior to performing anz reservation
        NS_ASSERT (m_selections.size() == 0);
        // We allow re-basing only to multiples of the slot duration
        ns3::Time baseTime = now - m_lastFrameStart;
        NS_ASSERT(baseTime.GetNanoSeconds() % m_slotDuration.GetNanoSeconds() == 0);
        // Step 1: Calculate the number of slots that have passed since last frame start
        //         That means: all slot indices have to be reduced by this number
        uint32_t offset = baseTime.GetNanoSeconds() / m_slotDuration.GetNanoSeconds();
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:RebaseFrameStart() applying offset of "
                                           << (offset) << " to all " << m_slots.size() << " slots.");
        // Step 2: Adjust the slots IDs of all slot objects accordingly
        std::vector<ns3::Ptr<TdmaSlot> > newOrder;
        for (uint32_t i = 0; i < m_slots.size(); i++) {
            ns3::Ptr<TdmaSlot> slot = m_slots[i];
            uint32_t newIndex;
            if (slot->GetSlotIndex() < offset) {
                newIndex = m_numSlots - offset + slot->GetSlotIndex();
            }
            else {
                newIndex = slot->GetSlotIndex() - offset;
            }
            slot->RebaseIndex(newIndex);
            newOrder.push_back(slot);
        }
        m_slots.clear();
        // Re-insert the slots into the m_slots map
        for (uint32_t i = 0; i < newOrder.size(); i++) {
            ns3::Ptr<TdmaSlot> slot = newOrder.at(i);
            m_slots[slot->GetSlotIndex()] = slot;
        }
        // Step 3: Save the new m_lastFrameStart time stamp
        m_lastFrameStart = now;
    }
    ns3::Ptr<RandomAccessDetails> TdmaSlotManager::GetNetworkEntryTimestamp(uint32_t remainingSlots, double p) {
        NS_LOG_FUNCTION(this << remainingSlots << p);
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() current global slot id = "
                                           << GetGlobalSlotIndexForTimestamp(ns3::Simulator::Now()));

        ns3::Ptr<ns3::UniformRandomVariable> randomizer = ns3::CreateObject<ns3::UniformRandomVariable> ();;
        ns3::Ptr<RandomAccessDetails> details = ns3::Create<RandomAccessDetails>();

        double randomValue = randomizer->GetValue(0.0, 1.0);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() randomValue = " << randomValue);
        std::vector<uint32_t> candidates;
        uint32_t start = GetSlotIndexForTimestamp(ns3::Simulator::Now());
        uint32_t end = start + remainingSlots;
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() start = " << start);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() end = " << end);
        for (uint32_t i = start; i < end; i++) {
            NS_ASSERT(m_slots.find(i) != m_slots.end());
            NS_ASSERT(m_slots[i]->GetSlotIndex() == i);
            ns3::Time until = ns3::Simulator::Now() + ns3::Seconds((i-start+1) * m_slotDuration.GetSeconds());
            if (m_slots[i]->IsFree(until)) {
                candidates.push_back(i);
            }
        }
        // If up to now no free slot is available we will reuse an allocated slot
        if (candidates.size() < 1) {
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << "TdmaSlotManager:GetNetworkEntryTimestamp() has to add one already allocated slot");
            std::map<double, uint32_t> usedSlots;
            for (uint32_t i = start; i < end; i++) {
                if (m_slots[i]->IsAllocated())
                {
                    ns3::Vector owner = m_slots[i]->GetPosition();
                    ns3::Vector myself =
                            ns3::NodeList::GetNode(ns3::Simulator::GetContext())->GetObject<ns3::MobilityModel>()->GetPosition();
                    double distance = ns3::CalculateDistance(owner, myself);
                    while (usedSlots.find(distance) != usedSlots.end()) {
                        distance += 0.000001;
                    }
                    usedSlots[distance] = m_slots[i]->GetSlotIndex();
                }
            }
            std::map<double, uint32_t>::reverse_iterator it = usedSlots.rbegin();
            uint32_t index = it->second;
            candidates.push_back(index);
        }
        // Calculate the probability value that has to be met in order to transmit in the next slot
        uint32_t n = candidates.size();
        uint32_t no = 0;
        double prob = p; // 1.0 / n;
        double incr = 0;
        if (p <= 1.0) {
            incr = (1.0-p) / n;
        }
        prob += incr;
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() no = " << no
                                           << ", prob = " << prob << ", randomValue = " << randomValue);
        // As long as the randomValue is greater than 'prob', keep moving forward to the next
        // candidate slot
        while (randomValue > prob && no < (n-1)) {
            randomValue = randomizer->GetValue();
            no++;
            prob += incr;
            NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                               << " TdmaSlotManager:GetNetworkEntryTimestamp() no = " << no
                                               << ", prob = " << prob << ", randomValue = " << randomValue);
            if (randomValue <= prob) {
                NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                                   << " TdmaSlotManager:GetNetworkEntryTimestamp() stopping!");
            }
        }
        // Okay, we identified the candidate slot in which we will perform network entry
        uint32_t slotIndex = candidates.at(no);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() slotIndex = " << slotIndex);
        uint32_t slotoffset = (slotIndex - start);
        ns3::Time delay = ns3::NanoSeconds(slotoffset * m_slotDuration.GetNanoSeconds());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() offset to selected slot is "
                                           << slotoffset);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() delay until selected slot is "
                                           << delay.GetSeconds());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() timestamp is "
                                           << (ns3::Simulator::Now() + delay));
        // Set the information in the result object
        details->SetProbability(prob);
        details->SetRemainingSlots(remainingSlots - (slotIndex-start));
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:GetNetworkEntryTimestamp() remaining slots = "
                                           << details->GetRemainingSlots());
        details->SetWhen(ns3::Simulator::Now() + delay);
        return details;
    }

    bool TdmaSlotManager::HasFreeSlotsLeft(uint32_t remainingSlots) {
        uint32_t start = GetSlotIndexForTimestamp(ns3::Simulator::Now());
        uint32_t i = start;
        uint32_t end = start + remainingSlots;
        while (i < end) {
            ns3::Time until = ns3::Simulator::Now() + ns3::Seconds((i-start+1) * m_slotDuration.GetSeconds());
            if (m_slots[i]->IsFree(until)) {
                return true;
            }
            i++;
        }
        return false;
    }

    bool TdmaSlotManager::IsCurrentSlotStillFree()
    {
        NS_LOG_FUNCTION_NOARGS();
        // Update the slot reservation / observation / allocation status at the beginning
        // of each new frame
        if (ns3::Simulator::Now() >= m_lastFrameStart + m_frameDuration) {
            UpdateSlotObservations();
        }
        uint32_t index = GetSlotIndexForTimestamp(ns3::Simulator::Now());
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:IsCurrentSlotStillFree() m_start = " << m_start);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:IsCurrentSlotStillFree() m_lastFrameStart = "
                                           << m_lastFrameStart);
        NS_LOG_DEBUG(ns3::Simulator::Now() << " " << ns3::Simulator::GetContext()
                                           << " TdmaSlotManager:IsCurrentSlotStillFree() index = " << index);
        NS_ASSERT(index >= 0);
        NS_ASSERT(index < m_slots.size());
        return m_slots[index]->IsFree(ns3::Simulator::Now() + m_slotDuration);
    }
    ns3::Ptr<TdmaSlot> TdmaSlotManager::GetSlot(uint32_t index) {
        NS_ASSERT(m_slots.find(index) != m_slots.end());
        return m_slots[index];
    }
}
