## Overview
Metin2 is a MMORPG distributed by GameForge. This hack was developed and tested in a private server (not the official one). It includes, appart from the usual function hooking and memory manipulation, a packet system which allows creating packets, sending them to the server and simulating receiving them in a very easy way. Reversing the network protocol of the game is simpler than reversing the client, and allows you doing things that the client wouldn't allow otherwise.

The hack is used from the client chat. Some of the hacks it includes are wallhack, dmg hack, attracting enemies, chat hacks, etc. Creating bots once you have control over packets would be easy. But there is something better.

## Remote packet injection
After writing a **kick hack** (for kicking other players from the game), I realised there was something else behind it. The way it works is you send a lot of whisper packets to the victim, flooding its client and making it close. However, the truth is that there's a bug that allows you to **inject packets**. The client closes just because the injected packet isn't valid, but if you craft those whispers carefully you can make the victim client process a fake packet. We can do very interesting things, and probably there are more I haven't discovered. Some of them are:
- Kicking the player injecting an invalid packet.
- Making the placer receive a fake whisper from whoever we want, even a Game Master. This includes a server memory leak because strings are not null-terminated.
- Opening the integrated browser, which uses the same API as IE, in any url we want.

This last point is actually a **security flaw**. The browser will happily connect to any UNC path, so we can make anyone connect to a malicious samba server to get his NetNTLM hash, crack it and get the password of the Windows user. Similar to the [Zoom vulnerability](https://www.bleepingcomputer.com/news/security/zoom-lets-attackers-steal-windows-credentials-run-programs-via-unc-links/), but **without user interaction**. This have been successfully tested and exploited.

Other things that I have tried and could be done include:
- Phishing. The URL of the page is not displayed to the victim, so it would be easy. 
- Browser exploitation, without success.
- URI schemes exploitation, without success. The most I could get was popping a calculator or running some Steam games.
- Making the victim download and run a malicious file. Running it and getting RCE is 1 or 2 clicks away.
- DOS, using URI schemes.

A little POC of the remote packet injection can be seen [here](https://www.youtube.com/watch?v=j26L3OChosA). When I discovered this, it was already fixed in the official servers.
