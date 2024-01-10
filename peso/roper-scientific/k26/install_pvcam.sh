cp libpvcam.so.2.7.0.0 /usr/lib
ldconfig /usr/lib
rm -f /usr/lib/libpvcam.so
ln -s /usr/lib/libpvcam.so.2.7.0.0 /usr/lib/libpvcam.so
