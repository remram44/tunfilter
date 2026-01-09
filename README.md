# tun device as terminal filter

This project creates an IP connection through any terminal. For example, it can be used over serial consoles, cloud shells, IRC, or many other programs.

You will need to be able to create a tun device on each side, which usually means root access.

# Example using GNU screen

On the local system, create named pipes to connect GNU screen to tunfilter:
```
mkfifo i o
stdbuf -i 0 -o 0 python3 hex.py dec < i | ./tunfilter tunf | stdbuf -i 0 -o 0 python3 hex.py enc > o
```

Then start screen, log in to the remote system, and set it up:
```
stty -icanon -echo; stdbuf -i 0 -o 0 python3 hex.py dec | ./tunfilter tunf 2> /dev/null | stdbuf -i 0 -o 0 python3 hex.py enc
Ctrl+A :exec !!. sh -c 'cat o & cat > i'
```

You can then set up the tun devices and use your new IP connection.

Example, on remote system:
```
ip addr add 10.0.44.1/24 dev tunf
ip link set tunf up
```

Example, on local system:
```
ip addr add 10.0.44.2/24 dev tunf
ip link set tunf up
```
