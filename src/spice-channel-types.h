#ifndef __SPICE_CHANNEL_TYPES_H__DEFINED__
#define __SPICE_CHANNEL_TYPES_H__DEFINED__

#include <spice-client.h>

const char *channel_type_to_string(int channel_type);
const char *channel_event_to_string(SpiceChannelEvent event);
const char *channel_to_string(const SpiceChannel *channel);

#endif // __SPICE_CHANNEL_TYPES_H__DEFINED__
