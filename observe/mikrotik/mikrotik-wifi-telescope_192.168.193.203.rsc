# apr/02/2023 13:00:49 by RouterOS 7.8
# software id = 8N5J-2IU9
#
# model = RB941-2nD
# serial number = A1C30A1C6C93
/interface bridge
add admin-mac=74:4D:28:79:D6:53 auto-mac=no comment=defconf name=bridge
/interface ethernet
set [ find default-name=ether1 ] advertise=\
    10M-half,10M-full,100M-half,100M-full,1000M-half,1000M-full
set [ find default-name=ether2 ] advertise=\
    10M-half,10M-full,100M-half,100M-full,1000M-half,1000M-full
set [ find default-name=ether3 ] advertise=\
    10M-half,10M-full,100M-half,100M-full,1000M-half,1000M-full
set [ find default-name=ether4 ] advertise=\
    10M-half,10M-full,100M-half,100M-full,1000M-half,1000M-full
/interface wireless
set [ find default-name=wlan1 ] antenna-gain=0 band=2ghz-b/g/n channel-width=\
    20/40mhz-Ce country=no_country_set disabled=no distance=indoors \
    frequency=auto frequency-mode=manual-txpower mode=ap-bridge ssid=\
    telescope-service station-roaming=enabled wireless-protocol=802.11
/interface list
add comment=defconf name=WAN
add comment=defconf name=LAN
/interface wireless security-profiles
set [ find default=yes ] authentication-types=wpa2-psk mode=dynamic-keys \
    supplicant-identity=MikroTik wpa-pre-shared-key=serviceobserv2m \
    wpa2-pre-shared-key=serviceobserv2m
/routing ospf instance
add disabled=no name=default-v2
/routing ospf area
add disabled=yes instance=default-v2 name=backbone-v2
/interface bridge port
add bridge=bridge comment=defconf ingress-filtering=no interface=ether2
add bridge=bridge comment=defconf ingress-filtering=no interface=ether3
add bridge=bridge comment=defconf ingress-filtering=no interface=ether4
add bridge=bridge comment=defconf ingress-filtering=no interface=wlan1
add bridge=bridge ingress-filtering=no interface=ether1
/ip neighbor discovery-settings
set discover-interface-list=none
/ip settings
set max-neighbor-entries=8192
/ipv6 settings
set max-neighbor-entries=8192
/interface list member
add comment=defconf interface=bridge list=LAN
add comment=defconf interface=ether1 list=WAN
/interface ovpn-server server
set auth=sha1,md5
/ip address
add address=192.168.193.203/24 comment=defconf interface=bridge network=\
    192.168.193.0
/ip dns
set servers=192.168.192.122,192.168.192.111
/ip route
add disabled=no dst-address=0.0.0.0/0 gateway=192.168.193.1
/ip service
set telnet disabled=yes
set ftp disabled=yes
set www disabled=yes
set api disabled=yes
set winbox disabled=yes
set api-ssl disabled=yes
/ip ssh
set forwarding-enabled=remote strong-crypto=yes
/system clock
set time-zone-name=Europe/Prague
/system identity
set name=wifi-telescope
/system ntp client
set mode=broadcast
/system package update
set channel=long-term
/tool bandwidth-server
set enabled=no
/tool mac-server
set allowed-interface-list=none
/tool mac-server mac-winbox
set allowed-interface-list=none
