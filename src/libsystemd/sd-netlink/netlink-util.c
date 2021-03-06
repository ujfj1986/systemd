/* SPDX-License-Identifier: LGPL-2.1+ */

#include "sd-netlink.h"

#include "memory-util.h"
#include "netlink-internal.h"
#include "netlink-util.h"
#include "strv.h"

int rtnl_set_link_name(sd_netlink **rtnl, int ifindex, const char *name) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *message = NULL;
        int r;

        assert(rtnl);
        assert(ifindex > 0);
        assert(name);

        if (!ifname_valid(name))
                return -EINVAL;

        if (!*rtnl) {
                r = sd_netlink_open(rtnl);
                if (r < 0)
                        return r;
        }

        r = sd_rtnl_message_new_link(*rtnl, &message, RTM_SETLINK, ifindex);
        if (r < 0)
                return r;

        r = sd_netlink_message_append_string(message, IFLA_IFNAME, name);
        if (r < 0)
                return r;

        r = sd_netlink_call(*rtnl, message, 0, NULL);
        if (r < 0)
                return r;

        return 0;
}

int rtnl_set_link_properties(sd_netlink **rtnl, int ifindex, const char *alias,
                             const struct ether_addr *mac, uint32_t mtu) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *message = NULL;
        int r;

        assert(rtnl);
        assert(ifindex > 0);

        if (!alias && !mac && mtu == 0)
                return 0;

        if (!*rtnl) {
                r = sd_netlink_open(rtnl);
                if (r < 0)
                        return r;
        }

        r = sd_rtnl_message_new_link(*rtnl, &message, RTM_SETLINK, ifindex);
        if (r < 0)
                return r;

        if (alias) {
                r = sd_netlink_message_append_string(message, IFLA_IFALIAS, alias);
                if (r < 0)
                        return r;
        }

        if (mac) {
                r = sd_netlink_message_append_ether_addr(message, IFLA_ADDRESS, mac);
                if (r < 0)
                        return r;
        }

        if (mtu != 0) {
                r = sd_netlink_message_append_u32(message, IFLA_MTU, mtu);
                if (r < 0)
                        return r;
        }

        r = sd_netlink_call(*rtnl, message, 0, NULL);
        if (r < 0)
                return r;

        return 0;
}

int rtnl_set_link_alternative_names(sd_netlink **rtnl, int ifindex, char * const *alternative_names) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *message = NULL;
        int r;

        assert(rtnl);
        assert(ifindex > 0);

        if (strv_isempty(alternative_names))
                return 0;

        if (!*rtnl) {
                r = sd_netlink_open(rtnl);
                if (r < 0)
                        return r;
        }

        r = sd_rtnl_message_new_link(*rtnl, &message, RTM_NEWLINKPROP, ifindex);
        if (r < 0)
                return r;

        r = sd_netlink_message_open_container(message, IFLA_PROP_LIST);
        if (r < 0)
                return r;

        r = sd_netlink_message_append_strv(message, IFLA_ALT_IFNAME, alternative_names);
        if (r < 0)
                return r;

        r = sd_netlink_message_close_container(message);
        if (r < 0)
                return r;

        r = sd_netlink_call(*rtnl, message, 0, NULL);
        if (r < 0)
                return r;

        return 0;
}

int rtnl_set_link_alternative_names_by_ifname(sd_netlink **rtnl, const char *ifname, char * const *alternative_names) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *message = NULL;
        int r;

        assert(rtnl);
        assert(ifname);

        if (strv_isempty(alternative_names))
                return 0;

        if (!*rtnl) {
                r = sd_netlink_open(rtnl);
                if (r < 0)
                        return r;
        }

        r = sd_rtnl_message_new_link(*rtnl, &message, RTM_NEWLINKPROP, 0);
        if (r < 0)
                return r;

        r = sd_netlink_message_append_string(message, IFLA_IFNAME, ifname);
        if (r < 0)
                return r;

        r = sd_netlink_message_open_container(message, IFLA_PROP_LIST);
        if (r < 0)
                return r;

        r = sd_netlink_message_append_strv(message, IFLA_ALT_IFNAME, alternative_names);
        if (r < 0)
                return r;

        r = sd_netlink_message_close_container(message);
        if (r < 0)
                return r;

        r = sd_netlink_call(*rtnl, message, 0, NULL);
        if (r < 0)
                return r;

        return 0;
}

