#pragma once

#include <cstdint>
#include <utility>

#include <pcap.h>

namespace ntmd {

/* Performance based list of IP interface addresses from a device.
 * Since in most cases a network device will have multiple ip addresses but one primary one we will
 * attempt to put that address as the first element in the list for the best performance when
 * calling IPList::contains.
 */
class IPList
{
  public:
    IPList() = default;
    ~IPList() = default;

    /* Should only be called once. */
    void init(const pcap_if* device);

    /* Searchs the list to determine if param ip is contained in it. */
    bool contains(uint32_t ip) const;

  private:
    /* For performance reasons this will be stack allocated with a set amount.
     * This is an arbitrary amount for the expected max of local ip interfaces a machine will have.
     */
    uint32_t mIPs[64]{};
    int mSize{0};
};

} // namespace ntmd