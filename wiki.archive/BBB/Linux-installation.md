This page is dedicated to the installation of a Linux distribution **on the board**.

The BBB board has both embedded eMMC memory has well as a microSD card port.


## Installing a pre-built image on a microSD card
The Beagleboard website has a [page](https://www.beagleboard.org/distros) listing supported Linux images.
Fetch the [Debian 10.3 SD IoT image](https://www.beagleboard.org/distros/am3358-debian-10-3-2020-04-06-4gb-sd-iot). It may not be the most up-to-date distribution, but it will suffice for our purposes.

You simply need to flash the downloaded file onto a microSD card, for example using [Balena Etcher](https://www.balena.io/etcher).

For future reference, the default login:password is `debian:temppwd`.