int rtnl_resolve_link_alternative_name(sd_netlink **rtnl, const char *name) {
        _cleanup_(sd_netlink_unrefp) sd_netlink *our_rtnl = NULL;
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *message = NULL, *reply = NULL;
        int r, ret;

        assert(name);

        if (!rtnl)
                rtnl = &our_rtnl;
        if (!*rtnl) {
                r = sd_netlink_open(rtnl);
                if (r < 0)
                        return r;
        }

        r = sd_rtnl_message_new_link(*rtnl, &message, RTM_GETLINK, 0);
        if (r < 0)
                return r;

        r = sd_netlink_message_append_string(message, IFLA_ALT_IFNAME, name);
        if (r < 0)
                return r;

        r = sd_netlink_call(*rtnl, message, 0, &reply);
        if (r == -EINVAL)
                return -ENODEV; /* The device doesn't exist */
        if (r < 0)
                return r;

        r = sd_rtnl_message_link_get_ifindex(reply, &ret);
        if (r < 0)
                return r;
        assert(ret > 0);
        return ret;
}

int rtnl_get_link_iftype(sd_netlink **rtnl, int ifindex, unsigned short *ret) {
        _cleanup_(sd_netlink_message_unrefp) sd_netlink_message *message = NULL, *reply = NULL;
        int r;

        if (!*rtnl) {
                r = sd_netlink_open(rtnl);
                if (r < 0)
                        return r;
        }

        r = sd_rtnl_message_new_link(*rtnl, &message, RTM_GETLINK, ifindex);
        if (r < 0)
                return r;

        r = sd_netlink_call(*rtnl, message, 0, &reply);
        if (r == -EINVAL)
                return -ENODEV; /* The device does not exist */
        if (r < 0)
                return r;

        return sd_rtnl_message_link_get_type(reply, ret);
}

int rtnl_message_new_synthetic_error(sd_netlink *rtnl, int error, uint32_t serial, sd_netlink_message **ret) {
        struct nlmsgerr *err;
        int r;

        assert(error <= 0);

        r = message_new(rtnl, ret, NLMSG_ERROR);
        if (r < 0)
                return r;

        (*ret)->hdr->nlmsg_seq = serial;

        err = NLMSG_DATA((*ret)->hdr);

        err->error = error;

        return 0;
}

int rtnl_log_parse_error(int r) {
        return log_error_errno(r, "Failed to parse netlink message: %m");
}

int rtnl_log_create_error(int r) {
        return log_error_errno(r, "Failed to create netlink message: %m");
}

void rtattr_append_attribute_internal(struct rtattr *rta, unsigned short type, const void *data, size_t data_length) {
        size_t padding_length;
        uint8_t *padding;

        assert(rta);
        assert(!data || data_length > 0);

        /* fill in the attribute */
        rta->rta_type = type;
        rta->rta_len = RTA_LENGTH(data_length);
        if (data)
                /* we don't deal with the case where the user lies about the type
                 * and gives us too little data (so don't do that)
                 */
                padding = mempcpy(RTA_DATA(rta), data, data_length);

        else
                /* if no data was passed, make sure we still initialize the padding
                   note that we can have data_length > 0 (used by some containers) */
                padding = RTA_DATA(rta);

        /* make sure also the padding at the end of the message is initialized */
        padding_length = (uint8_t *) rta + RTA_SPACE(data_length) - padding;
        memzero(padding, padding_length);
}

int rtattr_append_attribute(struct rtattr **rta, unsigned short type, const void *data, size_t data_length) {
        struct rtattr *new_rta, *sub_rta;
        size_t message_length;

        assert(rta);
        assert(!data || data_length > 0);

        /* get the new message size (with padding at the end) */
        message_length = RTA_ALIGN(rta ? (*rta)->rta_len : 0) + RTA_SPACE(data_length);

        /* buffer should be smaller than both one page or 8K to be accepted by the kernel */
        if (message_length > MIN(page_size(), 8192UL))
                return -ENOBUFS;

        /* realloc to fit the new attribute */
        new_rta = realloc(*rta, message_length);
        if (!new_rta)
                return -ENOMEM;
        *rta = new_rta;

        /* get pointer to the attribute we are about to add */
        sub_rta = (struct rtattr *) ((uint8_t *) *rta + RTA_ALIGN((*rta)->rta_len));

        rtattr_append_attribute_internal(sub_rta, type, data, data_length);

        /* update rta_len */
        (*rta)->rta_len = message_length;

        return 0;
}
