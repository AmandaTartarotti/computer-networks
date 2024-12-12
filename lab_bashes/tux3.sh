ifconfig $1 172.16.20.1/24

route add -net 172.16.21.0/24 gw 172.16.20.254

route add -net 172.16.1.0/24 gw 172.16.20.254