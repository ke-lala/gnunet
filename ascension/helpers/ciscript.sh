# Copyright (C) 2019 rexxnor
# License AGPLv3+: GNU AGPL version 3 or later <https://www.gnu.org/licenses/agpl.html>
# This is free software: you are free to change and redistribute it.
# There is NO WARRANTY, to the extent permitted by law.
apt update
apt install -y python-all python3-stdeb git
python3 setup.py --command-package=stdeb.command sdist_dsc
cd deb_dist/*/
cp ../../helpers/ascension.1 debian/ascension.1
echo "debian/ascension.1" > debian/python3-ascension.manpages
dh_installman
cp ../../helpers/postinst_ascension.sh debian/postinst
cp ../../helpers/copyright debian/copyright
dpkg-buildpackage -rfakeroot -uc -us
cd ../../
apt install -y ./deb_dist/python3-ascension*.deb
ascension -h
bash helpers/gnunet-installer-0.11-debian.sh Production
apt install -y bind9 dnsutils procps
gnunet-arm -Esq
cd ascension/test/
bash test_ascension_simple.sh
gnunet-arm -e
apt install -y dh-make
