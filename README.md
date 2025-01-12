# pfpb
Packet Filter Peer Blocker

Some may ask, what purpose does this software serve? Have not VPNs eliminated the need for peer blocking software? Is it not true most of them no longer work? The lists used by this software are taken from https://www.iblocklist.com/lists. If you view the different paid vs free lists, you will probably see why a paid subscription is much better at protecting privacy. So if you want better privacy it will cost just a few dollars a year. It has also been shown time and time again that VPN providers who claim to not keep logs are often not telling the truth. Even if it is true, this does not stop forensics performed on memory. So you may use this software with a VPN to gain even more privacy as an extra layer of protection.

There are also the lists from https://www.iblocklist.com/lists?category=country, which can allow one to block an entire country. While this is usually a horrible to do in Linux, the pf firewall can handle 50,000 ips the same way it handles 5. This software will also convert ip ranges to cidr subnet notation, making the job even easier.

This software requires the following dependencies you may install as root like this:

```
# pkg install curl pkgconf
```

Once installed you will need to run the following as root:

```
# sysrc pf_enable="YES"
pf_enable: NO -> YES
```
You will need to have some kind of /etc/pf.conf for pf and this program to work. If you are unfamilar with this, you may like the following and edit for your needs:

```
# Skip filtering on the loopback interface
set skip on lo0

# Default block all traffic
block all

# Allow SSH traffic
pass in proto tcp to port { 22 }

# Allow DNS, NTP, and HTTP/HTTPS traffic
pass out proto { tcp udp } to port { 22 53 80 123 443 }

# Allow ICMP echo requests
pass out inet proto icmp icmp-type { echoreq }
```

You will also need the following in your /boot/loader.conf:

```
net.pf.states_hashsize=2097152 
net.pf.src_nodes_hashsize=2097152 
net.pf.request_maxcount=3000000
```

As these values are only loaded at boot-time, you will need to reboot to apply them. From there you may run the following as root to install:

```
chmod +x configure
./configure
make install clean
```

This might take a minute to complete, but you should eventually see something like this:

```
# ./configure
Checking for cc...
Checking for pfctl...
Checking for curl...
Checking for pkgconf...
You may now run 'make install clean'.
# make install clean
cc -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized `pkg-config --cflags libcurl` -c retrieve.c -o retrieve.o
cc  -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized -c copy.c -o copy.o
cc  -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized -c parser.c -o parser.o
cc  -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized -c dupe_parse.c -o dupe_parse.o
cc  -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized -c table_loader.c -o table_loader.o
cc  -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized -c pfcount.c -o pfcount.o
cc  -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized -c main.c -o main.o
cc -Wall -Wextra -Werror -pedantic -O2 -pipe -march=native --std=c17 -march=native -fPIC -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat -Wformat-security -flto -fvisibility=hidden -fno-common -Wshadow -fstrict-aliasing -Wuninitialized retrieve.o copy.o parser.o dupe_parse.o table_loader.o pfcount.o main.o -o pfpb -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now,-lssl,-lcrypto,-lz -fPIE -pie -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now `pkg-config --libs libcurl`
mkdir -p /var/pfpb/gzips
mkdir -p /var/pfpb/tables
cp -v config.txt /var/pfpb
config.txt -> /var/pfpb/config.txt
touch /tmp/original_entries.txt
install -m 755 pfpb /usr/sbin/pfpb
Retrieving default free lists. Please wait...
Install is complete.
rm -f retrieve.o copy.o parser.o dupe_parse.o table_loader.o pfcount.o main.o pfpb
```

These are the only command options:

```
Usage: pfpb <command>
Commands:
 pfpb start   Start loading PF tables
 pfpb stop    Stop (flush) PF tables
 pfpb update  Update any new blocklists
```

This program will always need to be ran as root/sudo/doas to ensure it can make changes to the pf firewall. Failure to do so will result in this message:

```
$ pfpb
This program must be run as root.
```

By default, this software comes only with the free lists from iblocklist.com. If you wish to add more you may edit /var/pfpb/config.txt. You will find entries inside it like this:

```
http://list.iblocklist.com/?list=imlmncgrkbnacgcwfjvh&fileformat=p2p&archiveformat=gz;ads
http://list.iblocklist.com/?list=cwworuawihqvocglcoss&fileformat=p2p&archiveformat=gz;bad-peers
http://list.iblocklist.com/?list=gihxqmhyunbxhbmgqrla&fileformat=p2p&archiveformat=gz;bogon
http://list.iblocklist.com/?list=zbdlwrqkabxbcppvrnos&fileformat=p2p&archiveformat=gz;drop
http://list.iblocklist.com/?list=xpbqleszmajjesnzddhv&fileformat=p2p&archiveformat=gz;dshield
http://list.iblocklist.com/?list=imlmncgrkbnacgcwfjvh&fileformat=p2p&archiveformat=gz;edu
http://list.iblocklist.com/?list=ficutxiwawokxlcyoeye&fileformat=p2p&archiveformat=gz;forum-spam
http://list.iblocklist.com/?list=usrcshglbiilevmyfhse&fileformat=p2p&archiveformat=gz;hijacked
http://list.iblocklist.com/?list=pwqnlynprfgtjbgqoizj&fileformat=p2p&archiveformat=gz;iana-multicast
http://list.iblocklist.com/?list=cslpybexmxyuacbyuvib&fileformat=p2p&archiveformat=gz;iana-private
http://list.iblocklist.com/?list=bcoepfyewziejvcqyhqo&fileformat=p2p&archiveformat=gz;iana-reserved
http://list.iblocklist.com/?list=ydxerpxkpcfqjaybcssw&fileformat=p2p&archiveformat=gz;level1
http://list.iblocklist.com/?list=ydxerpxkpcfqjaybcssw&fileformat=p2p&archiveformat=gz;level2
http://list.iblocklist.com/?list=uwnukjqktoggdknzrhgh&fileformat=p2p&archiveformat=gz;level3
http://list.iblocklist.com/?list=xshktygkujudfnjfioro&fileformat=p2p&archiveformat=gz;microsoft
http://list.iblocklist.com/?list=xoebmbyexwuiogmbyprb&fileformat=p2p&archiveformat=gz;proxy
http://list.iblocklist.com/?list=mcvxsnihddgutbjfbghy&fileformat=p2p&archiveformat=gz;spider
http://list.iblocklist.com/?list=llvtlsjyoyiczbkjsxpf&fileformat=p2p&archiveformat=gz;spyware
http://list.iblocklist.com/?list=ghlzqtqxnzctvvajwwag&fileformat=p2p&archiveformat=gz;webexploit
```

To add or change any lists you can just enter another line in this file with the URL to the list followed by a semicolon and the name of the list, or what you wish to call it. Then you will need to restart pfpb and do a new update to apply any new lists. The names on the left will also be the name of the corresponding table created for it with pf:

```
# pfctl -sT
ads
anti-infringement
bad-peers
bogon
drop
dshield
edu
exclusions
for-non-lan-computers
forum-spam
government
hijacked
iana-multicast
iana-private
iana-reserved
level1
level2
level3
malicious
microsoft
piracy-related
prime
proxy
spider
spyware
webexploit
```

This software should work on any BSD, but has only been tested on FreeBSD 14.2 at this time.
