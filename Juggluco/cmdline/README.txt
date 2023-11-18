For more information see:
http://jkaltes.byethost16.com/Juggluco/cmdline/index.html

Compile for debugging with:

cmake -DDEBUG=ON $srcdir
make juggluco

To compile without logging and debug information (in a fresh directory):

cmake $srcdir
make juggluco

Juggluco server can also function as simplified Nightscout/xDrip web server.

SSL doesn't work with the precompiled version, because statically linked programs can't use dlopen.

To use the SSL version, you need an authenticated ssl key for the hostname that is used to contact with the juggluco server.
You can get such a key for your domain name for free using certbot (https://certbot.eff.org/instructions) or 
ZeroSSL (https://www.sslforfree.com/). 

Sometimes there is a separate certificate.pem and chain.pem file. You get the fullchain.pem  by concatenating these files:
cat cert.pem chain.pem  > fullchain.pem 
If you receive the keys from ZeroSSL.com cert.pem is named certificate.crt  and chain.pem is named ca_bundle.crt. 
So you have to do the following:

	cat certificate.crt ca_bundle.crt >  fullchain.pem 

ZeroSSL.com calls prevkey.pem private.key, so you have to do

	cp private.key prevkey.pem

Put fullchain.pem privkey.pem in the jugglucodata directory where also the other data is saved. 

It does not work with Nsclient and scout.
It does work with Garmin Nightscout watches, xDrip and Diabox as followers, Nightwatch, G-Watch as follower
Glimp doesn't connect at all and complaints about invalid nightscout web address also when I enter a webaddress of another nightscout server doesn't it work.
For more information see in Juggluco Left menu->Settings->Web Server->Help.
