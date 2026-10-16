/// \file
/// Defines recipient policies for messenger approach and delivery responses.
/// These values are separate from projectile flight states and launch modes.

#ifndef MESSENGER_CONTACT_H
#define MESSENGER_CONTACT_H

/// Known recipient contact policies retained in a full signed 32-bit word.
/// Setters accept arbitrary words; other values use direct approach and ordinary
/// delivery/removal for supported recipients. The enum imposes no validation.
/// Orbit policies affect steering; contact uses the unoffset target distance.
typedef enum MessengerContactPolicy {
    MESSENGER_CONTACT_DIRECT = 0,       ///< Approach directly and use ordinary delivery/removal.
    MESSENGER_CONTACT_REBOUND = 1,      ///< Set only the message type to zero before posting, then rebound.
    MESSENGER_CONTACT_ACTION_ORBIT = 2, ///< Orbit when the effective action attributes enable it.
    MESSENGER_CONTACT_PREFER_ORBIT = 3  ///< Prefer orbit unless suppressed; the enabling attribute takes precedence.
} MessengerContactPolicy;

#endif
