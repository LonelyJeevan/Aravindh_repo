// For kernel module
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>

// For Network Device Driver
#include <linux/netdevice.h>	// For struct net_device 
#include <linux/skbuff.h>	// For struct sk_buff
#include <linux/etherdevice.h>	// For eth_type_trans()

// For accessing protocol headers
#include <linux/if_ether.h>	// For struct ethhdr and ETH_P_IP
#include <linux/ip.h>		// For struct iphdr 

MODULE_LICENSE("GPL");

// Driver private data structure
struct net_dev_priv
{
    struct net_device *dev; // Save a pointer to corresponding struct net_device
    struct sk_buff *skb;	// Save pointer to current sk_buff under process
};

// Create two instances of Virtual Network Device (VND) 
struct net_device *drv_net_devs[2];

// Start network interface and start sending network packets.
static int drv_net_open(struct net_device *dev)
{
    printk(KERN_INFO "drv_net_open() called\n");

    /*
     *Request and assign mem region, I/O ports, irq etc in actual device driver.
     * Call request_region( ), request_irq( ) etc.
     *
     * Assign the hardware address of the board: use "\0CAPGx", where
     * x is 0 or 1. The first byte is ’\0’ to avoid being a multicast
     * MAC address (the first byte of multicast MAC address is odd).
     */

    // Fill up the MAC address of the first virtual network device: "\0CAPG0".
    memcpy(dev->dev_addr, "\0CAPG0", ETH_ALEN);	// MAC address: "\0CAPG0"

    // Fill up the MAC address of the second virtual network device: "\0CAPG1".
    if (dev == drv_net_devs[1] )
        dev->dev_addr[ETH_ALEN-1]++; // MAC address: "\0CAPG1"

    // Tell the network stack to start the packet transmission.
    netif_start_queue(dev);

    return 0;
}

// Stop network interface and stop sending network packets.
static int drv_net_stop(struct net_device *dev)
{
    printk(KERN_INFO "drv_net_stop() called\n");

    // Release Mem Region, I/O ports, irq and such in actual device driver.

    // Tell the network stack to stop the packet transmission.
    netif_stop_queue(dev);

    return 0;
}

// Receive incoming network packets and inject these packets into network stack.
static int drv_net_rx(struct net_device *dev, char *data, int len)
{
    struct sk_buff *skb;
    struct net_dev_priv *nd_priv = netdev_priv(dev);

    printk(KERN_INFO "drv_net_rx() called\n");

    /*
     * In actual device drivers, the network packets are retrieved from the
     * DMA buffer at this point in code.
     * Build an skb around it, so upper layers can handle it. Add 2 bytes 
     * of buffer space for IP header alignment.
     */
    skb = dev_alloc_skb(len + 2);
    if (!skb)
    {
        printk(KERN_NOTICE "drv_net_rx(): No memory - packet dropped\n");
        goto err_alloc_skb;
    }

    // Copy the packet data to newly allocated SKB.
    memcpy(skb_put(skb, len), data, len);

    // Write some metadata into SKB.
    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY; // Don’t check checksum.

    // Pass the network packet (SKB) to the network stack. 
    netif_rx(skb);

    return 0;

err_alloc_skb:
    return -1;
}

/*
 * Transmit outgoing network packets - handle actual device HW dependent code.
 * This function deals with hw details in real device driver.
 * In this case, this virtual network device/interface loops back
 * the packet to the other virtual network device/interface (if any).
 * In other words, this function implements the virtual network device
 * behaviour, while all other fucntions are rather device-independent.
 */
static void drv_net_hw_tx(char *data, int len, struct net_device *txdev)
{
    struct iphdr *ih;
    struct net_device *rxdev;
    struct net_dev_priv *nd_priv;
    unsigned int *saddr, *daddr;

    printk(KERN_INFO "drv_net_hw_tx() called\n");

    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) 
    {
        printk("drv_net: packet too short (%d bytes)\n", len);
        return;
    }

    // Enable this conditional "if" to print the packet data.
    if (0) 
    {
        int i;
        printk(KERN_DEBUG "len is %d\n" KERN_DEBUG "data:",len);
        for (i=14 ; i<len; i++)
            printk(" %02x",data[i]&0xff);
        printk("\n");
    }
    /*
     * Ethernet/MAC header is 14 bytes, but the network stack arranges for
     * IP header to be aligned at 4-byte boundary (i.e. Ethernet/MAC 
     * header is unaligned).
     */
    ih = (struct iphdr *)(data+sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;

    /*
     * Manipulate the IP header to pretend that the packet is coming from
     * another host from another network. 
     * Change the third byte of class C source and destination IP addresses,
     * so that the network address part of the IP address gets changed.
     */
    ((unsigned char *)saddr)[2] ^= 1;
    ((unsigned char *)daddr)[2] ^= 1;

    // Rebuild the checksum. IP needs it.
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

    // Now the packet is ready for transmission.
    rxdev = drv_net_devs[txdev == drv_net_devs[0] ? 1 : 0];
    nd_priv = netdev_priv(rxdev);

/*
 * This is the point where the network packet is changing the direction
 * from TX path to RX path and from/to vnd0 N/W iface to/from vnd1 N/W iface.
 *             TX ===============> RX
 *            vnd0 <============> vnd1
 * drv_net_devs[0] <============> drv_net_devs[1] 
 */

    /*
     * At this point, incoming packet is supposed to have been received.
     * Send the incoming packet to drv_net_rx() for handling.
     */
    drv_net_rx(rxdev, data, len);

    // At this point,the incoming packet has been sent to the network stack.

    /*
     * At this point, outgoing packet is supposed to have been
     * transmitted.
     */
    nd_priv = netdev_priv(txdev);

    // Free up SKB.
    if (nd_priv->skb)
    {
        dev_kfree_skb(nd_priv->skb);
        nd_priv->skb = NULL;
    }
}

