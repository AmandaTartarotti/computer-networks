ifconfig $1 172.16.21.1/24

#route to network 21
route add -net 172.16.20.0/24 gw 172.16.21.253

route add -net 172.16.1.0/24 gw 172.16.21.254

#step 4.1
# sysctl net.ipv4.conf.eth1.accept_redirects=0
# sysctl net.ipv4.conf.all.accept_redirects=0

# step 4.2
# route del -net 172.16.20.0/24 gw 172.16.21.253
# route add -net 172.16.20.0/24 gw 172.16.21.254

#step 4.6
# route del -net 172.16.20.0/24 gw 172.16.21.254
# route add -net 172.16.20.0/24 gw 172.16.21.253

#Activate the acceptance of ICMP redirect at tuxY2 when there is no route to 172.16.Y0.0/24 via tuxY4 and try to understand what happens (Como?)