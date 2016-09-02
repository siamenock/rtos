/*
 *
 *void init_nics(int count) {
 *        int index = 0;
 *        extern uint64_t manager_mac;
 *        for(int i = 0; i < count; i++) {
 *                nic_devices[i] = device_get(DEVICE_TYPE_NIC, i);
 *                if(!nic_devices[i])
 *                        continue;
 *
 *                NICPriv* nic_priv = gmalloc(sizeof(NICPriv));
 *                if(!nic_priv)
 *                        continue;
 *
 *                nic_priv->nics = map_create(8, NULL, NULL, NULL);
 *
 *                nic_devices[i]->priv = nic_priv;
 *
 *                NICInfo info;
 *                ((NICDriver*)nic_devices[i]->driver)->get_info(nic_devices[i]->id, &info);
 *
 *                nic_priv->port_count = info.port_count;
 *                for(int j = 0; j < info.port_count; j++) {
 *                        nic_priv->mac[j] = info.mac[j];
 *
 *                        char name_buf[64];
 *                        sprintf(name_buf, "eth%d", index);
 *                        uint16_t port = j << 12;
 *
 *                        Map* vnics = map_create(16, NULL, NULL, NULL);
 *                        map_put(nic_priv->nics, (void*)(uint64_t)port, vnics);
 *
 *                        printf("\t%s : [%02x:%02x:%02x:%02x:%02x:%02x] [%c]\n", name_buf,
 *                                        (info.mac[j] >> 40) & 0xff,
 *                                        (info.mac[j] >> 32) & 0xff,
 *                                        (info.mac[j] >> 24) & 0xff,
 *                                        (info.mac[j] >> 16) & 0xff,
 *                                        (info.mac[j] >> 8) & 0xff,
 *                                        (info.mac[j] >> 0) & 0xff,
 *                                        manager_mac == 0 ? '*' : ' ');
 *
 *                        if(!manager_mac)
 *                                manager_mac = info.mac[j];
 *
 *                        index++;
 *                }
 *        }
 *}
 *
 */
