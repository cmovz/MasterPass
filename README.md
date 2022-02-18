# MasterPass
## A deterministic password manager.

This is a small tool I wrote to generate and manage passwords. Unlike most password-managers, there's no encrypted file holding all the passwords, instead it generates passwords deterministically based on a master password.

The main advantage of using this technique is that it doesn't require backups and online services to sync passwords. The same master pass, site and login combination will generate the same password on any machine. If the user has a weak or compromised master password, any hacker anywhere can figure out all of his passwords, so it's extremely important to choose a good, random, unique master password.

An important part of managing passwords this way is to allow changing, perhaps a derived password was exposed or a certain web service requires periodic changes, the way it's handled here is by including a 128-bit counter in the process that can be incremented or reset. This brings the necessity of managing state, which is done by using hashes, no encryption required and no privacy issues (unless SHA256 can be reversed by someone). Does it mean backups are actually required? I don't think so in most cases as you can just derive the next version on another machine.

All derived passwords have `min(master_pass_entropy, 256-bit)` entropy which is converted to a usable pass by encoding both hex and base64 versions. I don't think this is the right way of doing it for use in real websites. Not because it has any problems, but because there are a lot of bad web systems out there with bad password policies. In order to handle those fine I'll replace both encodings by custom alphabets and lengths which are likely to be compatible with most systems out there.

It also remembers the created accounts/safes by storing a file with a name derived from the master password under `$HOME/.master_pass/[hash_here]`.

I tried to be as careful as possible by cleaning up memory when appropriate and locking the stack to memory, but using GTK, there's a lot of memory outside my control. I want to look next on ways of handling that.

## Screenshots
![](https://raw.githubusercontent.com/cmovz/MasterPass/master/screenshots/welcome.png)
![](https://raw.githubusercontent.com/cmovz/MasterPass/master/screenshots/working.png)