ifconfig $1 172.16.20.254/24

ifconfig $2 172.16.21.253/24

#ip foward
sysctl net.ipv4.ip_forward=1

#disable echo broadcast
sysctl net.ipv4.icmp_echo_ignore_broadcasts=0


route add -net 172.16.1.0/24 gw 172.16.21.254