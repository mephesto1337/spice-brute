#include <spice-client.h>
#include <stdio.h>

#include "spice-channel-types.h"

const char *channel_event_to_string(SpiceChannelEvent event) {
    static char buffer[8];
    switch (event) {
    case SPICE_CHANNEL_NONE:
        return "none";
    case SPICE_CHANNEL_OPENED:
        return "opened";
    case SPICE_CHANNEL_SWITCHING:
        return "switching";
    case SPICE_CHANNEL_CLOSED:
        return "closed";
    case SPICE_CHANNEL_ERROR_CONNECT:
        return "error connect";
    case SPICE_CHANNEL_ERROR_TLS:
        return "error tls";
    case SPICE_CHANNEL_ERROR_LINK:
        return "error link";
    case SPICE_CHANNEL_ERROR_AUTH:
        return "error auth";
    case SPICE_CHANNEL_ERROR_IO:
        return "error io";
    default:
        snprintf(buffer, sizeof(buffer), "%d", (int)event);
        return buffer;
    }
}

const char *channel_to_string(const SpiceChannel *channel) {
    static char buffer[256];

    int channel_id = -1;
    int channel_type = -1;

    g_object_get(G_OBJECT(channel), "channel-id", &channel_id, "channel-type", &channel_type, NULL);

    snprintf(buffer, sizeof(buffer), "Channel { .id=%d, .type=%s }", channel_id,
             spice_channel_type_to_string(channel_type));
    return buffer;
}
