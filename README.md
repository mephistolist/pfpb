# pfpb
Packet Filter Peer Blocker

*See INSTALL for the installation process. 

# Why does this software exist?

Some may ask, what purpose does this software serve? Have not VPNs eliminated the need for peer blocking software? Is it not true most of them no longer work? The lists used by this software are taken from https://www.iblocklist.com/lists. If you view the different paid vs free lists, you will probably see why a paid subscription is much better at protecting privacy. So if you want better privacy it will cost just a few dollars a year. It has also been shown time and time again that VPN providers who claim to not keep logs are often not telling the truth. Even if it is true, this does not stop forensics performed on memory. So you may use this software with a VPN to gain even more privacy as an extra layer of protection.

There are also the lists from https://www.iblocklist.com/lists?category=country, which can allow one to block an entire country. While this is usually a horrible to do in Linux, the pf firewall can handle 50,000 ips the same way it handles 5. This software will also convert ip ranges to cidr subnet notation, making the job even easier.

Most of what this software does is take the lists mentioned and converts them to CIDR notation. These lists originally come ip ranges in hypenated ranges like 127.0.0.1-127.0.0.7, which iptables would accept. However, the pf firewall will only accept ip ranges in CIDR notation. This software is not a deamon, but passes information to the pf firewall to help automate the functions of other peer blocking software like this. 

# Usage
These are the only command options:

```
Usage: pfpb <command>
Commands:
 pfpb start -  Start load the PF tables containing ip ranges from iblocklist to block.
 pfpb stop  -  Stop all PF tables with ips ranges from being blocked. 
 pfpb update - Update any new blocklists so if any new ip ranges exist, they can be applied.
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

If you desire, one can set up a root cron job to start these at boot:

```
@reboot pfpb start >/dev/null
```

You can also automate updating. Something like the follow as a root cron job will update every day at 3am:

```
0 3 * * * pfpb update >/dev/null
```

This software should work on any BSD, but has only been tested on FreeBSD 14.2 and HardenedBSD 14.2 at this time.
