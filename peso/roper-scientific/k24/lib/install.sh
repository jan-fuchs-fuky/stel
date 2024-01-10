PV_VER="2.6.5"

echo "--------------------------------"
echo " Instalace knihovny libpvcam.so "
echo "--------------------------------"

rm -fv /lib/libpvcam.*

cp -v libpvcam.so.$PV_VER /usr/lib/
ln -sv /usr/lib/libpvcam.so.$PV_VER /usr/lib/libpvcam.so
ldconfig /usr/lib