// Transmit outgoing network packets.
static netdev_tx_t drv_net_tx(struct sk_buff *skb, struct net_device *dev)
{
    int len;
    char *data, shortpkt[ETH_ZLEN];
    struct net_dev_priv *nd_priv = netdev_priv(dev);

    printk(KERN_INFO "drv_net_tx() called\n");

    /*
     * If packet is shorter than the minimum length, then copy the data into
     * a short packet buffer before processing.
     */
    data = skb->data;
    len = skb->len;
    if (len < ETH_ZLEN)
    {
        memset(shortpkt, 0, ETH_ZLEN);
        memcpy(shortpkt, skb->data, skb->len);
        len = ETH_ZLEN;
        data = shortpkt;
    }

    // Save the timestamp of packet transmission.
    dev->_tx->trans_start = jiffies;

    // Remember the skb, so we can free it after transmitting the packet.
    nd_priv->skb = skb;

    /* 
     * Actual transmission of data is device-specific.
     * Perform device-specific transmission in the called fucntion below. 
     */
    drv_net_hw_tx(data, len, dev);

    return NETDEV_TX_OK;
}

// Function to allow device driver (instead of ARP) to create MAC header.
static int drv_net_header(struct sk_buff *skb, struct net_device *dev,
                          unsigned short type, const void *daddr,
                          const void *saddr,unsigned int len)
{
    struct ethhdr *eth = (struct ethhdr *)skb_push(skb,ETH_HLEN);

    printk(KERN_INFO "drv_net_header() called\n");

    eth->h_proto = htons(type);
    memcpy(eth->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
    memcpy(eth->h_dest, daddr ? daddr : dev->dev_addr, dev->addr_len);

    // Flip the LSBit of destination MAc address (of the other interface). 
    eth->h_dest[ETH_ALEN-1] ^= 0x01;

    return (dev->hard_header_len);
}

static struct net_device_ops nd_ops =
{
    .ndo_open = drv_net_open,
    .ndo_stop = drv_net_stop,
    .ndo_start_xmit = drv_net_tx
};

static struct header_ops hdr_ops =
{
    .create = drv_net_header    
}; 

// Network device setup/initialization function.
static void drv_net_setup(struct net_device *dev)
{
    struct net_dev_priv *nd_priv = (struct net_dev_priv *)netdev_priv(dev);

    printk(KERN_INFO "drv_net_setup() called\n");

        ether_setup(dev);
        dev->netdev_ops = &nd_ops; 
        dev->header_ops = &hdr_ops; 

        /*
         * keep the default flags, just add NOARP as we can not use ARP
         * with this virtual device driver. 
         */
        dev->flags |= IFF_NOARP;
        // Pretend that checksum is performed in the hardware.
        //dev->features |= NETIF_F_HW_CSUM; // Not required

        // Initialize nd_priv with zeros.
        memset(nd_priv, 0, sizeof(struct net_dev_priv));

        //Save pointer to struct net_device in corresponding struct net_dev_priv
        nd_priv->dev = dev;

    return;
}

static int __init drv_net_init(void)
{
    int i;
    int result;
    struct net_device *dev;
    struct net_dev_priv *nd_priv;

    printk(KERN_INFO "Hello Kernel\n");

    /* 
     * alloc_netdev() allocates memory for struct net_dev_priv
     * along with struct net_device.
     */
    dev = alloc_netdev(sizeof(struct net_dev_priv), "vnd%d", NET_NAME_UNKNOWN,
                       drv_net_setup);
    if (dev == NULL)
    {
        printk(KERN_ERR "Failed to allocate netdev for dev 0\n");
        return -1;
    }
    drv_net_devs[0] = dev;

    dev = alloc_netdev(sizeof(struct net_dev_priv), "vnd%d", NET_NAME_UNKNOWN,
                       drv_net_setup);
    if (dev == NULL)
    {
        printk(KERN_ERR "Failed to allocate netdev for dev 1\n");
        free_netdev(drv_net_devs[0]);
        return -1;
    }
    drv_net_devs[1] = dev;

    for (i = 0; i < 2; i++)
    {
        dev = drv_net_devs[i];
        // Register the network device.
        if ((result = register_netdev(dev)))
        {
            printk("drv_net: error %d registering device %s",result,
                   drv_net_devs[i]->name);
            if (i == 1)
            {
                unregister_netdev(drv_net_devs[0]);
            }
            goto err_reg_netdev;
        }
        
    }

    return 0;
 
err_reg_netdev:
    for (i = 0; i < 2; i++)
    {
        free_netdev(drv_net_devs[i]);
    }

    return -1;
}

static void __exit drv_net_exit(void)
{
    int i;
    struct net_device *dev;

    for (i = 0; i < 2; i++)
    {
        dev = drv_net_devs[i];
        if (dev)
        {
            unregister_netdev(dev);
            free_netdev(dev);
        }
    }

    printk(KERN_INFO "Bye-bye Kernel\n");
    return ;
}
 
module_init(drv_net_init);
module_exit(drv_net_exit);
