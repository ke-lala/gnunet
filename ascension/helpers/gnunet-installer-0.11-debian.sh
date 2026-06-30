#!/bin/bash
# Written by Torsten Grothoff
# Licensed under GPLv3+
# Updated by rexxnor
# SPDX-License-Identifier: GPL3.0-or-later

export bold="\e[1m"
export dim="\e[2m"
export underlined="\e[4m"
export blink="\e[5m"
export inverted="\e[7m"
export hidden="\e[8m"
export resetall="\e[0m" #All
export resetbold="\e[21m" #Bold
export resetdim="\e[22m" #Dim
export resetunderlined="\e[24m" #Underlined
export resetblink="\e[25m" #Blink
export resetinverted="\e[27m" #Inverted
export resethidden="\e[28m" #Hidden
export defaultcolor="\e[39m"
export black="\e[30m"
export red="\e[31m"
export green="\e[32m"
export yellow="\e[33m"
export blue="\e[34m"
export magenta="\e[35m"
export cyan="\e[36m"
export lightgray="\e[37m"
export darkgray="\e[90m"
export lightred="\e[91m"
export lightgreen="\e[92m"
export lightyellow="\e[93m"
export lightblue="\e[94m"
export lightmagenta="\e[95m"
export lightcyan="\e[96m"
export white="\e[97m"
export default
export blackbg="\e[30m"
export redbg="\e[31m"
export greenbg="\e[42m"
export yellowbg="\e[43m"
export bluebg="\e[44m"
export magentabg="\e[45m"
export cyanbg="\e[46m"
export lightgraybg="\e[47m"
export darkgraybg="\e[100m"
export lightredbg="\e[101m"
export lightgreenbg="\e[102m"
export lightyellowbg="\e[103m"
export lightbluebg="\e[104m"
export lightmagentabg="\e[105m"
export lightcyanbg="\e[106m"
export whitebg="\e[107m"
function errorhandler {
  errorcode=$?
  if [[ "$errorcode" != "0" ]]; then
    echo "An Error has Occured [Error Code: $errorcode]."
    if [[ "$errorcode" == "127" ]]; then
      echo "Potential Fatal Error; Exiting"
      exit 127
    fi
  fi
}
if [[ "$UID" -ne 0 ]]; then
  if [[ "$1" == "--ignore-root-check" ]]; then
    echo "Warning: You are not running this file as root; As such, this installer will probabally not work. Expect Errors"
  else
    echo "You must run this program as root that way it can work\nIf you are sure you want to continue without root[will most likely not work]; run this file with --ignore-root-check"
    exit 1
  fi
fi
apt install --yes --fix-missing git libtool autoconf autopoint build-essential libgcrypt-dev libidn11-dev zlib1g-dev libunistring-dev libglpk-dev miniupnpc libextractor-dev libjansson-dev libcurl4-gnutls-dev libsqlite3-dev openssl libnss3-tools
apt install --yes git libtool autoconf autopoint \
build-essential libgcrypt-dev libidn11-dev zlib1g-dev \
libunistring-dev libglpk-dev miniupnpc libextractor-dev \
libjansson-dev libcurl4-gnutls-dev libsqlite3-dev openssl \
libnss3-tools libmicrohttpd-dev libgnutls28-dev libp11-kit-dev libp11-kit0 dialog
errorhandler
mkdir ~/gnunet_installation
cd ~/gnunet_installation
git clone --depth 1 https://gnunet.org/git/gnunet.git
git clone --depth 1 https://gnunet.org/git/libmicrohttpd.git
cd ~/gnunet_installation/libmicrohttpd
autoreconf -fi
apt install -y libgnutls28-dev
./configure --disable-doc --prefix=/opt/libmicrohttpd
make -j$(nproc || echo -n 1)
make install
#
if [[ "$1" == "Production" || "$1" == "Development" ]]; then
  echo "$1" > ./result.txt
fi

if [[ ! -f ./result.txt ]]; then
  dialog --menu Production\ Or\ Developement? -1 -1 2 Production 1 Development 2 2> ./result.txt
fi
if [[ `cat ./result.txt` == Production ]]; then
  cd ~/gnunet_installation/gnunet
  ./bootstrap
  export GNUNET_PREFIX=/usr
  ./configure --prefix=$GNUNET_PREFIX --disable-documentation --with-microhttpd=/opt/libmicrohttpd
  addgroup gnunetdns
  adduser --system --group --disabled-login --home /var/lib/gnunet gnunet
  make -j$(nproc || echo -n 1)
  make install
else
  if [[ `cat ./result.txt` == Developement ]]; then
    cd ~/gnunet_installation/gnunet
    ./bootstrap
    export GNUNET_PREFIX=/usr
    export CFLAGS="-g -Wall -O0"
    ./configure --prefix=$GNUNET_PREFIX --disable-documentation --enable-logging=verbose --with-microhttpd=/opt/libmicrohttpd
    make -j$(nproc || echo -n 1)
    make install
  fi
fi
rm ~/gnunet_installation/libmicrohttpd/result.txt
touch ~/.config/gnunet.conf
echo -e "Associating ${green}gnunet://${resetall} with gnunet"
for n in `cat /etc/passwd | awk '{print $6}' FS=:`; do
  dir=`echo $n/.local/share/applications`
  echo "[Added Associations]
  x-scheme-handler/gnunet=gnunet.desktop" >> $dir/mimeapps.list
  echo "[Desktop Entry]
Version=1.0
Type=Application
Exec=sh -c \"gnunet-uri $@\"
Icon=/usr/share/gnunet/gnunet-logo-color.png
StartupNotify=false
Terminal=false
Categories=Internet
MimeType=x-scheme-handler/gnunet
Name=GNUNet
Comment=GNUNet Protocol Executer
GenericName=GNUNet Protocol Executer[Created By Gnunet Installer; by TStudios]" > $dir/gnunet.desktop
  chmod 644 gnunet.desktop
done
echo -e "To start GNUNet Services: ${green}gnunet-arm -s${resetall}\nTo view GNUNet Services: ${green}gnunet-arm -I${resetall}\nTo Stop GNUNet Services: ${green}gnunet-arm -e${resetall}\n\n${red}NOTE: This GNUNet Installation Might only be available as ${green}ROOT${resetall}\n${lightgreen}For more info: https://gnunet.org/en/tutorial-debian9.html${resetall}"
